#include <stdio.h>
#include <stdlib.h>


void analisarArquivo(FILE* arquivo) {
    char linha[256];
    int num_linha = 1;
    int aux_sintitico = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        // analisarLinha(linha, num_linha);
        lexico(linha, num_linha);
        // sintatico(&aux_sintitico);
        num_linha++;
    }

}

int main() {
    char nomeArquivo[256];

    printf("Digite o nome do arquivo: ");
    if (fgets(nomeArquivo, sizeof(nomeArquivo), stdin) != NULL) {
        nomeArquivo[strcspn(nomeArquivo, "\n")] = '\0';
    }

    FILE* arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        printf("Erro ao abrir arquivo: %s\n", nomeArquivo);
        return 1;
    }

    // analisarArquivo(arquivo);

    fclose(arquivo);

    // imprimeTokens(tokens, tokenCount);
    // imprimeTokensT2(tokens, tokenCount);

    return 0;
}

