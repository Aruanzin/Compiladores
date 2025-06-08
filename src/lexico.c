#include "lexico.h"
#include <string.h>

// lexer state (moved from header)
FILE* fonte = NULL;
char linha[256];
int linha_num = 0;
int pos = 0;

// token storage
Token tokens[MAX_TOKENS];
int tokenCount = 0;

// Define global variables
typedef struct { const char *lex; TokenType tipo;} Reservado;
static Reservado tabelaReservados[] = {
    {"CALL",TOKEN_CALL}, {"VAR",TOKEN_VAR}, {"BEGIN",TOKEN_BEGIN}, {"END",TOKEN_END}, {"WHILE",TOKEN_WHILE}, {"CONST",TOKEN_CONST}, {"PROCEDURE",TOKEN_PROCEDURE}, {"ELSE",TOKEN_ELSE}, {"THEN",TOKEN_THEN}, {"IF",TOKEN_IF}, {"DO",TOKEN_DO}, {"FOR",TOKEN_FOR}, {NULL,0}
};

// restore comma to symbol list for delimiter purposes
static const char *simbolos[] = {
    ";", ":", "+", "-", "*", "/", "(", ")", "=", ",", ">", "<", "."
};
static TokenType simbolosType[] = {
    TOKEN_SEMICOLON, TOKEN_COLON, TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_MULT,      TOKEN_DIV,   TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_EQUAL,     TOKEN_COMMA, TOKEN_GT,    TOKEN_LT,    TOKEN_DOT
};

TabelaHash tabelaPalavrasReservadas;  // Hash table for reserved words
TabelaHash tabelaSimbolos;            // Hash table for symbols

// Initialize hash table for reserved words
void inicializarTabelaReservadas() {
    // Count the number of reserved words
    int numPalavrasReservadas = 0;
    while (tabelaReservados[numPalavrasReservadas].lex != NULL) {
        numPalavrasReservadas++;
    }
    
    // Create a hash table with size about 2x the number of words (using prime number)
    int tamanhoTabela = numPalavrasReservadas * 2 + 1;
    hash_criar(&tabelaPalavrasReservadas, tamanhoTabela);
    
    // Insert all reserved words
    for (int i = 0; i < numPalavrasReservadas; i++) {
        hash_inserir(&tabelaPalavrasReservadas, (string)tabelaReservados[i].lex, tabelaReservados[i].tipo);
    }
    
    DBG_PRINT("RES hash init %d\n", numPalavrasReservadas);

    // Inicializar hash de símbolos
    {
        int numSimbolos = sizeof(simbolos) / sizeof(simbolos[0]);
        int tamanhoSimbolos = numSimbolos * 2 + 1;
        hash_criar(&tabelaSimbolos, tamanhoSimbolos);
        for (int i = 0; i < numSimbolos; i++) {
            hash_inserir(&tabelaSimbolos, (string)simbolos[i], simbolosType[i]);
        }
        DBG_PRINT("SYM hash init %d\n", numSimbolos);
    }
}

// Free hash table resources
void liberarTabelaReservadas() {
    hash_destruir(&tabelaPalavrasReservadas);
    DBG_PRINT("Tabela hash de palavras reservadas liberada\n");

    // Liberar tabela de símbolos
    hash_destruir(&tabelaSimbolos);
    DBG_PRINT("Tabela hash de símbolos liberada\n");
}

// signature fix to match header
int isReservedWord(const char *palavra) {
    return hash_buscar(&tabelaPalavrasReservadas, (string)palavra) >= 0;
}

// signature fix; drop TRUE/FALSE
int isSymbol(const char *s) {
    return hash_buscar(&tabelaSimbolos, (string)s) >= 0;
}

int isSymbolChar(char c) {
    char tmp[2] = {c,'\0'};
    return isSymbol(tmp);
}

int isDelimiter(char c){
    return isSymbolChar(c) || isspace((unsigned char)c);
}

