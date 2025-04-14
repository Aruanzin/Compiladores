#include "lexico.h"

tabelaReservados[] = {
    {"CALL","simbolo_CALL"},
    {"PROGRAM", "PROGRAM"},
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


void lexico(const char* linha, int num_linha){
    int pointer = 0;


}