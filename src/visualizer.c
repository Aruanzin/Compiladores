#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "visualizer.h"
#include "colors.h"
#include "lexico.h"

void print_banner() {
    printf("\n");
    printf(BOLD CYAN "╔═══════════════════════════════════════════════════════════════════╗\n" RESET_COLOR);
    printf(BOLD CYAN "║" RESET_COLOR "                   " BOLD YELLOW SYMBOL_ROCKET " COMPILADOR PL/0 AVANÇADO " SYMBOL_ROCKET RESET_COLOR "                  " BOLD CYAN "║\n" RESET_COLOR);
    printf(BOLD CYAN "║" RESET_COLOR "                      " ITALIC "Análise Léxica e Sintática" RESET_COLOR "                   " BOLD CYAN "║\n" RESET_COLOR);
    printf(BOLD CYAN "║" RESET_COLOR "                   " DIM "com Recuperação Inteligente de Erros" RESET_COLOR "            " BOLD CYAN "║\n" RESET_COLOR);
    printf(BOLD CYAN "╚═══════════════════════════════════════════════════════════════════╝\n" RESET_COLOR);
    printf("\n");
}

void print_syntax_highlighted_line(const char* line, int line_number, int error_column) {
    printf(BRIGHT_BLACK "%4d │ " RESET_COLOR, line_number);
    
    int pos = 0;
    int in_comment = 0;
    int in_string = 0;
    
    while (line[pos] != '\0' && line[pos] != '\n') {
        char c = line[pos];
        
        // Check for comments
        if (c == '{' && !in_string) {
            in_comment = 1;
            printf(BRIGHT_BLACK "%c", c);
        } else if (c == '}' && in_comment) {
            in_comment = 0;
            printf("%c" RESET_COLOR, c);
        } else if (in_comment) {
            printf("%c", c);
        }
        // Check for strings (if PL/0 had them)
        else if (c == '"' || c == '\'') {
            in_string = !in_string;
            printf(GREEN "%c", c);
        } else if (in_string) {
            printf("%c", c);
        }
        // Highlight keywords
        else if (isalpha(c)) {
            char word[50] = {0};
            int word_pos = 0;

            while (isalnum(line[pos]) && word_pos < 49) {
                word[word_pos++] = line[pos++];
            }
            pos--; // Back up one
            
            // Check if it's a keyword
            if (strcmp(word, "CONST") == 0 || strcmp(word, "VAR") == 0 || 
                strcmp(word, "PROCEDURE") == 0 || strcmp(word, "BEGIN") == 0 ||
                strcmp(word, "END") == 0 || strcmp(word, "IF") == 0 ||
                strcmp(word, "THEN") == 0 || strcmp(word, "ELSE") == 0 ||
                strcmp(word, "WHILE") == 0 || strcmp(word, "DO") == 0 ||
                strcmp(word, "CALL") == 0 || strcmp(word, "FOR") == 0) {
                printf(BOLD BLUE "%s" RESET_COLOR, word);
            } else {
                // Regular identifier
                if (pos >= error_column - strlen(word) && pos <= error_column) {
                    printf(BOLD RED "%s" RESET_COLOR, word);
                } else {
                    printf(CYAN "%s" RESET_COLOR, word);
                }
            }
        }
        // Highlight numbers
        else if (isdigit(c)) {
            if (pos == error_column) {
                printf(BOLD RED "%c", c);
            } else {
                printf(YELLOW "%c", c);
            }
        }
        // Highlight operators
        else if (strchr("+-*/=<>:;(),.{}", c)) {
            if (pos == error_column) {
                printf(BOLD RED "%c" RESET_COLOR, c);
            } else {
                printf(MAGENTA "%c" RESET_COLOR, c);
            }
        }
        // Regular character
        else {
            if (pos == error_column) {
                printf(BOLD RED "%c" RESET_COLOR, c);
            } else {
                printf("%c", c);
            }
        }
        
        pos++;
    }
    printf(RESET_COLOR "\n");
}

void print_error_context(const char* line, int error_pos, int line_number) {
    printf("\n" BOLD YELLOW SYMBOL_CODE " CONTEXTO DO ERRO:" RESET_COLOR "\n");
    
    // Print line with syntax highlighting
    print_syntax_highlighted_line(line, line_number, error_pos);
    
    // Print error pointer
    printf(BRIGHT_BLACK "     │ " RESET_COLOR);
    for (int i = 0; i < error_pos; i++) {
        printf(" ");
    }
    printf(BOLD RED "▲" RESET_COLOR "\n");
    
    // Print additional context lines if available
    printf(BRIGHT_BLACK "     │ " RESET_COLOR);
    for (int i = 0; i < error_pos; i++) {
        printf(" ");
    }
    printf(BOLD RED "│" RESET_COLOR "\n");
    
    printf(BRIGHT_BLACK "     └─" RESET_COLOR BOLD RED "Erro aqui" RESET_COLOR "\n");
}

