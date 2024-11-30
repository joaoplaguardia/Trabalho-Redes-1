#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/evp.h>

#define PORT 8080
#define BUFFER_SIZE 4096

void calculate_hash(FILE *file, unsigned char *hash_out) {
    EVP_MD_CTX *mdctx;
    unsigned char buffer[BUFFER_SIZE];
    size_t bytesRead;

    mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        perror("Erro ao criar o contexto de hash");
        return;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        perror("Erro ao inicializar o hash");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (EVP_DigestUpdate(mdctx, buffer, bytesRead) != 1) {
            perror("Erro ao atualizar o hash");
            EVP_MD_CTX_free(mdctx);
            return;
        }
    }

    if (EVP_DigestFinal_ex(mdctx, hash_out, NULL) != 1) {
        perror("Erro ao finalizar o hash");
    }

    EVP_MD_CTX_free(mdctx);
}

int main() {
    int my_socket, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    const char *filename = "arquivo.bin";
    FILE *file;

    if ((my_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Falha ao criar o socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(my_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Falha no bind");
        close(my_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(my_socket, 3) < 0) {
        perror("Falha no listen");
        close(my_socket);
        exit(EXIT_FAILURE);
    }

    if ((client_socket = accept(my_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Falha no accept");
        close(my_socket);
        exit(EXIT_FAILURE);
    }

    // Perguntar se o cliente quer fazer o download
    char response;
    if (recv(client_socket, &response, 1, 0) < 0) {
        perror("Erro ao receber a resposta do cliente");
        close(client_socket);
        close(my_socket);
        exit(EXIT_FAILURE);
    }

    // Se a resposta não for 's', fecha a conexão
    if (response != 's') {
        printf("Cliente não quis fazer o download. Conexão fechada.\n");
        close(client_socket);
        close(my_socket);
        return 0;
    }

    file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Falha ao abrir o arquivo");
        close(client_socket);
        close(my_socket);
        exit(EXIT_FAILURE);
    }

    unsigned char hash_out[EVP_MAX_MD_SIZE];
    calculate_hash(file, hash_out);
    fclose(file);

    // Enviar o hash para o cliente
    if (send(client_socket, hash_out, EVP_MD_size(EVP_sha256()), 0) < 0) {
        perror("Erro ao enviar hash");
        close(client_socket);
        close(my_socket);
        exit(EXIT_FAILURE);
    }

    file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Falha ao reabrir o arquivo");
        close(client_socket);
        close(my_socket);
        exit(EXIT_FAILURE);
    }

    // Enviar o arquivo em blocos
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytesRead, 0) < 0) {
            perror("Erro ao enviar dados");
            fclose(file);
            close(client_socket);
            close(my_socket);
            exit(EXIT_FAILURE);
        }
    }

	printf("Arquivo enviado. Fechando conexão...\n");

    fclose(file);
    close(client_socket);
    close(my_socket);

    return 0;
}
