#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096  // Defina o tamanho do buffer

// Função para comparar dois arquivos binários
int compare_files(FILE *file1, FILE *file2) {
    unsigned char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];
    size_t bytesRead1, bytesRead2;

    // Ler os arquivos em blocos e comparar byte a byte
    while ((bytesRead1 = fread(buffer1, 1, BUFFER_SIZE, file1)) > 0) {
        bytesRead2 = fread(buffer2, 1, BUFFER_SIZE, file2);

        // Se o número de bytes lidos for diferente, os arquivos são diferentes
        if (bytesRead1 != bytesRead2) {
            return 0;  // Arquivos diferentes
        }

        // Comparar os blocos lidos
        if (memcmp(buffer1, buffer2, bytesRead1) != 0) {
            return 0;  // Arquivos diferentes
        }
    }

    // Verificar se o arquivo 2 chegou ao final ao mesmo tempo que o arquivo 1
    if (fread(buffer2, 1, BUFFER_SIZE, file2) > 0) {
        return 0;  // Arquivos diferentes
    }

    return 1;  // Arquivos são iguais
}

int main() {
    const char *file1_name = "arquivo.bin";  // Caminho do arquivo enviado
    const char *file2_name = "arquivo_recebido.bin";  // Caminho do arquivo recebido

    FILE *file1 = fopen(file1_name, "rb");
    FILE *file2 = fopen(file2_name, "rb");

    if (!file1) {
        perror("Erro ao abrir o arquivo enviado");
        return 1;
    }

    if (!file2) {
        perror("Erro ao abrir o arquivo recebido");
        fclose(file1);
        return 1;
    }

    // Comparar os arquivos
    if (compare_files(file1, file2)) {
        printf("Os arquivos são iguais.\n");
    } else {
        printf("Os arquivos são diferentes.\n");
    }

    // Fechar os arquivos
    fclose(file1);
    fclose(file2);

    return 0;
}