void print_summary_box(int errors, int warnings, double time) {
    printf("\n" BOLD CYAN "╔══════════════════════════════════════════════╗\n" RESET_COLOR);
    printf(BOLD CYAN "║" RESET_COLOR "                  " BOLD "RESUMO FINAL" RESET_COLOR "                " BOLD CYAN "║\n" RESET_COLOR);
    printf(BOLD CYAN "╠══════════════════════════════════════════════╣\n" RESET_COLOR);
    
    if (errors == 0 && warnings == 0) {
        printf(BOLD CYAN "║  " RESET_COLOR BOLD GREEN SYMBOL_SUCCESS " Análise concluída com SUCESSO!" RESET_COLOR "          " BOLD CYAN "║\n" RESET_COLOR);
    } else {
        printf(BOLD CYAN "║  " RESET_COLOR BOLD RED SYMBOL_ERROR " Erros encontrados: %-2d" RESET_COLOR "                    " BOLD CYAN "║\n" RESET_COLOR, errors);
        if (warnings > 0) {
            printf(BOLD CYAN "║  " RESET_COLOR BOLD YELLOW SYMBOL_WARNING " Avisos: %-2d" RESET_COLOR "                             " BOLD CYAN "║\n" RESET_COLOR, warnings);
        }
    }
    
    printf(BOLD CYAN "║  " RESET_COLOR CYAN SYMBOL_GEAR " Tempo de processamento: %.4f s" RESET_COLOR "          " BOLD CYAN "║\n" RESET_COLOR, time);
    printf(BOLD CYAN "╚══════════════════════════════════════════════╝\n" RESET_COLOR);
}

void print_underline_error(const char* line, int start_pos, int end_pos) {
    printf(BRIGHT_BLACK "     │ " RESET_COLOR);
    
    for (int i = 0; i < start_pos; i++) {
        printf(" ");
    }
    
    for (int i = start_pos; i <= end_pos && i < strlen(line); i++) {
        printf(BOLD RED "~" RESET_COLOR);
    }
    printf("\n");
}

void highlight_token_in_line(const char* line, const char* token, int position) {
    int token_len = strlen(token);
    
    printf(BRIGHT_BLACK "     │ " RESET_COLOR);
    
    // Print spaces before the token
    for (int i = 0; i < position; i++) {
        printf(" ");
    }
    
    // Highlight the token
    printf(BOLD RED);
    for (int i = 0; i < token_len; i++) {
        printf("^");
    }
    printf(RESET_COLOR "\n");
}

void pause_for_user() {
    printf("\n" DIM "Pressione Enter para continuar..." RESET_COLOR);
    getchar();
}

int ask_user_choice(const char* question, const char* options[], int option_count) {
    printf("\n" BOLD YELLOW SYMBOL_INFO " %s" RESET_COLOR "\n", question);
    
    for (int i = 0; i < option_count; i++) {
        printf("  %d. %s\n", i + 1, options[i]);
    }
    
    printf("\n" CYAN "Escolha (1-%d): " RESET_COLOR, option_count);
    
    int choice;
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > option_count) {
        // Clear input buffer
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return 0; // Invalid choice
    }
    
    // Clear input buffer
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    return choice - 1; // Return 0-based index
}

void print_code_structure_tree() {
    printf("\n" BOLD GREEN SYMBOL_GEAR " ESTRUTURA DO CÓDIGO:" RESET_COLOR "\n");
    printf(BRIGHT_BLACK "├─ " RESET_COLOR CYAN "Programa Principal" RESET_COLOR "\n");
    printf(BRIGHT_BLACK "│  ├─ " RESET_COLOR "Declarações de Constantes\n");
    printf(BRIGHT_BLACK "│  ├─ " RESET_COLOR "Declarações de Variáveis\n");
    printf(BRIGHT_BLACK "│  ├─ " RESET_COLOR "Declarações de Procedimentos\n");
    printf(BRIGHT_BLACK "│  └─ " RESET_COLOR "Bloco Principal (BEGIN...END)\n");
    printf(BRIGHT_BLACK "└─ " RESET_COLOR DIM "Fim do Programa (.)" RESET_COLOR "\n");
}
