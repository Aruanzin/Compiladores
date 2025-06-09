#ifndef LEXICO_H
#define LEXICO_H

#include "hash.h"
#include <ctype.h>

#ifdef DEBUG
 #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
 #define DBG_PRINT(...)
#endif

#define MAX_TOKENS 10000

typedef enum {
    TOKEN_CALL, TOKEN_VAR, TOKEN_BEGIN, TOKEN_END,
    TOKEN_WHILE, TOKEN_CONST, TOKEN_PROCEDURE, TOKEN_ELSE,
    TOKEN_THEN, TOKEN_IF, TOKEN_DO, TOKEN_FOR,
    TOKEN_SEMICOLON, TOKEN_COLON, TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_MULT, TOKEN_DIV, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_EQUAL, TOKEN_COMMA, TOKEN_GT, TOKEN_LT, TOKEN_DOT,
    TOKEN_LE, TOKEN_GE, TOKEN_NE, TOKEN_ASSIGN,
    TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_ERROR_LEXICO, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType tipo;
    char lexema[100];
    int linha;
    int status;
} Token;

typedef enum {
    LEX_ERROR_INVALID_CHAR,
    LEX_ERROR_UNDERSCORE,
    LEX_ERROR_DECIMAL_NUMBER,
    LEX_ERROR_STRING_LITERAL,
    LEX_ERROR_LOGICAL_OPERATOR,
    LEX_ERROR_UNTERMINATED_COMMENT,
    LEX_ERROR_IDENTIFIER_TOO_LONG,
    LEX_ERROR_NUMBER_TOO_LONG,
    LEX_ERROR_INVALID_NUMBER,
    LEX_ERROR_IDENTIFIER_INVALID_CHAR
} LexErrorType;

extern Token tokens[MAX_TOKENS];
extern int tokenCount;
extern const char *tokenTypeNames[];

// scanTermo classifies via provided automaton
void scanTermo(const char *linha, int *ptr, int num_linha,
               Token (*classify)(const char*,int,int,int));
void inicializarTabelaReservadas();
void liberarTabelaReservadas();
int isReservedWord(const char *palavra);
int isSymbol(const char *s);
int addToken(Token t);
Token automatoSymbol(const char *c, char next, int l);
Token get_next_token();
void init_lexer(const char* nome_arquivo);
// add scanner entrypoint
void lexico(const char* linha, int num_linha);
void report_lexical_error(const char* problematic_text, int line, int position, const char* full_line, LexErrorType error_type);
LexErrorType classify_lex_error(char c, const char* term, int error_flags);

// moved globals into lexico.c
extern FILE* fonte;
extern char linha[256];
extern int linha_num;
extern int pos;

// Global flag for hints (declared extern)
extern int show_hints;

// External declaration for error counter
extern int num_erros_lexicos;

// helper functions (no longer static)
int automatoComentario(const char* linha, int pointer, int num_linha);
int isSymbolChar(char c);
int tamanhoTermo(const char* linha, int pos, int *erro);
int tamanhoNumero(const char* linha, int pos, int *erro);
Token automatoIdentificador(const char* t, int err, int len, int l);
Token automatoNumero(const char* t, int err, int len, int l);

#endif // LEXICO_H