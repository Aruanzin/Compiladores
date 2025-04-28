#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef char *string;

typedef struct {
    string *keys;
    int    *values;   // store TokenType
    unsigned tamanho;
} TabelaHash;

void hash_criar(TabelaHash *t, unsigned tamanho);
void hash_destruir(TabelaHash *t);
int  hash_inserir(TabelaHash *t, string chave, int valor);
int  hash_buscar(TabelaHash *t, string chave);

#endif // HASH_H
