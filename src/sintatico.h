#ifndef PARSER_H
#define PARSER_H

// Define common synchronization token sets
extern TokenType SYNC_STATEMENT[];
extern TokenType SYNC_DECLARATION[];
extern TokenType SYNC_BLOCK[];
extern TokenType SYNC_EXPRESSION[];
extern TokenType SYNC_PROCEDURE[];  // Adicionar sincronização para procedimentos
extern int SYNC_STATEMENT_SIZE;
extern int SYNC_DECLARATION_SIZE;
extern int SYNC_BLOCK_SIZE;
extern int SYNC_EXPRESSION_SIZE;
extern int SYNC_PROCEDURE_SIZE;

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

// Global flag for hints (declared extern)
extern int show_hints;

void parse();
void programa();
int bloco();
int comando();
void condicao();
void expressao();
void termo();
void fator();
int sincronizar(TokenType sincronizadores[], int n);
int eat(TokenType esperado, TokenType sincronizadores[], int n);
int eat_advanced(TokenType esperado, ParseContext context);
const char* sugerir_correcao(TokenType esperado, TokenType encontrado);
void syntax_error_with_hint(const char* msg, const char* dica);
void syntax_error(const char* msg);
void lexico_error(const char* msg);
#endif // PARSER_H
