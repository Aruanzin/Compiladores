#ifndef STATISTICS_H
#define STATISTICS_H

#include <time.h>
#include "lexico.h"

typedef struct {
    int total_lines;
    int total_tokens;
    int total_identifiers;
    int total_numbers;
    int total_keywords;
    int total_operators;
    int total_comments;
    int lexical_errors;
    int syntax_errors;
    int recoveries_successful;
    int recoveries_failed;
    double parsing_time;
    clock_t start_time;
    clock_t end_time;
    
    // Complexity metrics
    int max_nesting_depth;
    int current_nesting_depth;
    int total_procedures;
    int total_variables;
    int total_constants;
    int total_calls;
    
    // Error distribution
    int errors_by_line[1000]; // Support up to 1000 lines
    int max_line;
    
    // Most common errors
    char common_errors[10][100];
    int error_counts[10];
    int unique_errors;
} CompilerStats;

extern CompilerStats stats;

void init_statistics();
void update_token_stats(TokenType type);
void update_error_stats(int line, const char* error_msg);
void start_parsing_timer();
void end_parsing_timer();
void print_detailed_statistics();
void print_performance_metrics();
void print_code_quality_metrics();

#endif // STATISTICS_H
