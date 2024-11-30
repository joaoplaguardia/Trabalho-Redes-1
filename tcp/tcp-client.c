#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/evp.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 4096

void calculate_hash(const char *filename, unsigned char *hash_out) {
    FILE *file = fopen(filename, "rb");
    unsigned char buffer[BUFFER_SIZE];
    size_t bytesRead;

    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        perror("Erro ao criar o contexto de hash");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        perror("Erro ao inicializar o hash");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (EVP_DigestUpdate(mdctx, buffer, bytesRead) != 1) {
            perror("Erro ao atualizar o hash");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    if (EVP_DigestFinal_ex(mdctx, hash_out, NULL) != 1) {
        perror("Erro ao finalizar o hash");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    EVP_MD_CTX_free(mdctx);
    fclose(file);
}

int main() {
    int my_socket = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    const char *output_filename = "arquivo_recebido.bin";
    FILE *file;
    clock_t start, end;
    double total_time, download_rate;

    if ((my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Endereço inválido ou não suportado");
        return -1;
    }

    if (connect(my_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Falha na conexão");
        return -1;
    }

    printf("Conectado ao servidor.\n");

    // Pergunta ao cliente se ele quer fazer o download
    char response;
    printf("Você deseja fazer o download do arquivo? (s/n): ");
    scanf(" %c", &response);  // O espaço antes de %c é para capturar o '\n' deixado pelo enter
    if (send(my_socket, &response, 1, 0) < 0) {
        perror("Erro ao enviar resposta ao servidor");
        close(my_socket);
        return -1;
    }

    // Se a resposta não for 's', encerra a conexão
    if (response != 's') {
        printf("Download não realizado. Conexão encerrada.\n");
        close(my_socket);
        return 0;
    }

    // Recebe o hash do servidor
    unsigned char server_hash[EVP_MAX_MD_SIZE];
    ssize_t hashBytesReceived = recv(my_socket, server_hash, EVP_MD_size(EVP_sha256()), MSG_WAITALL);
    if (hashBytesReceived < 0) {
        perror("Erro ao receber o hash");
        close(my_socket);
        return -1;
    }

    // Abre o arquivo para salvar os dados recebidos
    file = fopen(output_filename, "wb");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo para escrita");
        close(my_socket);
        return -1;
    }

    ssize_t fileBytesReceived;
    size_t total_bytes_received = 0;

    start = clock();

    // Recebe o arquivo em blocos
    while ((fileBytesReceived = recv(my_socket, buffer, BUFFER_SIZE, MSG_WAITALL)) > 0) {
        if (fwrite(buffer, 1, fileBytesReceived, file) != fileBytesReceived) {
            perror("Erro ao escrever no arquivo");
            fclose(file);
            close(my_socket);
            return -1;
        }
        total_bytes_received += fileBytesReceived;
    }

    end = clock();
    total_time = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("Arquivo recebido.\n");
    fclose(file);

    // Calcula e imprime a taxa de download
    download_rate = total_bytes_received / total_time;
    printf("Tempo de download: %.3f segundos\n", total_time);
    printf("Taxa de download: %.2f b/s\n", download_rate);

    // Calcula o hash do arquivo recebido
    unsigned char client_hash[EVP_MAX_MD_SIZE];
    calculate_hash(output_filename, client_hash);

    // Verifica se o hash do cliente corresponde ao hash do servidor
    int match = 1;
    printf("Hash do servidor: ");
    for (int i = 0; i < EVP_MD_size(EVP_sha256()); i++) {
        printf("%02x", server_hash[i]);
    }
    printf("\n");

    printf("Hash do cliente: ");
    for (int i = 0; i < EVP_MD_size(EVP_sha256()); i++) {
        printf("%02x", client_hash[i]);
    }
    printf("\n");

    for (int i = 0; i < EVP_MD_size(EVP_sha256()); i++) {
        if (server_hash[i] != client_hash[i]) {
            match = 0;
            break;
        }
    }

    if (match) {
        printf("Arquivo recebido corretamente.\n");
    } else {
        printf("Erro. Arquivo corrompido.\n");
    }

    close(my_socket);

    // Grava os resultados de download em um arquivo de log
    FILE *result_file = fopen("resultados-tcp.txt", "a");
    if (result_file == NULL) {
        perror("Erro ao abrir o arquivo de resultados");
        return -1;
    }

    fprintf(result_file, "%.3f segundos\n", total_time);
    fprintf(result_file, "%.2f b/s\n", download_rate);
    fclose(result_file);

    return 0;
}
