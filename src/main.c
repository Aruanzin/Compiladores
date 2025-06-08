#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexico.h"
#include "sintatico.h"

// Global flag for hints
int show_hints = 0;

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

void init_lexer(const char* nome_arquivo) {
    fonte = fopen(nome_arquivo, "r");
    if (!fonte) {
        fprintf(stderr, "Erro ao abrir arquivo fonte: %s\n", nome_arquivo);
        exit(1);
    }
    linha_num = 0;
    pos = 0;
    linha[0] = '\0';
    inicializarTabelaReservadas();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s [--hint] <arquivo.pl0>\n", argv[0]);
        //return 1;
        argv[1] = "tests/test9.txt"; // Default file for testing
    }

    // Check for hint flag
    char* filename = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--hint") == 0) {
            show_hints = 1;
        } else {
            filename = argv[i];
        }
    }
    
    // Use default if no filename provided
    if (!filename) {
        filename = "tests/test9.txt";
    }

    init_lexer(filename);
    parse();
    liberarTabelaReservadas();
    return 0;
}


