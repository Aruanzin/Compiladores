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

// Move num_erros_lexicos to lexico.c
int num_erros_lexicos = 0;

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
    int has_dot = 0; // Flag to track if a decimal point has been encountered

    while (linha[pos+len] != '\0') {
        char c = linha[pos+len];

        if (isdigit(c)) {
            // Character is a digit, consume it
            len++;
        } else if (c == '.') {
            // Character is a decimal point
            if (has_dot) {
                // This is a second decimal point (e.g., "1.2.3").
                // The current numeric token ends before this second dot.
                // The first dot already set *erro = 1.
                break;
            }
            has_dot = 1; // Mark that a decimal point has been found
            *erro = 1;   // Set error flag because a non-digit (the dot) is part of the number
            len++;       // Consume the decimal point
        } else if (isspace((unsigned char)c) || isSymbolChar(c)) {
            // Character is a whitespace or a symbol (e.g., '+', ';').
            // This marks the end of the numeric token.
            // Note: isSymbolChar('.') is true, but the case of a first '.' is handled
            // by the `else if (c == '.')` block. If a '.' reaches here, it implies
            // it's a delimiter after non-numeric characters or a second dot scenario
            // which would have already broken the loop.
            break;
        } else {
            // Character is not a digit, not a decimal point, and not a recognized delimiter.
            // This means it's an invalid character within the number (e.g., 'a' in "12a3" or "1.a").
            *erro = 1; // Set error flag to indicate a non-digit character was found
            len++;     // Consume the invalid character as part of the malformed number token
        }
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
    // consumed should be the number of characters to advance in the input buffer
    // If '}' is found, consumed includes '}'. 
    // If '}' is not found (unterminated), consumed is up to the end of the current buffer segment.
    int consumed = pointer - start + (linha_buf[pointer] == '}' ? 1 : 0);
    
    if (linha_buf[pointer] == '}') {
        return consumed; // Correctly terminated comment
    }
    
    // Unterminated comment handling
    // raw_len_fragment is the length of the comment fragment on the current line
    int raw_len_fragment = pointer - start; 
    int copy_len = raw_len_fragment;
    if (copy_len >= 99) { // Ensure space for null terminator in comment_text[100]
        copy_len = 99;
    }
    
    char comment_text[100];
    strncpy(comment_text, linha_buf + start, copy_len);
    comment_text[copy_len] = '\0'; // Null-terminate

    // Remove trailing newline from comment_text if it exists, for cleaner error messages
    if (copy_len > 0 && comment_text[copy_len - 1] == '\n') {
        comment_text[copy_len - 1] = '\0';
        copy_len--; // Update copy_len to reflect the new length
    }
    
    // Also remove trailing carriage return if it exists (Windows line endings)
    if (copy_len > 0 && comment_text[copy_len - 1] == '\r') {
        comment_text[copy_len - 1] = '\0';
        copy_len--;
    }
    
    Token e = { .tipo = TOKEN_ERROR_LEXICO, .linha = num_linha, .status = 1 };
    // Use the cleaned comment_text for the lexema
    strncpy(e.lexema, comment_text, 99);
    e.lexema[99] = '\0'; // Ensure null termination for e.lexema
    
    report_lexical_error(e.lexema, num_linha, start, linha_buf, LEX_ERROR_UNTERMINATED_COMMENT);
    num_erros_lexicos++;  // Increment error counter
    addToken(e); // Add the error token
    
    // For an unterminated comment, 'consumed' should advance 'pos' in get_next_token
    // to the end of the current line fragment that was processed.
    // If the loop stopped at linha_buf[pointer] == '\0', then consumed is raw_len_fragment.
    // This means pos will point to '\0', and carregar_linha() will be called.
    return raw_len_fragment; // Consume the part of the comment found on this line
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
Token automatoIdentificador(const char* t, int err, int len, int l) {
    if (err || len > 99) {
        Token e = { .tipo = TOKEN_ERROR_LEXICO, .linha = l, .status = 0 };
        if (err) e.status |= 1; 
        if (len > 99) e.status |= 2;
        strncpy(e.lexema, t, 99); e.lexema[99] = 0;
        
        // Reportar erro específico
        LexErrorType error_type = len > 99 ? LEX_ERROR_IDENTIFIER_TOO_LONG : LEX_ERROR_IDENTIFIER_INVALID_CHAR;
        report_lexical_error(e.lexema, l, 0, "", error_type);
        num_erros_lexicos++;  // Increment error counter
        return e;
    }
    
    // Identificador válido ou palavra reservada
    Token r = { .linha = l, .status = 0 };
    int ht = hash_buscar(&tabelaPalavrasReservadas, (string)t);
    r.tipo = (ht >= 0) ? (TokenType)ht : TOKEN_IDENTIFIER;
    strcpy(r.lexema, t);
    return r;
}

Token automatoNumero(const char* t, int err, int len, int l) {
    if (err || len > 99) {
        Token e = { .tipo = TOKEN_ERROR_LEXICO, .linha = l, .status = 0 };
        if (err) e.status |= 1;
        if (len > 99) e.status |= 2;
        strncpy(e.lexema, t, 99); e.lexema[99] = 0;
        
        // Reportar erro específico
        LexErrorType error_type = len > 99 ? LEX_ERROR_NUMBER_TOO_LONG : LEX_ERROR_INVALID_NUMBER;
        report_lexical_error(e.lexema, l, 0, "", error_type);
        num_erros_lexicos++;  // Increment error counter
        return e;
    }
    
    // Validação adicional para números
    for(int i = 0; i < len; i++) {
        if (!isdigit(t[i])) {
            Token e = { .tipo = TOKEN_ERROR_LEXICO, .linha = l, .status = 1 };
            strncpy(e.lexema, t, 99); e.lexema[99] = 0;
            report_lexical_error(e.lexema, l, 0, "", LEX_ERROR_INVALID_NUMBER);
            num_erros_lexicos++;  // Increment error counter
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

        // 5) caso não reconhecido - usar função de relatório de erro apropriada
        DBG_PRINT("Caractere não reconhecido: '%c'\n", caracter[0]);
        Token error = {0}; // Initialize struct
        error.linha = num_linha;
        error.status = 1; // Indicate error status
        error.tipo = TOKEN_ERROR_LEXICO; // Set the error type
        strcpy(error.lexema, caracter); // Store the problematic character
        
        // Usar a função de relatório de erro apropriada
        LexErrorType error_type = classify_lex_error(caracter[0], caracter, 0);
        report_lexical_error(caracter, num_linha, pointer, linha, error_type);
        num_erros_lexicos++;  // Increment error counter
        
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
// Função para classificar o tipo de erro léxico
LexErrorType classify_lex_error(char c, const char* term, int error_flags) {
    if (c == '_') return LEX_ERROR_UNDERSCORE;
    if (c == '.' && isdigit(term[0])) return LEX_ERROR_DECIMAL_NUMBER;
    if (c == '"' || c == '\'') return LEX_ERROR_STRING_LITERAL;
    if (c == '&' || c == '|' || c == '!' || c == '^') return LEX_ERROR_LOGICAL_OPERATOR;
    if (error_flags & 2) return LEX_ERROR_IDENTIFIER_TOO_LONG;
    if (error_flags & 1) return LEX_ERROR_IDENTIFIER_INVALID_CHAR;
    return LEX_ERROR_INVALID_CHAR;
}

// Função melhorada para reportar erros léxicos
void report_lexical_error(const char* problematic_text, int line, int position, const char* full_line, LexErrorType error_type) {
    if (show_hints) {
        printf("\033[1;31m🚨 Erro léxico\033[0m na linha %d:\n", line);
        printf("📄 Código: %s", full_line);
        if (full_line[strlen(full_line)-1] != '\n') {
            printf("\n");
        }
        
        // Mostrar ponteiro visual melhorado
        printf("   ");
        for (int i = 0; i < position; i++) {
            printf(full_line[i] == '\t' ? "\t" : " ");
        }
        printf("\033[1;31m");
        for (int i = 0; i < (int)strlen(problematic_text); i++) {
            printf("^");
        }
        printf("\033[0m\n");
        
        // Mensagens de erro específicas com dicas detalhadas
        switch (error_type) {
            case LEX_ERROR_UNDERSCORE:
                printf("❌ \033[1;31mErro:\033[0m Underscore não é permitido em identificadores\n");
                printf("💡 \033[1;33mDica:\033[0m Use apenas letras e números. Ex: 'minha_var' → 'minhaVar' ou 'minha2'\n");
                printf("📖 \033[1;36mExemplo:\033[0m 'contador', 'valor1', 'soma2024'\n");
                break;
                
            case LEX_ERROR_DECIMAL_NUMBER:
                printf("❌ \033[1;31mErro:\033[0m Números decimais não são suportados\n");
                printf("💡 \033[1;33mDica:\033[0m Esta linguagem suporta apenas números inteiros\n");
                printf("📖 \033[1;36mExemplo:\033[0m Use '42' em vez de '4.2', '100' em vez de '10.0'\n");
                break;
                
            case LEX_ERROR_STRING_LITERAL:
                printf("❌ \033[1;31mErro:\033[0m Strings literais não são suportadas\n");
                printf("💡 \033[1;33mDica:\033[0m Esta linguagem não possui tipo string\n");
                printf("📖 \033[1;36mAlternativa:\033[0m Use números ou identificadores para representar dados\n");
                break;
                
            case LEX_ERROR_LOGICAL_OPERATOR:
                printf("❌ \033[1;31mErro:\033[0m Operador lógico '%s' não é suportado\n", problematic_text);
                printf("💡 \033[1;33mDica:\033[0m Use os operadores de comparação disponíveis\n");
                printf("📖 \033[1;36mOperadores válidos:\033[0m =, <>, <, <=, >, >=\n");
                break;
                
            case LEX_ERROR_UNTERMINATED_COMMENT:
                printf("❌ \033[1;31mErro:\033[0m Comentário não foi fechado\n");
                printf("💡 \033[1;33mDica:\033[0m Comentários devem começar com '{' e terminar com '}'\n");
                printf("📖 \033[1;36mExemplo:\033[0m { este é um comentário válido }\n");
                printf("🔍 \033[1;36mComentário encontrado:\033[0m '%s'\n", problematic_text);
                break;
                
            case LEX_ERROR_IDENTIFIER_TOO_LONG:
                printf("❌ \033[1;31mErro:\033[0m Identificador muito longo (máximo 99 caracteres)\n");
                printf("💡 \033[1;33mDica:\033[0m Use nomes mais curtos e descritivos\n");
                printf("📖 \033[1;36mExemplo:\033[0m 'contadorDeIteracoes' em vez de nomes extremamente longos\n");
                break;
                
            case LEX_ERROR_NUMBER_TOO_LONG:
                printf("❌ \033[1;31mErro:\033[0m Número muito longo (máximo 99 dígitos)\n");
                printf("💡 \033[1;33mDica:\033[0m Use números menores dentro dos limites suportados\n");
                printf("📖 \033[1;36mExemplo:\033[0m '12345' em vez de números com dezenas de dígitos\n");
                break;
                
            case LEX_ERROR_INVALID_NUMBER:
                printf("❌ \033[1;31mErro:\033[0m Número contém caracteres inválidos: '%s'\n", problematic_text);
                printf("💡 \033[1;33mDica:\033[0m Números devem conter apenas dígitos (0-9)\n");
                printf("📖 \033[1;36mExemplo:\033[0m '123' ✓, '12a3' ✗, '1.23' ✗\n");
                break;
                
            case LEX_ERROR_IDENTIFIER_INVALID_CHAR:
                printf("❌ \033[1;31mErro:\033[0m Identificador contém caracteres inválidos: '%s'\n", problematic_text);
                printf("💡 \033[1;33mDica:\033[0m Identificadores devem conter apenas letras e números\n");
                printf("📖 \033[1;36mExemplo:\033[0m 'valor1' ✓, 'val@r' ✗, 'var#2' ✗\n");
                break;
                
            default:
                printf("❌ \033[1;31mErro:\033[0m Caractere inválido '%s'\n", problematic_text);
                printf("💡 \033[1;33mDica:\033[0m Este caractere não é reconhecido pela linguagem\n");
                printf("📖 \033[1;36mCaracteres válidos:\033[0m a-z, A-Z, 0-9, +, -, *, /, =, <, >, (, ), {, }, ;, :, ., ,\n");
                break;
        }
        
        // Show token info for non-comment errors
        if (error_type != LEX_ERROR_UNTERMINATED_COMMENT) {
            printf("🔍 Token problemático: '\033[1;36m%s\033[0m' (linha %d, posição %d)\n", 
                   problematic_text, line, position + 1);
        } else {
            printf("📍 \033[1;33mLocalização:\033[0m linha %d, posição %d\n", line, position + 1);
        }
        
        // Informação adicional sobre código ASCII para caracteres especiais
        if (strlen(problematic_text) == 1 && error_type != LEX_ERROR_UNTERMINATED_COMMENT) {
            unsigned char ascii = (unsigned char)problematic_text[0];
            if (ascii < 32 || ascii > 126) {
                printf("⚠️  \033[1;33mAviso:\033[0m Caractere não imprimível (código ASCII: %d)\n", ascii);
            } else if (ascii >= 128) {
                printf("⚠️  \033[1;33mAviso:\033[0m Caractere não-ASCII (código: %d)\n", ascii);
            }
        }
        printf("\n");
    } else {
        // Modo básico melhorado
        const char* error_msg = "";
        switch (error_type) {
            case LEX_ERROR_UNDERSCORE:
                error_msg = "underscore não permitido em identificadores";
                break;
            case LEX_ERROR_DECIMAL_NUMBER:
                error_msg = "números decimais não suportados";
                break;
            case LEX_ERROR_STRING_LITERAL:
                error_msg = "strings não suportadas";
                break;
            case LEX_ERROR_LOGICAL_OPERATOR:
                error_msg = "operador lógico não suportado";
                break;
            case LEX_ERROR_UNTERMINATED_COMMENT:
                error_msg = "comentário não fechado";
                break;
            case LEX_ERROR_IDENTIFIER_TOO_LONG:
                error_msg = "identificador muito longo";
                break;
            case LEX_ERROR_NUMBER_TOO_LONG:
                error_msg = "número muito longo";
                break;
            case LEX_ERROR_INVALID_NUMBER:
                error_msg = "número inválido";
                break;
            case LEX_ERROR_IDENTIFIER_INVALID_CHAR:
                error_msg = "caractere inválido em identificador";
                break;
            default:
                error_msg = "caractere inválido";
                break;
        }
        printf("Erro léxico na linha %d: %s ('%s')\n", line, error_msg, problematic_text);
    }
}