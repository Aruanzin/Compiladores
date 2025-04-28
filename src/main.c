#include <stdio.h>
#include <stdlib.h>
#include "lexico.h"

static const char *tokenTypeNames[] = {
    /* reserved words */
    "CALL","VAR","BEGIN","END","WHILE","CONST","PROCEDURE","ELSE",
    "THEN","IF","DO","FOR",
    /* symbols */
    "simbolo_ponto_virgula","simbolo_dois_pontos","simbolo_mais","simbolo_menos",
    "simbolo_multiplicacao","simbolo_divisao","simbolo_abre_parenteses","simbolo_fecha_parenteses",
    "simbolo_igual","simbolo_virgula","simbolo_maior","simbolo_menor","simbolo_ponto",
    "simbolo_menor_igual","simbolo_maior_igual","simbolo_diferente","simbolo_atribuicao",
    /* generic */
    "ident","numero","<ERRO_LEXICO>"
};

void analisarArquivo(FILE* arquivo) {
    char linha[256];
    int num_linha = 1;
    while (fgets(linha, sizeof(linha), arquivo)) {
        lexico(linha, num_linha);
        num_linha++;
    }
}

void imprimeTokens(Token* tokens, int tokenCount) {
    if (tokenCount == 0) {
        printf("ATENÇÃO: Nenhum token encontrado!\n");
        return;
    }
    
    for (int i = 0; i < tokenCount; i++) {
        printf("%s, %s\n",
               tokens[i].lexema,
               tokenTypeNames[tokens[i].tipo]);
    }
}

int main() {
    char nomeArquivo[256];

    if (fgets(nomeArquivo, sizeof(nomeArquivo), stdin) != NULL) {
        nomeArquivo[strcspn(nomeArquivo, "\n")] = '\0';
    }

    FILE* arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        fprintf(stderr, "Erro ao abrir arquivo: %s\n", nomeArquivo);
        return 1;
    }

    analisarArquivo(arquivo);
    fclose(arquivo);

    imprimeTokens(tokens, tokenCount);
    
    // Clean up the hash table
    liberarTabelaReservadas();

    return 0;
}

