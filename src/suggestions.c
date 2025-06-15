#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include "suggestions.h"
#include "colors.h"
#include "lexico.h"
#include "utils.h"

// Keywords for fuzzy matching
static const char* pl0_keywords[] = {
    "CONST", "VAR", "PROCEDURE", "BEGIN", "END", "IF", "THEN", "ELSE",
    "WHILE", "DO", "FOR", "CALL", "ODD", NULL
};

int levenshtein_distance(const char* s1, const char* s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    
    if (len1 == 0) return len2;
    if (len2 == 0) return len1;
    
    int** matrix = malloc((len1 + 1) * sizeof(int*));
    for (int i = 0; i <= len1; i++) {
        matrix[i] = malloc((len2 + 1) * sizeof(int));
    }
    
    for (int i = 0; i <= len1; i++) matrix[i][0] = i;
    for (int j = 0; j <= len2; j++) matrix[0][j] = j;
    
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            matrix[i][j] = fmin(fmin(
                matrix[i-1][j] + 1,      // deletion
                matrix[i][j-1] + 1),     // insertion
                matrix[i-1][j-1] + cost  // substitution
            );
        }
    }
    
    int result = matrix[len1][len2];
    
    for (int i = 0; i <= len1; i++) {
        free(matrix[i]);
    }
    free(matrix);
    
    return result;
}

char* find_closest_keyword(const char* input) {
    int min_distance = INT_MAX;
    const char* best_match = NULL;
    
    for (int i = 0; pl0_keywords[i] != NULL; i++) {
        int distance = levenshtein_distance(input, pl0_keywords[i]);
        if (distance < min_distance && distance <= 3) { // Max 3 character differences
            min_distance = distance;
            best_match = pl0_keywords[i];
        }
    }
    
    return best_match ? string_duplicate(best_match) : NULL;
}

SuggestionList suggest_identifier_fix(const char* invalid_identifier) {
    SuggestionList suggestions = {0};
    
    // Remove invalid characters
    char clean_id[100] = {0};
    int clean_pos = 0;
    
    for (int i = 0; invalid_identifier[i] && clean_pos < 99; i++) {
        char c = invalid_identifier[i];
        if (isalnum(c) || c == '_') {
            clean_id[clean_pos++] = c;
        }
    }
    
    if (clean_pos > 0) {
        snprintf(suggestions.suggestions[0].suggestion, 100, "%s", clean_id);
        suggestions.suggestions[0].confidence = 0.9;
        snprintf(suggestions.suggestions[0].explanation, 200, 
                "Remover caracteres inválidos: '%s'", clean_id);
        suggestions.count = 1;
    }
    
    // Check if it's a misspelled keyword
    char* closest = find_closest_keyword(invalid_identifier);
    if (closest && suggestions.count < 5) {
        snprintf(suggestions.suggestions[suggestions.count].suggestion, 100, "%s", closest);
        suggestions.suggestions[suggestions.count].confidence = 0.8;
        snprintf(suggestions.suggestions[suggestions.count].explanation, 200, 
                "Possível palavra-chave: '%s'", closest);
        suggestions.count++;
        free(closest);
    }
    
    return suggestions;
}

SuggestionList suggest_keyword_fix(const char* invalid_keyword, TokenType expected_context) {
    SuggestionList suggestions = {0};
    
    char* closest = find_closest_keyword(invalid_keyword);
    if (closest) {
        snprintf(suggestions.suggestions[0].suggestion, 100, "%s", closest);
        suggestions.suggestions[0].confidence = 0.9;
        snprintf(suggestions.suggestions[0].explanation, 200, 
                "Correção sugerida: '%s'", closest);
        suggestions.count = 1;
        free(closest);
    }
    
    // Context-specific suggestions
    switch (expected_context) {
        case TOKEN_BEGIN:
            if (suggestions.count < 5) {
                snprintf(suggestions.suggestions[suggestions.count].suggestion, 100, "BEGIN");
                suggestions.suggestions[suggestions.count].confidence = 0.7;
                snprintf(suggestions.suggestions[suggestions.count].explanation, 200, 
                        "Esperado 'BEGIN' para iniciar bloco");
                suggestions.count++;
            }
            break;
        case TOKEN_END:
            if (suggestions.count < 5) {
                snprintf(suggestions.suggestions[suggestions.count].suggestion, 100, "END");
                suggestions.suggestions[suggestions.count].confidence = 0.7;
                snprintf(suggestions.suggestions[suggestions.count].explanation, 200, 
                        "Esperado 'END' para fechar bloco");
                suggestions.count++;
            }
            break;
        default:
            break;
    }
    
    return suggestions;
}

