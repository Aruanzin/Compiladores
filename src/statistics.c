#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "statistics.h"
#include "colors.h"
#include "lexico.h"

CompilerStats stats = {0};

void init_statistics() {
    memset(&stats, 0, sizeof(CompilerStats));
    stats.start_time = clock();
}

void update_token_stats(TokenType type) {
    stats.total_tokens++;
    
    switch (type) {
        case TOKEN_IDENTIFIER:
            stats.total_identifiers++;
            break;
        case TOKEN_NUMBER:
            stats.total_numbers++;
            break;
        case TOKEN_CONST:
        case TOKEN_VAR:
        case TOKEN_PROCEDURE:
        case TOKEN_BEGIN:
        case TOKEN_END:
        case TOKEN_IF:
        case TOKEN_THEN:
        case TOKEN_ELSE:
        case TOKEN_WHILE:
        case TOKEN_DO:
        case TOKEN_FOR:
        case TOKEN_CALL:
            stats.total_keywords++;
            break;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MULT:
        case TOKEN_DIV:
        case TOKEN_ASSIGN:
        case TOKEN_EQUAL:
        case TOKEN_NE:
        case TOKEN_LT:
        case TOKEN_LE:
        case TOKEN_GT:
        case TOKEN_GE:
            stats.total_operators++;
            break;
        default:
            break;
    }
}

void update_error_stats(int line, const char* error_msg) {
    if (line < 1000) {
        stats.errors_by_line[line]++;
        if (line > stats.max_line) {
            stats.max_line = line;
        }
    }
    
    // Track common errors
    int found = 0;
    for (int i = 0; i < stats.unique_errors; i++) {
        if (strstr(stats.common_errors[i], error_msg)) {
            stats.error_counts[i]++;
            found = 1;
            break;
        }
    }
    
    if (!found && stats.unique_errors < 10) {
        strncpy(stats.common_errors[stats.unique_errors], error_msg, 99);
        stats.common_errors[stats.unique_errors][99] = '\0';
        stats.error_counts[stats.unique_errors] = 1;
        stats.unique_errors++;
    }
}

void start_parsing_timer() {
    stats.start_time = clock();
}

void end_parsing_timer() {
    stats.end_time = clock();
    stats.parsing_time = ((double)(stats.end_time - stats.start_time)) / CLOCKS_PER_SEC;
}

void print_detailed_statistics() {
    printf("\n" BOLD CYAN "═══════════════════════════════════════════════════════════" RESET_COLOR "\n");
    printf(BOLD CYAN "                    📊 ESTATÍSTICAS DETALHADAS                   " RESET_COLOR "\n");
    printf(BOLD CYAN "═══════════════════════════════════════════════════════════" RESET_COLOR "\n\n");
    
    // Análise Léxica
    printf(BOLD GREEN SYMBOL_GEAR " ANÁLISE LÉXICA:" RESET_COLOR "\n");
    printf("  " CYAN "Total de tokens:" RESET_COLOR " %d\n", stats.total_tokens);
    printf("  " CYAN "Identificadores:" RESET_COLOR " %d\n", stats.total_identifiers);
    printf("  " CYAN "Números:" RESET_COLOR " %d\n", stats.total_numbers);
    printf("  " CYAN "Palavras-chave:" RESET_COLOR " %d\n", stats.total_keywords);
    printf("  " CYAN "Operadores:" RESET_COLOR " %d\n", stats.total_operators);
    printf("  " RED "Erros léxicos:" RESET_COLOR " %d\n", stats.lexical_errors);
    
    // Análise Sintática
    printf("\n" BOLD GREEN SYMBOL_GEAR " ANÁLISE SINTÁTICA:" RESET_COLOR "\n");
    printf("  " CYAN "Total de linhas:" RESET_COLOR " %d\n", stats.total_lines);
    printf("  " CYAN "Procedimentos:" RESET_COLOR " %d\n", stats.total_procedures);
    printf("  " CYAN "Variáveis:" RESET_COLOR " %d\n", stats.total_variables);
    printf("  " CYAN "Constantes:" RESET_COLOR " %d\n", stats.total_constants);
    printf("  " CYAN "Chamadas:" RESET_COLOR " %d\n", stats.total_calls);
    printf("  " CYAN "Profundidade máxima:" RESET_COLOR " %d\n", stats.max_nesting_depth);
    printf("  " RED "Erros sintáticos:" RESET_COLOR " %d\n", stats.syntax_errors);
    
    // Recuperação de Erros
    printf("\n" BOLD GREEN SYMBOL_GEAR " RECUPERAÇÃO DE ERROS:" RESET_COLOR "\n");
    printf("  " GREEN "Recuperações bem-sucedidas:" RESET_COLOR " %d\n", stats.recoveries_successful);
    printf("  " RED "Recuperações falharam:" RESET_COLOR " %d\n", stats.recoveries_failed);
    
    if (stats.recoveries_successful + stats.recoveries_failed > 0) {
        double success_rate = (double)stats.recoveries_successful / 
                             (stats.recoveries_successful + stats.recoveries_failed) * 100;
        printf("  " YELLOW "Taxa de sucesso:" RESET_COLOR " %.1f%%\n", success_rate);
    }
}