//adiciona os tokens que coletamos a tabela com sua tipagem e tudo mais
int addToken(Token result) {
    if (tokenCount >= MAX_TOKENS) {
        printf("Erro: número máximo de tokens atingido.\n");
        return -1;
    }
    DBG_PRINT("Adicionando token: lexema='%s', tipo=%d, linha=%d\n",
              result.lexema, result.tipo, result.linha);
    tokens[tokenCount] = result;   // struct copy
    tokenCount++;
    return 0;
}

//identifica operador
Token automatoSymbol(const char *c, char next, int l){
    // handle two‑char symbols first
    if (c[0] == ':' && next == '=') {
        Token r = { .tipo = TOKEN_ASSIGN, .linha = l, .status = 0 };
        strcpy(r.lexema, ":=");
        return r;
    }
    if (c[0] == '<' && next == '=') {
        Token r = { .tipo = TOKEN_LE, .linha = l, .status = 0 };
        strcpy(r.lexema, "<=");
        return r;
    }
    if (c[0] == '>' && next == '=') {
        Token r = { .tipo = TOKEN_GE, .linha = l, .status = 0 };
        strcpy(r.lexema, ">=");
        return r;
    }
    if (c[0] == '<' && next == '>') {
        Token r = { .tipo = TOKEN_NE, .linha = l, .status = 0 };
        strcpy(r.lexema, "<>");
        return r;
    }

    Token r = {0}; r.linha = l; r.status = 0;
    char s2[3] = { c[0], next? next : '\0', '\0' };
    int ht = hash_buscar(&tabelaSimbolos, (string)s2);
    if (ht < 0) {
        s2[1] = '\0';
        ht = hash_buscar(&tabelaSimbolos, (string)s2);
    }
    r.tipo = (ht >= 0) ? (TokenType)ht : TOKEN_ERROR_LEXICO;
    strcpy(r.lexema, s2);

    // int tokelen = s2[1] ? 2 : 1;
    // skip comma tokens
    if (r.tipo != TOKEN_COMMA) {
        return r;
    }
    return r;
}

// alterar tamanhoTermo para reportar erro de caracter inválido (underscore now invalid)
int tamanhoTermo(const char* linha, int pos, int* erro) {
    int len = 0;
    *erro = 0;
    while (linha[pos+len] != '\0' && !isDelimiter(linha[pos+len])) {
        char c = linha[pos+len];
        if (!isalnum(c) && !isSymbolChar(c)) {
            *erro = 1;
        }
        len++;
    }
    return len;
}

// new: scan full numeric term (allow '.' but flag as error)
int tamanhoNumero(const char *linha, int pos, int *erro) {
    int len = 0;
    *erro = 0;
    while (linha[pos+len] != '\0' && !isspace((unsigned char)linha[pos+len])
           && !isSymbolChar(linha[pos+len])) {
        char c = linha[pos+len];
        if (!isdigit(c)) {
            *erro = 1;  // flag float or invalid char
        }
        len++;
    }
    return len;
}

// updated comment automaton: returns number of chars consumed
int automatoComentario(const char* linha_buf, int pointer, int num_linha) {
    int start = pointer;
    pointer++;  // skip '{'
    while (linha_buf[pointer] != '\0' && linha_buf[pointer] != '}') {
        pointer++;
    }
    int consumed = pointer - start + (linha_buf[pointer] == '}' ? 1 : 0);
    if (linha_buf[pointer] == '}') {
        return consumed;
    }
    // unterminated: emit error token
    int raw_len = pointer - start;
    int copy_len = raw_len > 99 ? 99 : raw_len;
    if (copy_len > 0 && linha_buf[start + copy_len] == '\n')
        copy_len--;
    Token e = { .tipo = TOKEN_ERROR_LEXICO, .linha = num_linha, .status = 1 };
    strncpy(e.lexema, linha_buf + start, copy_len);
    e.lexema[copy_len] = '\0';
    addToken(e);
    return consumed;
}