SuggestionList suggest_punctuation_fix(TokenType found, TokenType expected) {
    SuggestionList suggestions = {0};
    
    if (found == TOKEN_EQUAL && expected == TOKEN_ASSIGN) {
        snprintf(suggestions.suggestions[0].suggestion, 100, ":=");
        suggestions.suggestions[0].confidence = 0.95;
        snprintf(suggestions.suggestions[0].explanation, 200, 
                "Use ':=' para atribuição (não '=')");
        suggestions.count = 1;
    } else if (found == TOKEN_SEMICOLON && expected == TOKEN_DOT) {
        snprintf(suggestions.suggestions[0].suggestion, 100, ".");
        suggestions.suggestions[0].confidence = 0.9;
        snprintf(suggestions.suggestions[0].explanation, 200, 
                "Use '.' para terminar o programa (não ';')");
        suggestions.count = 1;
    }
    
    return suggestions;
}

SuggestionList suggest_structure_fix(const char* context, const char* problem) {
    SuggestionList suggestions = {0};
    
    if (strstr(problem, "procedimento") && strstr(problem, "bloco")) {
        snprintf(suggestions.suggestions[0].suggestion, 100, "Adicionar BEGIN...END");
        suggestions.suggestions[0].confidence = 0.8;
        snprintf(suggestions.suggestions[0].explanation, 200, 
                "Procedimentos devem ter comandos dentro de BEGIN...END");
        suggestions.count = 1;
    }
    
    if (strstr(problem, "comando") && strstr(problem, "fora")) {
        snprintf(suggestions.suggestions[suggestions.count].suggestion, 100, "Mover para dentro de BEGIN");
        suggestions.suggestions[suggestions.count].confidence = 0.7;
        snprintf(suggestions.suggestions[suggestions.count].explanation, 200, 
                "Comandos devem estar dentro de blocos BEGIN...END");
        suggestions.count++;
    }
    
    return suggestions;
}

void print_smart_suggestions(const char* error_context, const char* found_token, TokenType expected) {
    printf("\n" BOLD MAGENTA SYMBOL_MAGIC " SUGESTÕES INTELIGENTES:" RESET_COLOR "\n");
    
    // Try different suggestion strategies
    SuggestionList id_suggestions = suggest_identifier_fix(found_token);
    SuggestionList kw_suggestions = suggest_keyword_fix(found_token, expected);
    SuggestionList punct_suggestions = suggest_punctuation_fix(TOKEN_EQUAL, expected);
    
    int suggestion_count = 0;
    
    // Print identifier suggestions
    for (int i = 0; i < id_suggestions.count && suggestion_count < 3; i++) {
        printf("  %d. " CYAN "%s" RESET_COLOR " " BRIGHT_BLACK "(%.0f%% confiança)" RESET_COLOR "\n", 
               suggestion_count + 1, 
               id_suggestions.suggestions[i].suggestion,
               id_suggestions.suggestions[i].confidence * 100);
        printf("     " DIM "%s" RESET_COLOR "\n", id_suggestions.suggestions[i].explanation);
        suggestion_count++;
    }
    
    // Print keyword suggestions
    for (int i = 0; i < kw_suggestions.count && suggestion_count < 3; i++) {
        printf("  %d. " CYAN "%s" RESET_COLOR " " BRIGHT_BLACK "(%.0f%% confiança)" RESET_COLOR "\n", 
               suggestion_count + 1, 
               kw_suggestions.suggestions[i].suggestion,
               kw_suggestions.suggestions[i].confidence * 100);
        printf("     " DIM "%s" RESET_COLOR "\n", kw_suggestions.suggestions[i].explanation);
        suggestion_count++;
    }
    
    // Print punctuation suggestions
    for (int i = 0; i < punct_suggestions.count && suggestion_count < 3; i++) {
        printf("  %d. " CYAN "%s" RESET_COLOR " " BRIGHT_BLACK "(%.0f%% confiança)" RESET_COLOR "\n", 
               suggestion_count + 1, 
               punct_suggestions.suggestions[i].suggestion,
               punct_suggestions.suggestions[i].confidence * 100);
        printf("     " DIM "%s" RESET_COLOR "\n", punct_suggestions.suggestions[i].explanation);
        suggestion_count++;
    }
    
    if (suggestion_count == 0) {
        printf("  " DIM "Nenhuma sugestão automática disponível." RESET_COLOR "\n");
        printf("  " CYAN "Dica:" RESET_COLOR " Verifique a documentação da linguagem PL/0\n");
    }
}
