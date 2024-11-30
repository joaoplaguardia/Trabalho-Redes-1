#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *file = fopen("arquivo.bin", "wb");
    if (!file) {
        perror("Erro ao criar o arquivo");
        return 1;
    }

    for (int i = 0; i < 10 * 1024 * 1024; i++) { // 10 MB
        unsigned char byte = rand() % 256; // Gera um byte aleatório
        fwrite(&byte, 1, 1, file);
    }

    fclose(file);
    printf("Arquivo binário de 10 MB criado com sucesso.\n");
    return 0;
}
