#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define FILE_PATH "arquivos/teste.bin"
#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void handle_client(int client_socket) {
    char request[BUFFER_SIZE];
    char response_header[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    int n;

    n = read(client_socket, request, sizeof(request) - 1);
    if (n < 0) {
        error("ERROR reading from socket");
    }
    request[n] = '\0';

    printf("Requisição recebida:\n%s\n", request);

    if (strncmp(request, "GET", 3) == 0) {
        FILE *file = fopen(FILE_PATH, "rb");
        if (!file) {
            snprintf(response_header, sizeof(response_header),
                     "HTTP/1.1 404 Not Found\r\n"
                     "Content-Length: 0\r\n"
                     "Connection: Closed\r\n\r\n");
            write(client_socket, response_header, strlen(response_header));
            close(client_socket);
            return;
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %ld\r\n"
                 "Content-Type: application/octet-stream\r\n"
                 "Connection: Closed\r\n\r\n",
                 file_size);
        write(client_socket, response_header, strlen(response_header));

        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            write(client_socket, buffer, bytes_read);
        }

        fclose(file);
    } else {
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 400 Bad Request\r\n"
                 "Content-Length: 0\r\n"
                 "Connection: Closed\r\n\r\n");
        write(client_socket, response_header, strlen(response_header));
    }

    close(client_socket);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    printf("Servidor iterativo em execução na porta %d\n", portno);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");

        printf("Cliente conectado.\n");
        handle_client(newsockfd);
    }

    close(sockfd);
    return 0;
}
