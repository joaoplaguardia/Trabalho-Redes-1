#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define FILE_PATH "file.bin"  // Caminho para o arquivo binário de 10 MB

int main() {
    fd_set master_set, read_fds;
    struct sockaddr_in server_addr, client_addr;
    int listener, new_socket, max_fd, addr_len, i, nbytes;
    char buffer[BUFFER_SIZE];
    FILE *file;
    size_t file_size;
    int yes = 1;

    FD_ZERO(&master_set);
    FD_ZERO(&read_fds);

    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    FD_SET(listener, &master_set);
    max_fd = listener;

    printf("Server listening on port %d\n", PORT);

    while (1) {
        read_fds = master_set;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    addr_len = sizeof(client_addr);
                    if ((new_socket = accept(listener, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
                        perror("accept");
                    } else {
                        FD_SET(new_socket, &master_set);
                        if (new_socket > max_fd) {
                            max_fd = new_socket;
                        }
                        printf("New connection from %s on socket %d\n", inet_ntoa(client_addr.sin_addr), new_socket);
                    }
                } else {
                    if ((nbytes = recv(i, buffer, sizeof(buffer) - 1, 0)) <= 0) {
                        if (nbytes == 0) {
                            printf("Socket %d closed\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master_set);
                    } else {
                        buffer[nbytes] = '\0';
                        printf("Received request:\n%s\n", buffer);

                        // Abrir o arquivo binário
                        file = fopen(FILE_PATH, "rb");
                        if (file == NULL) {
                            perror("fopen");
                            close(i);
                            FD_CLR(i, &master_set);
                            break;
                        }

                        // Obter o tamanho do arquivo
                        fseek(file, 0, SEEK_END);
                        file_size = ftell(file);
                        fseek(file, 0, SEEK_SET);

                        // Enviar cabeçalho HTTP
                        const char *response_header = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n";
                        char response_header_buffer[256];
                        snprintf(response_header_buffer, sizeof(response_header_buffer), response_header, file_size);
                        
                        if (send(i, response_header_buffer, strlen(response_header_buffer), 0) == -1) {
                            perror("send() error");
                            fclose(file);
                            close(i);
                            FD_CLR(i, &master_set);
                            break;
                        }

                        // Enviar o arquivo em partes
                        size_t bytes_sent = 0;
                        while (bytes_sent < file_size) {
                            size_t to_send = (file_size - bytes_sent < BUFFER_SIZE) ? (file_size - bytes_sent) : BUFFER_SIZE;
                            size_t read_bytes = fread(buffer, 1, to_send, file);
                            if (read_bytes > 0) {
                                ssize_t sent = send(i, buffer, read_bytes, 0);
                                if (sent == -1) {
                                    perror("send() error");
                                    break;
                                }
                                bytes_sent += sent;
                            } else {
                                break;
                            }
                        }

                        fclose(file);
                        close(i);
                        FD_CLR(i, &master_set);
                    }
                }
            }
        }
    }

    return 0;
}
