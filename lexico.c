#include "lexico.h"

tabelaReservados[] = {
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

operadores[] = {
    ";", ":", "+", "-", "*", "/", "(", ")", "=", ",", ">", "<", "."
};

void isDelimiter(){

}
//adiciona os tokens que coletamos a tabela com sua tipagem e tudo mais
int addToken(){

}
//identifica operador
void automatoOperador(){
    
}
//identica identificador ou palavras reservadas
void automatoIdentificador(){
    
}
//identifica boa formação de numeros inteiros ou reais
void automatoNumero(){

}

void automatoParenteses(){

}

void automatoComentario(){

}

//fazer aqui o começo da analise para saber qual automato acionar
void lexico(const char* linha, int num_linha){
    int pointer = 0;


}