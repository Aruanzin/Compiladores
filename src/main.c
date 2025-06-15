#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexico.h"
#include "sintatico.h"
#include "colors.h"
#include "statistics.h"
#include "visualizer.h"

// Global flag for hints
int show_hints = 0;
int interactive_mode = 0;

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
        printf(BOLD CYAN "Uso: " RESET_COLOR "%s [opções] <arquivo.pl0>\n", argv[0]);
        printf("\n" BOLD "Opções:" RESET_COLOR "\n");
        printf("  --hint        Habilitar dicas detalhadas e cores\n");
        printf("  --interactive Modo interativo com pausas\n");
        printf("  --stats       Mostrar apenas estatísticas\n");
        printf("  --help        Mostrar esta ajuda\n");
        printf("\n" BOLD "Exemplos:" RESET_COLOR "\n");
        printf("  %s programa.pl0\n", argv[0]);
        printf("  %s --hint programa.pl0\n", argv[0]);
        printf("  %s --interactive --hint programa.pl0\n", argv[0]);
        return 1;
    }

    // Parse command line arguments
    char* filename = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--hint") == 0) {
            show_hints = 1;
        } else if (strcmp(argv[i], "--interactive") == 0) {
            interactive_mode = 1;
            show_hints = 1; // Interactive mode implies hints
        } else if (strcmp(argv[i], "--stats") == 0) {
            show_hints = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf(BOLD CYAN "Compilador PL/0 Avançado" RESET_COLOR "\n");
            printf("Um analisador léxico e sintático com recursos avançados.\n\n");
            printf(BOLD "Recursos:" RESET_COLOR "\n");
            printf("  " SYMBOL_CHECK " Análise léxica e sintática completa\n");
            printf("  " SYMBOL_CHECK " Recuperação inteligente de erros\n");
            printf("  " SYMBOL_CHECK " Sugestões automáticas de correção\n");
            printf("  " SYMBOL_CHECK " Estatísticas detalhadas de código\n");
            printf("  " SYMBOL_CHECK " Visualização colorida de erros\n");
            printf("  " SYMBOL_CHECK " Métricas de qualidade de código\n");
            return 0;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }
    
    // Use default if no filename provided
    if (!filename) {
        filename = "tests/demo.txt";
        printf(YELLOW "Usando arquivo padrão: %s" RESET_COLOR "\n", filename);
    }
    
    init_error_recovery_stack();
    init_lexer(filename);
    
    if (interactive_mode) {
        printf(BOLD YELLOW SYMBOL_INFO " Modo interativo ativo" RESET_COLOR "\n");
        printf("O compilador pausará em pontos importantes.\n");
        pause_for_user();
    }
    
    parse();
    
    if (interactive_mode && (num_erros_sintaticos > 0 || num_erros_lexicos > 0)) {
        const char* options[] = {
            "Mostrar sugestões de correção",
            "Ver estrutura esperada do código",
            "Continuar"
        };
        
        int choice = ask_user_choice("O que gostaria de fazer?", options, 3);
        
        switch (choice) {
            case 0: // Suggestions
                printf("\n" BOLD MAGENTA SYMBOL_MAGIC " SUGESTÕES GERAIS:" RESET_COLOR "\n");
                printf("1. Verifique se todas as palavras-chave estão escritas corretamente\n");
                printf("2. Certifique-se de que ':=' é usado para atribuição (não '=')\n");
                printf("3. Todo programa deve terminar com '.'\n");
                printf("4. Comandos devem estar dentro de blocos BEGIN...END\n");
                break;
            case 1: // Structure
                print_code_structure_tree();
                break;
            default:
                break;
        }
    }
    
    liberarTabelaReservadas();
    if (error_stack) {
        free(error_stack);
    }
    
    return 0;
}


