#ifndef SUGGESTIONS_H
#define SUGGESTIONS_H

#include "lexico.h"

typedef struct {
    char suggestion[100];
    double confidence;
    char explanation[200];
} Suggestion;

typedef struct {
    Suggestion suggestions[5];
    int count;
} SuggestionList;

// Fuzzy matching for identifier suggestions
SuggestionList suggest_identifier_fix(const char* invalid_identifier);

// Context-aware keyword suggestions
SuggestionList suggest_keyword_fix(const char* invalid_keyword, TokenType expected_context);

// Smart punctuation suggestions
SuggestionList suggest_punctuation_fix(TokenType found, TokenType expected);

// Structure suggestions based on context
SuggestionList suggest_structure_fix(const char* context, const char* problem);

// Advanced error correction using Levenshtein distance
int levenshtein_distance(const char* s1, const char* s2);
char* find_closest_keyword(const char* input);

// AI-like suggestion system
void print_smart_suggestions(const char* error_context, const char* found_token, TokenType expected);

#endif // SUGGESTIONS_H