void print_performance_metrics() {
    printf("\n" BOLD YELLOW SYMBOL_ROCKET " MÉTRICAS DE PERFORMANCE:" RESET_COLOR "\n");
    printf("  " CYAN "Tempo de análise:" RESET_COLOR " %.4f segundos\n", stats.parsing_time);
    
    if (stats.parsing_time > 0) {
        printf("  " CYAN "Tokens por segundo:" RESET_COLOR " %.0f\n", 
               stats.total_tokens / stats.parsing_time);
        printf("  " CYAN "Linhas por segundo:" RESET_COLOR " %.0f\n", 
               stats.total_lines / stats.parsing_time);
    }
    
    // Memory efficiency (estimated)
    int estimated_memory = stats.total_tokens * sizeof(Token) + 
                          stats.total_procedures * 100 + 
                          stats.total_variables * 50;
    printf("  " CYAN "Memória estimada:" RESET_COLOR " %d bytes\n", estimated_memory);
}

void print_code_quality_metrics() {
    printf("\n" BOLD MAGENTA SYMBOL_MAGIC " MÉTRICAS DE QUALIDADE:" RESET_COLOR "\n");
    
    // Complexity score
    double complexity = stats.max_nesting_depth * 0.3 + 
                       stats.total_procedures * 0.2 + 
                       (stats.total_tokens > 0 ? (double)stats.total_operators / stats.total_tokens : 0) * 0.5;
    
    printf("  " CYAN "Índice de complexidade:" RESET_COLOR " %.2f", complexity);
    if (complexity < 2.0) {
        printf(" " GREEN "(Simples)" RESET_COLOR "\n");
    } else if (complexity < 4.0) {
        printf(" " YELLOW "(Moderado)" RESET_COLOR "\n");
    } else {
        printf(" " RED "(Complexo)" RESET_COLOR "\n");
    }
    
    // Error density
    if (stats.total_lines > 0) {
        double error_density = (double)(stats.lexical_errors + stats.syntax_errors) / stats.total_lines * 100;
        printf("  " CYAN "Densidade de erros:" RESET_COLOR " %.2f%% (erros por linha)\n", error_density);
    }
    
    // Code structure quality
    if (stats.total_procedures > 0) {
        double avg_proc_size = (double)stats.total_tokens / stats.total_procedures;
        printf("  " CYAN "Tamanho médio de procedimento:" RESET_COLOR " %.1f tokens\n", avg_proc_size);
    }
    
    // Most common errors
    if (stats.unique_errors > 0) {
        printf("\n" BOLD RED "  🔥 ERROS MAIS COMUNS:" RESET_COLOR "\n");
        for (int i = 0; i < stats.unique_errors && i < 5; i++) {
            printf("    %d. %s " BRIGHT_BLACK "(%d ocorrências)" RESET_COLOR "\n", 
                   i + 1, stats.common_errors[i], stats.error_counts[i]);
        }
    }
}
