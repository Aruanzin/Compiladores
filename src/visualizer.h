#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "lexico.h"

// Code visualization functions
void print_syntax_highlighted_line(const char* line, int line_number, int error_column);
void print_code_structure_tree();
void print_error_context(const char* line, int error_pos, int line_number);
void print_banner();
void print_summary_box(int errors, int warnings, double time);

// Advanced formatting
void print_underline_error(const char* line, int start_pos, int end_pos);
void highlight_token_in_line(const char* line, const char* token, int position);

// Interactive features
void pause_for_user();
int ask_user_choice(const char* question, const char* options[], int option_count);

#endif // VISUALIZER_H