// generic scan
void scanTermo(const char *linha, int *ptr, int num_linha,
               Token (*classify)(const char*,int,int,int)) {
    int err, len = tamanhoTermo(linha,*ptr,&err);
    char termo[100];
    int sz = len<99?len:99;
    strncpy(termo, linha+*ptr, sz); termo[sz]=0;
    classify(termo, err, len, num_linha);
    *ptr += len;
}

// identifier vs reserved
Token automatoIdentificador(const char* t,int err,int len,int l){
    // error if invalid char or too long
    if (err || len > 99) {
        Token e = { .tipo = TOKEN_ERROR_LEXICO, .linha = l, .status = 0 };
        if (err)      e.status |= 1; 
        if (len > 99) e.status |= 2;
        strncpy(e.lexema, t, 99); e.lexema[99]=0;
        return e;
    }
    // valid identifier or reserved
    Token r = { .linha = l, .status = 0 };
    int ht = hash_buscar(&tabelaPalavrasReservadas, (string)t);
    r.tipo = (ht >= 0) ? (TokenType)ht : TOKEN_IDENTIFIER;
    strcpy(r.lexema, t);
    return r;
}

Token automatoNumero(const char* t,int err,int len,int l){
    // error if invalid char or too long
    if (err || len > 99) {
        Token e = { .tipo = TOKEN_ERROR_LEXICO, .linha = l, .status = 0 };
        if (err)      e.status |= 1;
        if (len > 99) e.status |= 2;
        strncpy(e.lexema, t, 99); e.lexema[99]=0;
        return e;
    }
    // valid number
    for(int i = 0; i < len; i++) {
        if (!isdigit(t[i])) {
            Token e = { .tipo = TOKEN_ERROR_LEXICO, .linha = l, .status = 0 };
            e.status |= 1;
            strncpy(e.lexema, t, 99); e.lexema[99]=0;
            return e;
        }
    }


    Token r = { .tipo = TOKEN_NUMBER, .linha = l, .status = 0 };
    strcpy(r.lexema, t);
    return r;
}

