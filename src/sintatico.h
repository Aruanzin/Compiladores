#include "lexico.h"
#ifndef PARSER_H
#define PARSER_H

// Forward declarations for synchronization token sets
extern TokenType SYNC_STATEMENT[];
extern TokenType SYNC_DECLARATION[];
extern TokenType SYNC_BLOCK[];
extern TokenType SYNC_EXPRESSION[];
extern TokenType SYNC_PROCEDURE[];
extern TokenType SYNC_COMMAND_END[];
extern int SYNC_STATEMENT_SIZE;
extern int SYNC_DECLARATION_SIZE;
extern int SYNC_BLOCK_SIZE;
extern int SYNC_EXPRESSION_SIZE;
extern int SYNC_PROCEDURE_SIZE;
extern int SYNC_COMMAND_END_SIZE;

typedef enum {
    CONTEXT_PROGRAMA,
    CONTEXT_BLOCO,
    CONTEXT_DECLARACAO_CONST,
    CONTEXT_DECLARACAO_VAR,
    CONTEXT_DECLARACAO_PROC,
    CONTEXT_COMANDO,
    CONTEXT_COMANDO_COMPOSTO,
    CONTEXT_COMANDO_IF,
    CONTEXT_COMANDO_WHILE,
    CONTEXT_COMANDO_FOR,
    CONTEXT_CONDICAO,
    CONTEXT_EXPRESSAO,
    CONTEXT_TERMO,
    CONTEXT_FATOR
} ParseContext;

typedef struct ContextFrame {
    ParseContext context;
    TokenType* followers;
    int followers_count;
    struct ContextFrame* parent;
} ContextFrame;

// External declarations for global variables
extern ContextFrame* current_context;
extern Token current_token;
extern int num_erros_sintaticos;
extern char linha_atual[256];
extern int pos_atual;

// External declarations for FOLLOW sets
extern TokenType FOLLOW_PROGRAMA[];
extern TokenType FOLLOW_BLOCO[];
extern TokenType FOLLOW_DECLARACAO_CONST[];
extern TokenType FOLLOW_DECLARACAO_VAR[];
extern TokenType FOLLOW_DECLARACAO_PROC[];
extern TokenType FOLLOW_COMANDO[];
extern TokenType FOLLOW_COMANDO_COMPOSTO[];
extern TokenType FOLLOW_COMANDO_IF[];
extern TokenType FOLLOW_COMANDO_WHILE[];
extern TokenType FOLLOW_COMANDO_FOR[];
extern TokenType FOLLOW_CONDICAO[];
extern TokenType FOLLOW_EXPRESSAO[];
extern TokenType FOLLOW_TERMO[];
extern TokenType FOLLOW_FATOR[];
extern TokenType EXTRA_SYNC[];
extern int FOLLOW_SIZES[];
extern int EXTRA_SYNC_SIZE;

// Function declarations
void parse();
void programa();
int bloco();
int comando();
void condicao();
void expressao();
void termo();
void fator();

// Enhanced parsing functions for better error handling
void programa_enhanced();
int bloco_enhanced();
int comando_enhanced();
int detect_missing_var_declaration();
int detect_multiple_begin_blocks();

int sincronizar(TokenType sincronizadores[], int n);
int eat(TokenType esperado, TokenType sincronizadores[], int n);
int eat_advanced(TokenType esperado, ParseContext context);
const char* sugerir_correcao(TokenType esperado, TokenType encontrado);
void syntax_error_with_hint(const char* msg, const char* dica);
void syntax_error(const char* msg);
void lexico_error(const char* msg);

// Context management functions
void push_context(ParseContext context);
void pop_context();
TokenType* get_follow_set(ParseContext context, int* size);
void mostrar_erro_visual(const char* msg, const char* dica);

#endif // PARSER_H
