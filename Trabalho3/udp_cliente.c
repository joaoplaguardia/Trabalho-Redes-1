#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define N_DESISTENCIA 5  // Número máximo de timeouts consecutivos antes de desistir

typedef struct {
    uint32_t seq_num;
    size_t data_size;
    char payload[BUFFER_SIZE];
} Packet;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <arquivo> <timeout_em_segundos> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *file_name = argv[1];
    double timeout_val = atof(argv[2]);  // Converte para double
    int port = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in server_addr;
    struct timeval timeout;
    timeout.tv_sec = (int)timeout_val;
    timeout.tv_usec = (int)((timeout_val - timeout.tv_sec) * 1e6);

    socklen_t server_len = sizeof(server_addr);
    Packet packet;
    char ack[BUFFER_SIZE];
    uint32_t packet_num = 0;
    int total_retransmissions = 0;  // Contador total de retransmissões

    // Variáveis para medir o tempo de execução (a partir do primeiro ACK recebido)
    struct timeval start_time, end_time;
    int timing_started = 0;

    FILE *file = fopen(file_name, "rb");
    if (!file) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Laço principal: para cada bloco lido do arquivo
    while (1) {
        size_t bytes_read = fread(packet.payload, 1, BUFFER_SIZE, file);
        if (bytes_read == 0) { // Fim do arquivo
            break;
        }

        packet.seq_num = htonl(packet_num);
        packet.data_size = htonl(bytes_read);

        int acked = 0;
        int timeout_count = 0;  // Para este pacote

        // Laço de retransmissão: tenta enviar o mesmo pacote até ser ACKado
        while (!acked) {
            if (sendto(sockfd, &packet, sizeof(packet.seq_num) + sizeof(packet.data_size) + bytes_read, 0,
                       (struct sockaddr *)&server_addr, server_len) < 0) {
                perror("Erro ao enviar pacote");
                exit(EXIT_FAILURE);
            }
            printf("Enviado pacote #%u (%zu bytes)\n", packet_num, bytes_read);

            ssize_t ack_size = recvfrom(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&server_addr, &server_len);
            if (ack_size < 0) {
                timeout_count++;
                total_retransmissions++;
                printf("Timeout aguardando ACK para pacote #%u. (Timeouts consecutivos: %d)\n", packet_num, timeout_count);
                if (timeout_count >= N_DESISTENCIA) {
                    printf("Número máximo de timeouts consecutivos atingido para o pacote #%u. Desistindo da transmissão.\n", packet_num);
                    goto fim;
                }
                // Tenta novamente enviar o mesmo pacote (sem avançar o arquivo)
            } else {
                ack[ack_size] = '\0';
                int received_ack = -1;
                if (sscanf(ack, "ACK-%d", &received_ack) != 1) {
                    printf("Formato de ACK inválido: %s\n", ack);
                    timeout_count++;
                    total_retransmissions++;
                    continue; // Tenta novamente
                }
                // Inicia a medição do tempo a partir do primeiro ACK recebido
                if (!timing_started) {
                    gettimeofday(&start_time, NULL);
                    timing_started = 1;
                }
                printf("Esperado: #%u, Recebido ACK: #%d\n", packet_num, received_ack);
                if (received_ack != packet_num) {
                    printf("ACK incorreto para pacote #%u! (Recebido: #%d)\n", packet_num, received_ack);
                    timeout_count++;
                    total_retransmissions++;
                    // Tenta novamente enviar o mesmo pacote
                } else {
                    printf("Recebido ACK correto: #%d\n", received_ack);
                    acked = 1;  // Pacote ACKado corretamente
                }
            }
        } // Fim do laço de retransmissão

        packet_num++;  // Avança para o próximo pacote
    } // Fim do laço principal

fim:
    // Envia pacote de término ("FIM")
    snprintf(packet.payload, sizeof(packet.payload), "FIM");
    packet.seq_num = htonl(packet_num);
    packet.data_size = 0;
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, server_len);
    printf("Enviado pacote de término.\n");

    // Finaliza a medição do tempo, se iniciado
    if (timing_started) {
        gettimeofday(&end_time, NULL);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                         (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
        printf("Tempo total de transmissão: %.6f segundos.\n", elapsed);
    } else {
        printf("Nenhum ACK foi recebido. Tempo de transmissão não medido.\n");
    }

    fclose(file);
    close(sockfd);
    printf("Transferência concluída! Número total de retransmissões: %d\n", total_retransmissions);

    return 0;
}
