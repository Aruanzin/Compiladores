#ifndef LEXICO_H
#define LEXICO_H

#include "hash.h"
#include <ctype.h>

#ifdef DEBUG
 #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
 #define DBG_PRINT(...)
#endif

#define MAX_TOKENS 10000          // ← add limit

typedef enum {
    TOKEN_CALL, TOKEN_VAR, TOKEN_BEGIN, TOKEN_END,
    TOKEN_WHILE, TOKEN_CONST, TOKEN_PROCEDURE, TOKEN_ELSE,
    TOKEN_THEN, TOKEN_IF, TOKEN_DO, TOKEN_FOR,
    TOKEN_SEMICOLON, TOKEN_COLON, TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_MULT, TOKEN_DIV, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_EQUAL, TOKEN_COMMA, TOKEN_GT, TOKEN_LT, TOKEN_DOT,
    TOKEN_LE, TOKEN_GE, TOKEN_NE, TOKEN_ASSIGN,
    TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_ERROR_LEXICO
} TokenType;

typedef struct {
    TokenType tipo;
    char lexema[100];
    int linha;
    int status;
} Token;

extern Token tokens[MAX_TOKENS];
extern int tokenCount;

void scanTermo(const char *linha, int *ptr, int num_linha,
               void (*classify)(const char*,int,int,int));
void inicializarTabelaReservadas();
void liberarTabelaReservadas();
int isReservedWord(const char *palavra);
int isSymbol(const char *s);
int addToken(Token t);
int automatoSymbol(const char *c, char next, int l);
void lexico(const char *linha, int num_linha);

#endif // LEXICO_H