//fazer aqui o começo da analise para saber qual automato acionar
void lexico(const char* linha, int num_linha){
    // Ensure hash table is initialized on first call
    static int initialized = 0;
    if (!initialized) {
        inicializarTabelaReservadas();
        initialized = 1;
    }
    
    int pointer = 0;
    char caracter[2];

    DBG_PRINT("\nAnalisando linha %d: '%s'\n", num_linha, linha);

    while (linha[pointer] != '\0') {
        caracter[0] = linha[pointer];
        caracter[1] = '\0';

        DBG_PRINT("Caractere atual: '%c' (posição %d)\n", caracter[0], pointer);

        if (isspace(caracter[0])) {
            DBG_PRINT("Ignorando espaço\n");
            pointer++;
            continue;
        }

        // Tratar comentário {...}
        if (caracter[0] == '{') {
            DBG_PRINT("Caractere inicia comentário\n");
            int avanco = automatoComentario(linha, pointer, num_linha);
            pointer += avanco;
            DBG_PRINT("Avançando %d posições (comentário)\n", avanco);
            continue;
        }

        if (isSymbol(caracter)) {
            DBG_PRINT("Caractere é um símbolo\n");
            char next_char = '\0';
            if (linha[pointer + 1] != '\0') {
                next_char = linha[pointer + 1];
            }
            Token tok = automatoSymbol(caracter, next_char, num_linha);
            addToken(tok);
            int avanco = strlen(tok.lexema);
            pointer += avanco;
            DBG_PRINT("Avançando %d posições\n", avanco);
            continue;
        }

        if (isalpha(caracter[0])) {
            DBG_PRINT("Caractere é uma letra\n");
            int err, len = tamanhoTermo(linha, pointer, &err);
            char termo[100];
            int sz = len < 99 ? len : 99;
            strncpy(termo, linha + pointer, sz);
            termo[sz] = '\0';
            Token tok = automatoIdentificador(termo, err, len, num_linha);
            addToken(tok);
            pointer += len;
            continue;
        }

        // 4) idem para números
        if (isdigit(caracter[0])) {
            DBG_PRINT("Caractere é um dígito\n");
            // scan integer (dots cause err)
            int err_num, len_num = tamanhoNumero(linha, pointer, &err_num);
            char termo[100];
            int sz = len_num < 99 ? len_num : 99;
            strncpy(termo, linha + pointer, sz);
            termo[sz] = '\0';
            Token tok = automatoNumero(termo, err_num, len_num, num_linha);
            addToken(tok);
            pointer += len_num;
            continue;
        }

        // 5) caso não reconhecido - melhorar relatório de erro
        DBG_PRINT("Caractere não reconhecido: '%c'\n", caracter[0]);
        Token error = {0}; // Initialize struct
        error.linha = num_linha;
        error.status = 1; // Indicate error status
        error.tipo = TOKEN_ERROR_LEXICO; // Set the error type
        strcpy(error.lexema, caracter); // Store the problematic character
        
        // Adicionar informação contextual para o erro léxico apenas se hints estiverem ativadas
        if (show_hints) {
            printf("\033[1;31m🚨 Erro léxico\033[0m na linha %d:\n", num_linha);
            printf("📄 Código: %s", linha);
            if (linha[strlen(linha)-1] != '\n') {
                printf("\n");
            }
            
            // Mostrar ponteiro visual
            printf("   ");
            for (int i = 0; i < pointer; i++) {
                printf(" ");
            }
            printf("\033[1;31m^\033[0m\n");
            
            // Dar dica baseada no caractere
            if (caracter[0] == '_') {
                printf("💡 \033[1;33mDica:\033[0m Underscores não são permitidos em identificadores\n");
            } else if (caracter[0] == '.') {
                printf("💡 \033[1;33mDica:\033[0m Números decimais não são suportados (use apenas inteiros)\n");
            } else if (caracter[0] == '"' || caracter[0] == '\'') {
                printf("💡 \033[1;33mDica:\033[0m Strings não são suportadas nesta linguagem\n");
            } else if (caracter[0] == '&' || caracter[0] == '|') {
                printf("💡 \033[1;33mDica:\033[0m Operadores lógicos não são suportados\n");
            } else {
                printf("💡 \033[1;33mDica:\033[0m Caractere '%c' não é válido nesta linguagem\n", caracter[0]);
            }
            
            printf("🔍 Caractere problemático: '\033[1;36m%c\033[0m' (código ASCII: %d)\n\n", 
                   caracter[0], (int)caracter[0]);
        } else {
            printf("Erro léxico, linha %d\n", num_linha);
        }
        
        addToken(error);
        pointer++; // Evitar loop infinito
    }
    DBG_PRINT("Fim da linha %d\n", num_linha);
}

// Define tokenTypeNames array
const char *tokenTypeNames[] = {
    /* reserved words */
    "CALL","VAR","BEGIN","END","WHILE","CONST","PROCEDURE","ELSE",
    "THEN","IF","DO","FOR",
    /* symbols */
    "simbolo_ponto_virgula","simbolo_dois_pontos","simbolo_mais","simbolo_menos",
    "simbolo_multiplicacao","simbolo_divisao","simbolo_abre_parenteses","simbolo_fecha_parenteses",
    "simbolo_igual","simbolo_virgula","simbolo_maior","simbolo_menor","simbolo_ponto",
    "simbolo_menor_igual","simbolo_maior_igual","simbolo_diferente","simbolo_atribuicao",
    /* generic */
    "ident","numero","<ERRO_LEXICO>","<EOF>"
};