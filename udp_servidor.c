#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 5000
#define BUFFER_SIZE 1024
#define FILE_NAME "arquivo.bin"

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    int packets_sent = 0;
    FILE *file;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao bindar o socket");
        exit(EXIT_FAILURE);
    }

    printf("Servidor aguardando na porta %d...\n", PORT);

    client_len = sizeof(client_addr);
    memset(buffer, 0, BUFFER_SIZE);

    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
    printf("Solicitação recebida de %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    file = fopen(FILE_NAME, "rb");

    if (file == NULL) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }

    while (!feof(file)) {
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE, file);
        if (bytes_read > 0) {
            sendto(sockfd, buffer, bytes_read, 0, (struct sockaddr *)&client_addr, client_len);
            packets_sent++;
            printf("Enviando pacote de %ld bytes.\n", bytes_read);
        }
    }

    const char *end_message = "FIM";
    sendto(sockfd, end_message, strlen(end_message), 0, (const struct sockaddr *)&client_addr, client_len);

    printf("Arquivo enviado com sucesso! Total de pacotes enviados: %d\n", packets_sent);
    fclose(file);
    close(sockfd);
    return 0;
}
