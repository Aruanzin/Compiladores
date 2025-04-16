#ifndef ANALISADOR_LEXICO_H
#define ANALISADOR_LEXICO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 1000
#define TRUE 1
#define FALSE 0

typedef struct {
    char lex;
    char lexema[100];
    char token[100];
    int linha;
    int status;
} Token;

typedef struct {
    const char* lexema;
    const char* token;
} TabelaReservados;

extern TabelaReservados tabelaReservados[];
extern const char* simbolos[];
extern Token tokens[MAX_TOKENS];
extern int tokenCount;

void lexico(const char* linha, int num_linha);
int addToken(Token result);
int automatoSymbol(char* caracter, char next_caracter, int linha);

#endif