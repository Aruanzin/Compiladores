#include "hash.h"
#include <time.h>

#define MAX_STRING_LEN 20

// Função para converter uma string em um valor numérico usando codificação ASCII
unsigned converter_string_para_hash(string s)
{
    unsigned h = 0;
    for (int i = 0; s[i] != '\0'; i++)
        h = h * 256 + s[i];
    return h;
}

// Função de hash por multiplicação (método padrão)
static unsigned hash_funcao(unsigned x, unsigned i, unsigned B)
{
    const double A = 0.6180; // Constante para o método de multiplicação
    return ((int)((fmod(x * A, 1) * B) + i)) % B;
}

// Função para criar a tabela hash
void hash_criar(TabelaHash *t, unsigned tamanho)
{
    t->keys   = malloc(sizeof(string) * tamanho);
    t->values = malloc(sizeof(int)    * tamanho);
    t->tamanho = tamanho;
    for (int i = 0; i < tamanho; i++) {
        t->keys[i] = NULL;
        t->values[i] = -1;
    }
}

// Função para destruir a tabela hash, liberando a memória alocada
void hash_destruir(TabelaHash *t)
{
    for (int i = 0; i < t->tamanho; i++)
        if (t->keys[i]) free(t->keys[i]);
    free(t->keys);
    free(t->values);
    t->tamanho = 0;
}

// Função para inserir um elemento na tabela hash
int hash_inserir(TabelaHash *t, string chave, int valor)
{
    unsigned x = converter_string_para_hash(chave), B = t->tamanho;
    for (unsigned i = 0; i < B; i++) {
        int pos = hash_funcao(x, i, B);
        if (!t->keys[pos]) {
            t->keys[pos]   = strdup(chave);
            t->values[pos] = valor;
            return pos;
        }
        if (strcmp(t->keys[pos], chave) == 0)
            return pos;
    }
    return -1;
}

// Função para buscar um elemento na tabela hash
int hash_buscar(TabelaHash *t, string chave)
{
    unsigned x = converter_string_para_hash(chave), B = t->tamanho;
    for (unsigned i = 0; i < B; i++) {
        int pos = hash_funcao(x, i, B);
        if (!t->keys[pos]) break;
        if (strcmp(t->keys[pos], chave) == 0)
            return t->values[pos];
    }
    return -1;
}

// Função para inicializar a tabela com palavras reservadas
void hash_inicializar_palavras_reservadas(TabelaHash *t, string *palavras, unsigned num_palavras)
{
    for (unsigned i = 0; i < num_palavras; i++)
    {
        hash_inserir(t, palavras[i], i);
    }
}
