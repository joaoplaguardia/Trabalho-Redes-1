#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define MAX_PACKETS 100000

typedef struct {
    uint32_t seq_num;
    size_t data_size;
    char payload[BUFFER_SIZE];
} Packet;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso correto: %s <arquivo_de_saida> <percentual_perda> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *output_file = argv[1];
    int packet_loss_percentage = atoi(argv[2]);
    int port = atoi(argv[3]);

    if (packet_loss_percentage < 0 || packet_loss_percentage > 100) {
        fprintf(stderr, "Erro: A porcentagem de perda deve estar entre 0 e 100.\n");
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    Packet packet;
    char *received_packets[MAX_PACKETS] = {NULL};
    size_t received_sizes[MAX_PACKETS] = {0};
    uint32_t expected_seq = 0;
    int all_received = 1;

    srand(time(NULL));

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro no bind");
        exit(EXIT_FAILURE);
    }
    printf("Servidor aguardando pacotes na porta %d...\n", port);
    printf("Arquivo de saída: %s\n", output_file);
    printf("Porcentagem de perda de pacotes: %d%%\n", packet_loss_percentage);
    
    while (1) {
        ssize_t bytes_received = recvfrom(sockfd, &packet, sizeof(packet), 0,
                                          (struct sockaddr *)&client_addr, &client_len);
        if (bytes_received < 0) {
            perror("Erro ao receber pacote");
            continue;
        }

        uint32_t seq_num = ntohl(packet.seq_num);
        size_t data_size = ntohl(packet.data_size);

        if ((rand() % 100) < packet_loss_percentage) {
            continue;
        }

        char ack_msg[20];

        if (strncmp(packet.payload, "FIM", 3) == 0) {
            for (uint32_t i = 0; i < expected_seq; i++) {
                if (received_packets[i] == NULL) {
                    all_received = 0;
                    break;
                }
            }

            if (all_received) {
                FILE *file = fopen(output_file, "wb");
                if (!file) {
                    perror("Erro ao criar arquivo");
                    exit(EXIT_FAILURE);
                }

                for (uint32_t i = 0; i < expected_seq; i++) {
                    fwrite(received_packets[i], 1, received_sizes[i], file);
                    free(received_packets[i]);
                }
                fclose(file);
            } else {
                printf("Erro: Pacotes perdidos! Arquivo não foi salvo.\n");
            }

            sendto(sockfd, "ACK-FIM", 7, 0, (struct sockaddr *)&client_addr, client_len);
            break;
        }

        if (seq_num != expected_seq) {
            sprintf(ack_msg, "ACK-%u", expected_seq - 1);
        } else {
            received_packets[seq_num] = malloc(data_size);
            if (!received_packets[seq_num]) {
                perror("Erro ao alocar memória");
                exit(EXIT_FAILURE);
            }

            memcpy(received_packets[seq_num], packet.payload, data_size);
            received_sizes[seq_num] = data_size;
            expected_seq++;
            sprintf(ack_msg, "ACK-%u", seq_num);
        }

        sendto(sockfd, ack_msg, strlen(ack_msg), 0, (struct sockaddr *)&client_addr, client_len);
    }

    close(sockfd);
    return 0;
}
