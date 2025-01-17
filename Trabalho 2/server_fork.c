#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>

#define FILEPATH "arquivos/teste.bin"
#define BUFFERSIZE 1024

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    char buffer[BUFFERSIZE];

    read(client_socket, buffer, sizeof(buffer) - 1);
    printf("Requisição recebida:\n%s\n", buffer);

    if (strncmp(buffer, "GET", 3) == 0) {
        int file_fd = open(FILEPATH, O_RDONLY);
        if (file_fd < 0) {
            perror("Erro ao abrir arquivo");
            const char *error_response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            write(client_socket, error_response, strlen(error_response));
            close(client_socket);
            return NULL;
        }

        off_t file_size = lseek(file_fd, 0, SEEK_END);
        lseek(file_fd, 0, SEEK_SET);

        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %ld\r\n"
                 "Content-Type: application/octet-stream\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 file_size);
        write(client_socket, header, strlen(header));

        char file_buffer[BUFFERSIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(file_fd, file_buffer, sizeof(file_buffer))) > 0) {
            write(client_socket, file_buffer, bytes_read);
        }

        close(file_fd);
        printf("Arquivo enviado com sucesso.\n");
    } else {
        const char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        write(client_socket, response, strlen(response));
    }

    close(client_socket);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(1);
    }

    int portno = atoi(argv[1]);
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro no bind");
        close(server_socket);
        exit(1);
    }

    listen(server_socket, 5);
    printf("Servidor escutando na porta %d...\n", portno);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&cli_addr, &clilen);
        if (client_socket < 0) {
            perror("Erro no accept");
            continue;
        }

        int *client_socket_ptr = malloc(sizeof(int));
        if (client_socket_ptr == NULL) {
            perror("Erro ao alocar memória");
            close(client_socket);
            continue;
        }
        *client_socket_ptr = client_socket;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket_ptr) != 0) {
            perror("Erro ao criar thread");
            close(client_socket);
            free(client_socket_ptr);
        }

        pthread_detach(thread_id);
    }

    close(server_socket);
    return 0;
}
