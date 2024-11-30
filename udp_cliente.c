#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5000
#define BUFFER_SIZE 1024
#define FILE_NAME "arquivo_recebido.bin"

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_len;
    char buffer[BUFFER_SIZE];
    int packets_rec = 0, packets_lost = 0;
    size_t total_bytes_received = 0;
    FILE *file;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    const char *message = "Enviar arquivo";
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    file = fopen(FILE_NAME, "wb");

    clock_t start_time = clock();

    while (1) {
        ssize_t bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
        if (bytes_received > 0) {
            if (strncmp(buffer, "FIM", 3) == 0) {
                printf("Arquivo transferido com sucesso.\n");
                break;
            }
            fwrite(buffer, 1, bytes_received, file);
            packets_rec++;
            total_bytes_received += bytes_received;
            printf("Pacote recebido com %ld bytes.\n", bytes_received);
        } else {
            packets_lost++;
        }

        if (bytes_received < BUFFER_SIZE) {
            break;
        }
    }

    clock_t end_time = clock();
    double total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    if (total_time > 0) {
        double download_speed = total_bytes_received / total_time;
        printf("Arquivo recebido com sucesso!\n");
        printf("Pacotes recebidos: %d\n", packets_rec);
        printf("Pacotes perdidos: %d\n", packets_lost);
        printf("Taxa de download: %.2f KB/s\n", download_speed / 1024);
    } else {
        printf("Tempo de download inválido.\n");
    }

    fclose(file);
    close(sockfd);
    return 0;
}
