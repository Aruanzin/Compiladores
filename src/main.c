#include <stdio.h>
#include <stdlib.h>
#include "lexico.h"

void analisarArquivo(FILE* arquivo) {
    char linha[256];
    int num_linha = 1;

    printf("DEBUG: Iniciando análise do arquivo\n");

    while (fgets(linha, sizeof(linha), arquivo)) {
        printf("DEBUG: Lendo linha %d\n", num_linha);
        lexico(linha, num_linha);
        num_linha++;
    }

    printf("DEBUG: Finalizada análise do arquivo\n");
}

void imprimeTokens(Token* tokens, int tokenCount) {
    if (tokenCount == 0) {
        printf("ATENÇÃO: Nenhum token encontrado!\n");
        return;
    }
    
    for (int i = 0; i < tokenCount; i++) {
        // Formatação conforme test2.out
        printf("%s, %s\n", tokens[i].lexema, tokens[i].token);
    }
}

int main() {
    char nomeArquivo[256];

    printf("Digite o nome do arquivo: ");
    if (fgets(nomeArquivo, sizeof(nomeArquivo), stdin) != NULL) {
        nomeArquivo[strcspn(nomeArquivo, "\n")] = '\0';
    }

    printf("DEBUG: Abrindo arquivo: '%s'\n", nomeArquivo);
    FILE* arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        printf("Erro ao abrir arquivo: %s\n", nomeArquivo);
        return 1;
    }

    analisarArquivo(arquivo);
    fclose(arquivo);

    imprimeTokens(tokens, tokenCount);

    return 0;
}

