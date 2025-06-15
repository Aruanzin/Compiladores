#include "lexico.h"
#ifndef PARSER_H
#define PARSER_H

#define MAX_CONTEXT_DEPTH 100
#define SYNC_PROGRAM_TOKENS {TOKEN_PROGRAM, TOKEN_BEGIN, TOKEN_END, TOKEN_DOT}
#define SYNC_BLOCK_TOKENS {TOKEN_BEGIN, TOKEN_END, TOKEN_VAR, TOKEN_CONST, TOKEN_PROCEDURE}
#define SYNC_STATEMENT_TOKENS {TOKEN_SEMICOLON, TOKEN_END, TOKEN_ELSE, TOKEN_UNTIL}
#define SYNC_EXPRESSION_TOKENS {TOKEN_SEMICOLON, TOKEN_THEN, TOKEN_DO, TOKEN_RPAREN}

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
    TokenType* sync_tokens;      // Tokens de sincronização específicos
    int sync_count;
    int start_line;              // Linha onde o contexto começou
    int start_column;            // Coluna onde o contexto começou
    char* context_name;          // Nome legível do contexto
    struct ContextFrame* parent;
} ContextFrame;


typedef struct ErrorRecoveryStack {
    ContextFrame* contexts[MAX_CONTEXT_DEPTH];
    int top;
    int max_depth;
} ErrorRecoveryStack;

extern ErrorRecoveryStack* error_stack;

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

int eat_advanced(TokenType esperado, ParseContext context);
const char* sugerir_correcao(TokenType esperado, TokenType encontrado);
void syntax_error_with_hint(const char* msg, const char* dica);
void syntax_error(const char* msg);
void lexico_error(const char* msg);

// Context management functions
TokenType* get_follow_set(ParseContext context, int* size);
void mostrar_erro_visual(const char* msg, const char* dica);
int try_sync_with_tokens(TokenType* tokens, int count);
void init_error_recovery_stack();
void push_context_enhanced(ParseContext context, const char* name, 
                           TokenType* sync_tokens, int sync_count);
void pop_context_enhanced();
void suggest_corrections_for_context(ContextFrame* frame);
int panic_mode_recovery();
int advanced_synchronize();

#endif // PARSER_H
