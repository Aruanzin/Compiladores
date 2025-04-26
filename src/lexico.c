#include "lexico.h"

// Define global variables
TabelaReservados tabelaReservados[] = {
    {"CALL","simb_CALL"},
    {"VAR", "VAR"},
    {"BEGIN", "BEGIN"},
    {"END", "END"},
    {"WHILE", "WHILE"},
    {"CONST", "CONST"},
    {"PROCEDURE", "PROCEDURE"},
    {"ELSE", "ELSE"},
    {"THEN", "THEN"},
    {"IF", "IF"},
    {"DO", "DO"},
    {"FOR", "FOR"},
    {NULL, NULL}
};

const char* simbolos[] = {
    ";", ":", "+", "-", "*", "/", "(", ")", "=", ",", ">", "<", "."
};

Token tokens[MAX_TOKENS];
int tokenCount = 0;

int isDelimiter(){
    return FALSE;
}

int isReservedWord(){
    return FALSE;
}

int isSymbol(char* caracter){
    for (int i = 0; i < sizeof(simbolos)/sizeof(simbolos[0]); i++) {
        if (strcmp(caracter, simbolos[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

//adiciona os tokens que coletamos a tabela com sua tipagem e tudo mais
int addToken(Token result){
    if (tokenCount >= MAX_TOKENS) {
        printf("Erro: número máximo de tokens atingido.\n");
        return -1;
    }
    printf("DEBUG: Adicionando token: lexema='%s', token='%s', linha=%d\n", 
           result.lexema, result.token, result.linha);
    tokens[tokenCount].lex = result.lex;
    strcpy(tokens[tokenCount].lexema, result.lexema);
    strcpy(tokens[tokenCount].token, result.token);
    tokens[tokenCount].linha = result.linha;
    tokens[tokenCount].status = result.status;
    tokenCount++;
    return 0;
}

//identifica operador
int automatoSymbol(char* caracter, char next_caracter, int linha){
    Token result;
    result.lex = caracter[0];
    result.linha = linha;
    result.status = 0;
    int tokensRead = 1;
    
    switch (caracter[0]) {
        case '<':
            if (next_caracter == '=') {
                strcpy(result.token, "simbolo_menor_igual");
                strcpy(result.lexema, "<=");
                tokensRead = 2;
            } else if (next_caracter == '>') {
                strcpy(result.token, "simbolo_diferente");
                strcpy(result.lexema, "<>");
                tokensRead = 2;
            } else {
                strcpy(result.token, "simbolo_menor");
                strcpy(result.lexema, "<");
            }
            break;
        case '>':
            if (next_caracter == '=') {
                strcpy(result.token, "simbolo_maior_igual");
                strcpy(result.lexema, ">=");
                tokensRead = 2;
            } else {
                strcpy(result.token, "simbolo_maior");
                strcpy(result.lexema, ">");
            }
            break;
        case '+': 
            strcpy(result.token, "simbolo_mais"); 
            strcpy(result.lexema, "+"); 
            break;
        case '-': 
            strcpy(result.token, "simbolo_menos"); 
            strcpy(result.lexema, "-"); 
            break;
        case '*': 
            strcpy(result.token, "simbolo_multiplicacao"); 
            strcpy(result.lexema, "*"); 
            break;
        case '/': 
            strcpy(result.token, "simbolo_divisao"); 
            strcpy(result.lexema, "/"); 
            break;
        case '=': 
            strcpy(result.token, "simbolo_igual"); 
            strcpy(result.lexema, "="); 
            break;
        case ':':
            if (next_caracter == '=') {
                strcpy(result.token, "simbolo_atribuicao");
                strcpy(result.lexema, ":=");
                tokensRead = 2;
            } else {
                strcpy(result.token, "simbolo_dois_pontos");
                strcpy(result.lexema, ":");
            }
            break;
        case ',':
            strcpy(result.token, "simbolo_virgula");
            strcpy(result.lexema, ",");
            break;
        case ';':
            strcpy(result.token, "simbolo_ponto_virgula");
            strcpy(result.lexema, ";");
            break;
        case '(':
            strcpy(result.token, "simbolo_abre_parenteses");
            strcpy(result.lexema, "(");
            break;
        case ')':
            strcpy(result.token, "simbolo_fecha_parenteses");
            strcpy(result.lexema, ")");
            break;
        case '.':
            strcpy(result.token, "simbolo_ponto");
            strcpy(result.lexema, ".");
            break;
        default:
            strcpy(result.token, "<ERRO_LEXICO>");
            strcpy(result.lexema, caracter);
    }
    
    addToken(result);
    return tokensRead;
}

//identifica identificador ou palavras reservadas
int automatoIdentificador(char* linha, int num_linha, int pointer){
    printf("DEBUG: Implementação básica de identificador adicionada\n");
    Token result;
    result.lex = 'i';  // 'i' para identificador
    result.linha = num_linha;  // Será atualizado em uma implementação completa
    result.status = 0;
    char cadeia[100];
    int auxIndex = 0;
    
    while(isDelimiter(linha[pointer])){
        cadeia[auxIndex++] = linha[pointer++];
    }

    
    int i = 0;
    int key = 0;
    while(tabelaReservados[i].lexema != NULL){
        if(strcmp(tabelaReservados[i].lexema, cadeia) == 0){
            strcpy(result.lexema, tabelaReservados[i].lexema);
            strcpy(result.token, tabelaReservados[i].token);
            key = 1;
            break;
        }
        i++;
    }

    if(key == 0){
        strcpy(result.lexema, cadeia);
        strcpy(result.lexema, "ident");
    }
    
    // Por enquanto, simplesmente retorna um identificador genérico
    
    addToken(result);
    return auxIndex;
}

//identifica boa formação de numeros inteiros ou reais
int automatoNumero(){
    printf("DEBUG: Implementação básica de número adicionada\n");
    Token result;
    result.lex = 'n';  // 'n' para número
    result.linha = 0;  // Será atualizado em uma implementação completa
    result.status = 0;
    
    // Por enquanto, simplesmente retorna um número genérico
    strcpy(result.token, "numero");
    strcpy(result.lexema, "2");  // Placeholder
    
    addToken(result);
    return 1;
}

int automatoComentario(){
    // Implementar automato para comentarios
    return 0;

}

//fazer aqui o começo da analise para saber qual automato acionar
void lexico(const char* linha, int num_linha){
    int pointer = 0;
    char caracter[2];
    // char aux[50];
    // int auxIndex;
    // int counterWord = 0;

    printf("\nDEBUG: Analisando linha %d: '%s'\n", num_linha, linha);

    while (linha[pointer] != '\0') {
        caracter[0] = linha[pointer];
        caracter[1] = '\0';

        printf("DEBUG: Caractere atual: '%c' (posição %d)\n", caracter[0], pointer);

        if (isspace(caracter[0])) {
            printf("DEBUG: Ignorando espaço\n");
            pointer++;
            continue;
        }

        if (isalpha(caracter[0])) {
            printf("DEBUG: Caractere é uma letra\n");

            //adiciona cada caractere dentro da cadeia que possa ter no identificador
            // while(isblank(caracter[0])){
            //     if(isSymbol(caracter[0]))
            //         break;
            //     aux[counterWord++] = caracter[0];
            //     caracter[0] = linha[pointer + counterWord];
            // }
            // counterWord = 0;
            int avanco = automatoIdentificador(linha, num_linha, pointer);
            printf("DEBUG: Avançando %d posições\n", avanco);
            pointer += avanco;
        } else if (isdigit(caracter[0])) {
            printf("DEBUG: Caractere é um dígito\n");
            int avanco = automatoNumero();
            printf("DEBUG: Avançando %d posições\n", avanco);
            pointer += avanco;
        } else if (isSymbol(caracter)) {
            printf("DEBUG: Caractere é um símbolo\n");
            char next_char = '\0';
            if (linha[pointer + 1] != '\0') {
                next_char = linha[pointer + 1];
            }
            int avanco = automatoSymbol(caracter, next_char, num_linha);
            printf("DEBUG: Avançando %d posições\n", avanco);
            pointer += avanco;
        } else {
            printf("DEBUG: Caractere não reconhecido: '%c'\n", caracter[0]);
            Token error;
            error.lex = caracter[0];
            error.linha = num_linha;
            error.status = 0;
            strcpy(error.lexema, caracter);
            strcpy(error.token, "<ERRO_LEXICO>");
            addToken(error);
            pointer++; // Evitar loop infinito
        }
    }
    printf("DEBUG: Fim da linha %d\n", num_linha);
}