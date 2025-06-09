#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexico.h"
#include "sintatico.h"

// Global variable definitions
Token current_token;
int num_erros_sintaticos = 0;
char linha_atual[256] = "";
int pos_atual = 0;
ContextFrame* current_context = NULL;

// Define shared synchronization token sets
TokenType SYNC_STATEMENT[] = {
    TOKEN_SEMICOLON, TOKEN_END, TOKEN_ELSE, TOKEN_DO, TOKEN_DOT,
    TOKEN_EOF, TOKEN_BEGIN, TOKEN_PROCEDURE, TOKEN_IF, TOKEN_WHILE
};

TokenType SYNC_DECLARATION[] = {
    TOKEN_PROCEDURE, TOKEN_BEGIN, TOKEN_VAR, TOKEN_CONST,
    TOKEN_IDENTIFIER, TOKEN_EOF
};

TokenType SYNC_BLOCK[] = {
    TOKEN_DOT, TOKEN_SEMICOLON, TOKEN_END, TOKEN_EOF, TOKEN_PROCEDURE, TOKEN_BEGIN
};

TokenType SYNC_EXPRESSION[] = {
    TOKEN_SEMICOLON, TOKEN_THEN, TOKEN_DO, TOKEN_RPAREN,
    TOKEN_END, TOKEN_EOF, TOKEN_ELSE
};

TokenType SYNC_COMMAND_END[] = {
    TOKEN_END, TOKEN_ELSE, TOKEN_SEMICOLON, TOKEN_DOT, TOKEN_EOF
};

int SYNC_STATEMENT_SIZE = sizeof(SYNC_STATEMENT) / sizeof(SYNC_STATEMENT[0]);
int SYNC_DECLARATION_SIZE = sizeof(SYNC_DECLARATION) / sizeof(SYNC_DECLARATION[0]);
int SYNC_BLOCK_SIZE = sizeof(SYNC_BLOCK) / sizeof(SYNC_BLOCK[0]);
int SYNC_EXPRESSION_SIZE = sizeof(SYNC_EXPRESSION) / sizeof(SYNC_EXPRESSION[0]);
int SYNC_COMMAND_END_SIZE = sizeof(SYNC_COMMAND_END) / sizeof(SYNC_COMMAND_END[0]);

// FOLLOW sets for each non-terminal
TokenType FOLLOW_PROGRAMA[] = {TOKEN_EOF};
TokenType FOLLOW_BLOCO[] = {TOKEN_DOT, TOKEN_SEMICOLON, TOKEN_EOF};
TokenType FOLLOW_DECLARACAO_CONST[] = {TOKEN_VAR, TOKEN_PROCEDURE, TOKEN_BEGIN, TOKEN_EOF};
TokenType FOLLOW_DECLARACAO_VAR[] = {TOKEN_PROCEDURE, TOKEN_BEGIN, TOKEN_EOF};
TokenType FOLLOW_DECLARACAO_PROC[] = {TOKEN_PROCEDURE, TOKEN_BEGIN, TOKEN_EOF};
TokenType FOLLOW_COMANDO[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_ELSE, TOKEN_DOT, TOKEN_EQUAL, TOKEN_EOF};
TokenType FOLLOW_COMANDO_COMPOSTO[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_ELSE, TOKEN_DOT, TOKEN_EOF};
TokenType FOLLOW_COMANDO_IF[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_ELSE, TOKEN_DOT, TOKEN_EOF};
TokenType FOLLOW_COMANDO_WHILE[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_ELSE, TOKEN_DOT, TOKEN_EOF};
TokenType FOLLOW_COMANDO_FOR[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_ELSE, TOKEN_DOT, TOKEN_EOF};
TokenType FOLLOW_CONDICAO[] = {TOKEN_THEN, TOKEN_DO, TOKEN_EOF};
TokenType FOLLOW_EXPRESSAO[] = {TOKEN_SEMICOLON, TOKEN_THEN, TOKEN_DO, TOKEN_RPAREN, TOKEN_END, TOKEN_ELSE, TOKEN_COMMA, TOKEN_EQUAL, TOKEN_NE, TOKEN_LT, TOKEN_LE, TOKEN_GT, TOKEN_GE, TOKEN_EOF};
TokenType FOLLOW_TERMO[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_SEMICOLON, TOKEN_THEN, TOKEN_DO, TOKEN_RPAREN, TOKEN_END, TOKEN_ELSE, TOKEN_COMMA, TOKEN_EQUAL, TOKEN_NE, TOKEN_LT, TOKEN_LE, TOKEN_GT, TOKEN_GE, TOKEN_EOF};
TokenType FOLLOW_FATOR[] = {TOKEN_MULT, TOKEN_DIV, TOKEN_PLUS, TOKEN_MINUS, TOKEN_SEMICOLON, TOKEN_THEN, TOKEN_DO, TOKEN_RPAREN, TOKEN_END, TOKEN_ELSE, TOKEN_COMMA, TOKEN_EQUAL, TOKEN_NE, TOKEN_LT, TOKEN_LE, TOKEN_GT, TOKEN_GE, TOKEN_EOF};

// Extra synchronization symbols to prevent excessive consumption
TokenType EXTRA_SYNC[] = {
    TOKEN_CONST, TOKEN_VAR, TOKEN_PROCEDURE, TOKEN_BEGIN, TOKEN_END,
    TOKEN_IF, TOKEN_THEN, TOKEN_ELSE, TOKEN_WHILE, TOKEN_DO, TOKEN_FOR,
    TOKEN_CALL, TOKEN_DOT, TOKEN_SEMICOLON, TOKEN_EOF
};

int FOLLOW_SIZES[] = {
    1, 3, 4, 3, 3, 6, 5, 5, 5, 5, 3, 17, 17, 17
};

int EXTRA_SYNC_SIZE = sizeof(EXTRA_SYNC) / sizeof(EXTRA_SYNC[0]);

TokenType* get_follow_set(ParseContext context, int* size) {
    switch (context) {
        case CONTEXT_PROGRAMA: *size = FOLLOW_SIZES[0]; return FOLLOW_PROGRAMA;
        case CONTEXT_BLOCO: *size = FOLLOW_SIZES[1]; return FOLLOW_BLOCO;
        case CONTEXT_DECLARACAO_CONST: *size = FOLLOW_SIZES[2]; return FOLLOW_DECLARACAO_CONST;
        case CONTEXT_DECLARACAO_VAR: *size = FOLLOW_SIZES[3]; return FOLLOW_DECLARACAO_VAR;
        case CONTEXT_DECLARACAO_PROC: *size = FOLLOW_SIZES[4]; return FOLLOW_DECLARACAO_PROC;
        case CONTEXT_COMANDO: *size = FOLLOW_SIZES[5]; return FOLLOW_COMANDO;
        case CONTEXT_COMANDO_COMPOSTO: *size = FOLLOW_SIZES[6]; return FOLLOW_COMANDO_COMPOSTO;
        case CONTEXT_COMANDO_IF: *size = FOLLOW_SIZES[7]; return FOLLOW_COMANDO_IF;
        case CONTEXT_COMANDO_WHILE: *size = FOLLOW_SIZES[8]; return FOLLOW_COMANDO_WHILE;
        case CONTEXT_COMANDO_FOR: *size = FOLLOW_SIZES[9]; return FOLLOW_COMANDO_FOR;
        case CONTEXT_CONDICAO: *size = FOLLOW_SIZES[10]; return FOLLOW_CONDICAO;
        case CONTEXT_EXPRESSAO: *size = FOLLOW_SIZES[11]; return FOLLOW_EXPRESSAO;
        case CONTEXT_TERMO: *size = FOLLOW_SIZES[12]; return FOLLOW_TERMO;
        case CONTEXT_FATOR: *size = FOLLOW_SIZES[13]; return FOLLOW_FATOR;
        default: *size = 1; return &(TokenType){TOKEN_EOF};
    }
}

void push_context(ParseContext context) {
    ContextFrame* frame = malloc(sizeof(ContextFrame));
    frame->context = context;
    frame->followers = get_follow_set(context, &frame->followers_count);
    frame->parent = current_context;
    current_context = frame;
}

void pop_context() {
    if (current_context) {
        ContextFrame* old = current_context;
        current_context = current_context->parent;
        free(old);
    }
}

void mostrar_erro_visual(const char* msg, const char* dica) {
    if (show_hints) {
        printf("\033[1;31m🚨 Erro sintático\033[0m na linha %d:\n", current_token.linha);
        printf("   %s\n", msg);
        
        // Mostrar a linha original se disponível
        if (strlen(linha_atual) > 0) {
            printf("\n📄 Código:\n");
            printf("   %s", linha_atual);
            if (linha_atual[strlen(linha_atual)-1] != '\n') {
                printf("\n");
            }
            
            // Calcular posição do ponteiro
            int pos_ponteiro = pos_atual;
            
            // Ajustar posição se o token atual tem lexema conhecido
            if (strlen(current_token.lexema) > 0) {
                char* token_pos = strstr(linha_atual, current_token.lexema);
                if (token_pos) {
                    pos_ponteiro = token_pos - linha_atual;
                }
            }
            
            // Mostrar ponteiro visual
            printf("   ");
            for (int i = 0; i < pos_ponteiro; i++) {
                printf(" ");
            }
            printf("\033[1;31m^\033[0m\n");
            
            // Mostrar dica
            if (dica && strlen(dica) > 0) {
                printf("💡 \033[1;33mDica:\033[0m %s\n", dica);
            }
        }
        
        printf("🔍 Token encontrado: '\033[1;36m%s\033[0m' (tipo: %s)\n\n", 
               current_token.lexema, tokenTypeNames[current_token.tipo]);
    } else {
        printf("Erro sintático na linha %d: %s (token: '%s')\n",
           current_token.linha, msg, current_token.lexema);
    }
    
    num_erros_sintaticos++;
}

void syntax_error(const char* msg) {
    mostrar_erro_visual(msg, "");
}

void syntax_error_with_hint(const char* msg, const char* dica) {
    if (show_hints) {
        mostrar_erro_visual(msg, dica);
    } else {
        mostrar_erro_visual(msg, "");
    }
}

void lexico_error(const char* msg) {
    
    if (show_hints) {
        printf("\033[1;31m🚨 Erro léxico\033[0m na linha %d: %s\n", 
               current_token.linha, msg);
        
        if (strlen(linha_atual) > 0) {
            printf("📄 Código: %s", linha_atual);
            if (linha_atual[strlen(linha_atual)-1] != '\n') {
                printf("\n");
            }
        }
        
        printf("🔍 Token problemático: '\033[1;36m%s\033[0m'\n\n", current_token.lexema);
    } else {
        printf("Erro léxico, linha %d\n", current_token.linha);
    }
    num_erros_lexicos++;
}

// Função para sugerir correção baseada no contexto
const char* sugerir_correcao(TokenType esperado, TokenType encontrado) {
    // Correções comuns baseadas no contexto
    if (esperado == TOKEN_SEMICOLON) {
        if (encontrado == TOKEN_END || encontrado == TOKEN_ELSE) {
            return "Adicione ';' antes desta palavra-chave";
        }
        return "Adicione ';' para terminar a declaração";
    }
    
    if (esperado == TOKEN_ASSIGN && encontrado == TOKEN_EQUAL) {
        return "Use ':=' para atribuição (não '=')";
    }
    
    if (esperado == TOKEN_THEN && encontrado == TOKEN_DO) {
        return "Use 'THEN' após condição em IF (DO é para WHILE)";
    }
    
    if (esperado == TOKEN_DO && encontrado == TOKEN_THEN) {
        return "Use 'DO' após condição em WHILE (THEN é para IF)";
    }
    
    if (esperado == TOKEN_IDENTIFIER) {
        if (encontrado == TOKEN_NUMBER) {
            return "Identificadores não podem começar com números";
        }
        return "Digite um nome válido para a variável";
    }
    
    if (esperado == TOKEN_NUMBER && encontrado == TOKEN_IDENTIFIER) {
        return "Esperado um número, mas encontrou identificador";
    }
    
    if (esperado == TOKEN_RPAREN && encontrado == TOKEN_SEMICOLON) {
        return "Feche os parênteses antes de terminar a expressão";
    }
    
    if (esperado == TOKEN_END && encontrado == TOKEN_DOT) {
        return "Use 'END' para fechar bloco BEGIN";
    }
    
    return NULL; // Sem dica específica
}

// Dynamic synchronization token calculation
TokenType* calculate_sync_tokens(int* total_size) {
    static TokenType sync_buffer[100];
    int count = 0;
    
    // Add FOLLOW set of current context
    if (current_context) {
        for (int i = 0; i < current_context->followers_count && count < 90; i++) {
            sync_buffer[count++] = current_context->followers[i];
        }
        
        // Add FOLLOW sets of parent contexts
        ContextFrame* parent = current_context->parent;
        while (parent && count < 80) {
            for (int i = 0; i < parent->followers_count && count < 80; i++) {
                // Avoid duplicates
                int duplicate = 0;
                for (int j = 0; j < count; j++) {
                    if (sync_buffer[j] == parent->followers[i]) {
                        duplicate = 1;
                        break;
                    }
                }
                if (!duplicate) {
                    sync_buffer[count++] = parent->followers[i];
                }
            }
            parent = parent->parent;
        }
    }
    
    // Add extra synchronization symbols
    for (int i = 0; i < EXTRA_SYNC_SIZE && count < 95; i++) {
        // Avoid duplicates
        int duplicate = 0;
        for (int j = 0; j < count; j++) {
            if (sync_buffer[j] == EXTRA_SYNC[i]) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate) {
            sync_buffer[count++] = EXTRA_SYNC[i];
        }
    }
    
    *total_size = count;
    return sync_buffer;
}

int advanced_synchronize() {
    int sync_size;
    TokenType* sync_tokens = calculate_sync_tokens(&sync_size);
    
    int tokens_consumed = 0;
    while (current_token.tipo != TOKEN_EOF) {
        // Check if current token is a synchronization token
        for (int i = 0; i < sync_size; i++) {
            if (current_token.tipo == sync_tokens[i]) {
                if (show_hints && tokens_consumed > 0) {
                    printf("🔄 \033[1;33mRecuperação:\033[0m Sincronizado em '%s' após %d token(s)\n", 
                           current_token.lexema, tokens_consumed);
                }
                return tokens_consumed;
            }
        }
        
        // Consume token and continue
        current_token = get_next_token();
        tokens_consumed++;
        
        // Safety check: don't consume too many tokens
        if (tokens_consumed > 50) {
            if (show_hints) {
                printf("⚠️  \033[1;33mAviso:\033[0m Muitos tokens consumidos na recuperação\n");
            }
            break;
        }
    }
    
    return tokens_consumed;
}

// Enhanced eat function with advanced recovery
int eat_advanced(TokenType esperado, ParseContext context) {
    if (current_token.tipo == esperado) {
        current_token = get_next_token();
        if (current_token.tipo == TOKEN_ERROR_LEXICO) {
            return 0; // Error
        }
        return 1; // Success
    } else {
        char msg[256];
        // Updated message to show both expected and found tokens
        snprintf(msg, sizeof(msg), "Esperado token '%s', mas encontrado '%s'", 
                 tokenTypeNames[esperado], tokenTypeNames[current_token.tipo]);

        // Perform advanced synchronization for general errors
        push_context(context);
        int consumed = advanced_synchronize();
        pop_context();
        
        if (show_hints && consumed > 0) {
            printf("🔧 \033[1;32mContinuando análise...\033[0m\n\n");
        }
        
        return 0; // Error, synchronization occurred
    }
}

// Special function to detect and handle missing VAR declarations
int detect_missing_var_declaration() {
    // Look for pattern: identifier followed by comma or identifier followed by semicolon
    // This suggests a variable list without VAR keyword
    if (current_token.tipo == TOKEN_IDENTIFIER) {
        // Save current state for lookahead
        Token saved_token = current_token;
        Token next_token = get_next_token();
        
        // Check for variable declaration patterns
        int looks_like_var_list = 0;
        if (next_token.tipo == TOKEN_COMMA || next_token.tipo == TOKEN_SEMICOLON) {
            looks_like_var_list = 1;
        }
        
        // Restore token for processing
        current_token = saved_token;
        return looks_like_var_list;
    }
    return 0;
}

// Special function to detect multiple BEGIN blocks (missing PROCEDURE)
int detect_multiple_begin_blocks() {
    // This function is called when we encounter a second BEGIN
    // Check if we're at the top level (not inside a procedure)
    ContextFrame* frame = current_context;
    int in_procedure = 0;
    
    while (frame) {
        if (frame->context == CONTEXT_DECLARACAO_PROC) {
            in_procedure = 1;
            break;
        }
        frame = frame->parent;
    }
    
    return !in_procedure; // Multiple BEGINs at top level suggests missing PROCEDURE
}

// Enhanced programa function with better structure error detection
void programa_enhanced() {
    push_context(CONTEXT_PROGRAMA);
    
    // First, try to parse as a normal program
    int parse_success = bloco_enhanced();
    
    // Check for common program structure errors
    if (!parse_success) {
        if (show_hints) {
            printf("🔍 \033[1;33mAnálise de estrutura:\033[0m Detectando problemas comuns...\n");
        }
        
        // Reset and try alternative parsing strategies
        // This is where we could implement recovery for major structural issues
        syntax_error_with_hint("Estrutura de programa inválida", 
                             "Verifique se o programa segue a estrutura: [CONST...] [VAR...] [PROCEDURE...] BEGIN...END.");
    }
    
    if (!eat_advanced(TOKEN_DOT, CONTEXT_PROGRAMA)) {
        if (show_hints) {
            printf("⚠️  Programa pode estar incompleto - esperado '.' no final\n");
        }
    }
    pop_context();
}

// Enhanced bloco function with special error handling
int bloco_enhanced() {
    push_context(CONTEXT_BLOCO);
    int success = 1;
    int has_main_command = 0;
    
    // Handle CONST declarations
    if (current_token.tipo == TOKEN_CONST) {
        push_context(CONTEXT_DECLARACAO_CONST);
        eat_advanced(TOKEN_CONST, CONTEXT_DECLARACAO_CONST);
        eat_advanced(TOKEN_IDENTIFIER, CONTEXT_DECLARACAO_CONST);
        eat_advanced(TOKEN_EQUAL, CONTEXT_DECLARACAO_CONST);
        eat_advanced(TOKEN_NUMBER, CONTEXT_DECLARACAO_CONST);
        while (current_token.tipo == TOKEN_COMMA) {
            eat_advanced(TOKEN_COMMA, CONTEXT_DECLARACAO_CONST);
            eat_advanced(TOKEN_IDENTIFIER, CONTEXT_DECLARACAO_CONST);
            eat_advanced(TOKEN_EQUAL, CONTEXT_DECLARACAO_CONST);
            eat_advanced(TOKEN_NUMBER, CONTEXT_DECLARACAO_CONST);
        }
        eat_advanced(TOKEN_SEMICOLON, CONTEXT_DECLARACAO_CONST);
        pop_context();
    }

    // Enhanced VAR handling with missing VAR detection
    if (current_token.tipo == TOKEN_VAR) {
        push_context(CONTEXT_DECLARACAO_VAR);
        eat_advanced(TOKEN_VAR, CONTEXT_DECLARACAO_VAR);
        eat_advanced(TOKEN_IDENTIFIER, CONTEXT_DECLARACAO_VAR);
        while (current_token.tipo == TOKEN_COMMA) {
            eat_advanced(TOKEN_COMMA, CONTEXT_DECLARACAO_VAR);
            eat_advanced(TOKEN_IDENTIFIER, CONTEXT_DECLARACAO_VAR);
        }
        if (current_token.tipo != TOKEN_SEMICOLON) {
            syntax_error("esperado ';' após declaração de variáveis");
            advanced_synchronize();
        } else {
            eat_advanced(TOKEN_SEMICOLON, CONTEXT_DECLARACAO_VAR);
        }
        pop_context();
    } else if (detect_missing_var_declaration()) {
        // Handle missing VAR keyword
        if (show_hints) {
            syntax_error_with_hint("Possível declaração de variáveis sem 'VAR'", 
                                 "Adicione 'VAR' antes da lista de identificadores. Ex: VAR x, y;");
        } else {
            syntax_error("Esperado 'VAR' antes da declaração de variáveis");
        }
        
        // Try to recover by parsing as variable declaration
        push_context(CONTEXT_DECLARACAO_VAR);
        eat_advanced(TOKEN_IDENTIFIER, CONTEXT_DECLARACAO_VAR);
        while (current_token.tipo == TOKEN_COMMA) {
            eat_advanced(TOKEN_COMMA, CONTEXT_DECLARACAO_VAR);
            eat_advanced(TOKEN_IDENTIFIER, CONTEXT_DECLARACAO_VAR);
        }
        if (current_token.tipo == TOKEN_SEMICOLON) {
            eat_advanced(TOKEN_SEMICOLON, CONTEXT_DECLARACAO_VAR);
        } else {
            syntax_error("esperado ';' após declaração de variáveis");
            advanced_synchronize();
        }
        pop_context();
        success = 0; // Mark as having errors but recovered
    }

    // Handle PROCEDURE declarations
    while (current_token.tipo == TOKEN_PROCEDURE) {
        push_context(CONTEXT_DECLARACAO_PROC);
        eat_advanced(TOKEN_PROCEDURE, CONTEXT_DECLARACAO_PROC);
        eat_advanced(TOKEN_IDENTIFIER, CONTEXT_DECLARACAO_PROC);
        eat_advanced(TOKEN_SEMICOLON, CONTEXT_DECLARACAO_PROC);
        if(!bloco_enhanced()){
            syntax_error_with_hint("Erro no bloco do procedimento", 
                                 "Verifique se o procedimento está bem formado");
            advanced_synchronize();
            success = 0;
        }
        if (current_token.tipo != TOKEN_SEMICOLON) {
            syntax_error_with_hint("Esperado ';' após bloco do procedimento", 
                                 "Procedimentos devem terminar com ponto e vírgula");
            advanced_synchronize();
            success = 0;
        } else {
            eat_advanced(TOKEN_SEMICOLON, CONTEXT_DECLARACAO_PROC);
        }
        pop_context();
    }

    // Handle main command with special detection for multiple BEGINs
    if (current_token.tipo == TOKEN_BEGIN) {
        if (has_main_command && detect_multiple_begin_blocks()) {
            if (show_hints) {
                syntax_error_with_hint("Múltiplos blocos BEGIN detectados", 
                                     "Cada bloco adicional deveria ser um PROCEDURE. Ex: PROCEDURE nome; BEGIN...END;");
            } else {
                syntax_error("Múltiplos blocos BEGIN sem PROCEDURE");
            }
            success = 0;
        }
        has_main_command = 1;
    }
    
    int command_result = comando_enhanced();
    if (!command_result) {
        success = 0;
    }
    
    pop_context();
    return success;
}

// Enhanced comando function with better error detection
int comando_enhanced() {
    push_context(CONTEXT_COMANDO);
    int result = 1;
    
    if (current_token.tipo == TOKEN_IDENTIFIER) {
        eat_advanced(TOKEN_IDENTIFIER, CONTEXT_COMANDO);
        if(!eat_advanced(TOKEN_ASSIGN, CONTEXT_COMANDO)){
            if (current_token.tipo == TOKEN_EQUAL) {
                syntax_error_with_hint("Esperado ':=' para atribuição, mas encontrado '='", 
                                     "Use ':=' para atribuições em PL/0");
                eat_advanced(TOKEN_EQUAL, CONTEXT_COMANDO);
            } else if (current_token.tipo == TOKEN_COMMA) {
                // This might be a misplaced variable declaration
                syntax_error_with_hint("Possível declaração de variáveis fora do lugar", 
                                     "Declarações de variáveis devem vir antes dos comandos e começar com 'VAR'");
                advanced_synchronize();
                result = 0;
                pop_context();
                return result;
            }
        }
        if (result) {
            expressao();
        }
    } else if (current_token.tipo == TOKEN_CALL) {
        eat_advanced(TOKEN_CALL, CONTEXT_COMANDO);
        eat_advanced(TOKEN_IDENTIFIER, CONTEXT_COMANDO);
    } else if (current_token.tipo == TOKEN_BEGIN) {
        // Check for multiple BEGIN blocks at wrong level
        if (detect_multiple_begin_blocks()) {
            if (show_hints) {
                syntax_error_with_hint("Bloco BEGIN adicional detectado", 
                                     "Considere usar PROCEDURE para organizar código adicional");
            }
        }
        
        push_context(CONTEXT_COMANDO_COMPOSTO);
        eat_advanced(TOKEN_BEGIN, CONTEXT_COMANDO_COMPOSTO);
        
        int comandos_processados = 0;
        if (current_token.tipo != TOKEN_END) {
            if (comando_enhanced()) {
                comandos_processados++;
            }
        }
        
        while (current_token.tipo == TOKEN_SEMICOLON) {
            eat_advanced(TOKEN_SEMICOLON, CONTEXT_COMANDO_COMPOSTO);
            
            if (current_token.tipo == TOKEN_END) {
                syntax_error_with_hint("Ponto e vírgula desnecessário antes de 'END'", 
                                     "Remova o ';' antes de END");
                break;
            } else if (current_token.tipo == TOKEN_ELSE) {
                syntax_error_with_hint("Ponto e vírgula inválido antes de 'ELSE'", 
                                     "ELSE deve seguir diretamente após comando IF");
                break;
            } else if (current_token.tipo == TOKEN_DOT) {
                syntax_error_with_hint("Ponto e vírgula antes de '.' final do programa", 
                                     "O programa deve terminar apenas com '.'");
                break;
            } else if (current_token.tipo == TOKEN_EOF) {
                syntax_error_with_hint("Programa terminado inesperadamente", 
                                     "Bloco BEGIN não foi fechado com END");
                break;
            }
            
            if (comando_enhanced()) {
                comandos_processados++;
            } else {
                break;
            }
        }
        
        if (current_token.tipo != TOKEN_END) {
            if (current_token.tipo == TOKEN_EOF) {
                syntax_error_with_hint("Bloco BEGIN não foi fechado", 
                                     "Adicione 'END' para fechar o bloco BEGIN");
            } else if (current_token.tipo == TOKEN_DOT) {
                syntax_error_with_hint("Bloco BEGIN não foi fechado antes do fim do programa", 
                                     "Adicione 'END' antes do '.' final");
            } else {
                syntax_error_with_hint("Token inesperado em bloco BEGIN", 
                                     "Esperado 'END' para fechar o bloco");
            }
            advanced_synchronize();
        } else {
            eat_advanced(TOKEN_END, CONTEXT_COMANDO_COMPOSTO);
        }
        
        result = comandos_processados > 0;
        pop_context();

    } else if (current_token.tipo == TOKEN_IF) {
        push_context(CONTEXT_COMANDO_IF);
        eat_advanced(TOKEN_IF, CONTEXT_COMANDO_IF);
        condicao();
        if (current_token.tipo != TOKEN_THEN) {
            syntax_error_with_hint("Esperado 'THEN' após condição IF", 
                                 "Estrutura: IF condição THEN comando");
            advanced_synchronize();
        } else {
            eat_advanced(TOKEN_THEN, CONTEXT_COMANDO_IF);
        }

        if (!comando_enhanced()) {
            syntax_error_with_hint("Comando inválido após THEN", 
                                 "IF deve ter um comando válido após THEN");
        }
        
        if (current_token.tipo == TOKEN_ELSE) {
            eat_advanced(TOKEN_ELSE, CONTEXT_COMANDO_IF);
            if (!comando_enhanced()) {
                syntax_error_with_hint("Comando inválido após ELSE", 
                                     "ELSE deve ter um comando válido");
            }
        }
        pop_context();
    } else if (current_token.tipo == TOKEN_WHILE) {
        push_context(CONTEXT_COMANDO_WHILE);
        eat_advanced(TOKEN_WHILE, CONTEXT_COMANDO_WHILE);
        condicao();
        
        if (current_token.tipo != TOKEN_DO) {
            syntax_error_with_hint("Esperado 'DO' após condição WHILE", 
                                 "Estrutura: WHILE condição DO comando");
            advanced_synchronize();
        } else {
            eat_advanced(TOKEN_DO, CONTEXT_COMANDO_WHILE);
        }
        
        if (!comando_enhanced()) {
            syntax_error_with_hint("Comando inválido após DO", 
                                 "WHILE deve ter um comando válido após DO");
        }
        pop_context();
    } else if (current_token.tipo == TOKEN_FOR) {
        push_context(CONTEXT_COMANDO_FOR);
        eat_advanced(TOKEN_FOR, CONTEXT_COMANDO_FOR);
        eat_advanced(TOKEN_IDENTIFIER, CONTEXT_COMANDO_FOR);
        eat_advanced(TOKEN_ASSIGN, CONTEXT_COMANDO_FOR);
        expressao();
        if (current_token.tipo != TOKEN_DO) {
            syntax_error_with_hint("Esperado 'DO' em comando FOR", 
                                 "Estrutura: FOR var := expr DO comando");
            advanced_synchronize();
        } else {
            eat_advanced(TOKEN_DO, CONTEXT_COMANDO_FOR);
        }
        
        if (!comando_enhanced()) {
            syntax_error_with_hint("Comando inválido após DO em FOR", 
                                 "FOR deve ter um comando válido após DO");
        }
        pop_context();
    } else if (current_token.tipo == TOKEN_END || current_token.tipo == TOKEN_ELSE || 
               current_token.tipo == TOKEN_DOT || current_token.tipo == TOKEN_SEMICOLON) {
        // comando vazio: epsilon
    } else {
        if (show_hints) {
            const char* dica = NULL;
            if (current_token.tipo == TOKEN_NUMBER) {
                dica = "Comandos não podem começar com números";
            } else if (current_token.tipo == TOKEN_EQUAL) {
                dica = "Use ':=' para atribuição, não '='";
            } else if (current_token.tipo == TOKEN_EOF) {
                dica = "Programa terminou inesperadamente";
            }
            
            if (dica) {
                syntax_error_with_hint("Esperado início de comando", dica);
            } else {
                syntax_error("Esperado início de comando");
            }
        } else {
            syntax_error("Esperado início de comando");
        }
        advanced_synchronize();
        result = 0;
    }
    
    pop_context();
    return result;
}

void condicao() {
    push_context(CONTEXT_CONDICAO);
    if (current_token.tipo == TOKEN_IDENTIFIER || current_token.tipo == TOKEN_NUMBER) {
        expressao();
        if (current_token.tipo == TOKEN_EQUAL || current_token.tipo == TOKEN_NE ||
            current_token.tipo == TOKEN_LT || current_token.tipo == TOKEN_LE ||
            current_token.tipo == TOKEN_GT || current_token.tipo == TOKEN_GE) {
            eat_advanced(current_token.tipo, CONTEXT_CONDICAO);
            expressao();
        }
    } else {
        syntax_error("esperada condição");
        advanced_synchronize();
    }
    pop_context();
}

void expressao() {
    push_context(CONTEXT_EXPRESSAO);
    if (current_token.tipo == TOKEN_PLUS || current_token.tipo == TOKEN_MINUS) {
        eat_advanced(current_token.tipo, CONTEXT_EXPRESSAO);
    }
    termo();
    while (current_token.tipo == TOKEN_PLUS || current_token.tipo == TOKEN_MINUS) {
        eat_advanced(current_token.tipo, CONTEXT_EXPRESSAO);
        termo();
    }
    pop_context();
}

void termo() {
    push_context(CONTEXT_TERMO);
    fator();
    while (current_token.tipo == TOKEN_MULT || current_token.tipo == TOKEN_DIV) {
        eat_advanced(current_token.tipo, CONTEXT_TERMO);
        fator();
    }
    pop_context();
}

void fator() {
    push_context(CONTEXT_FATOR);
    if (current_token.tipo == TOKEN_IDENTIFIER) {
        eat_advanced(TOKEN_IDENTIFIER, CONTEXT_FATOR);
    } else if (current_token.tipo == TOKEN_NUMBER) {
        eat_advanced(TOKEN_NUMBER, CONTEXT_FATOR);
    } else if (current_token.tipo == TOKEN_LPAREN) {
        eat_advanced(TOKEN_LPAREN, CONTEXT_FATOR);
        expressao();
        eat_advanced(TOKEN_RPAREN, CONTEXT_FATOR);
    } else {
        if (show_hints) {
            const char* dica = NULL;
            if (current_token.tipo == TOKEN_RPAREN) {
                dica = "Expressão vazia dentro dos parênteses";
            } else if (current_token.tipo == TOKEN_SEMICOLON) {
                dica = "Expressão incompleta antes do ';'";
            }
            
            if (dica) {
                syntax_error_with_hint("Esperado fator (identificador, número ou expressão entre parênteses)", dica);
            } else {
                syntax_error("Esperado fator");
            }
        } else {
            syntax_error("Esperado fator");
        }
        advanced_synchronize();
    }
    pop_context();
}
static int carregar_linha() {
    if (fgets(linha, sizeof(linha), fonte)) {
        linha_num++;
        pos = 0;
        
        // Armazenar linha atual para exibição de erros
        strcpy(linha_atual, linha);
        pos_atual = pos;
        
        return 1;
    }
    return 0; // EOF
}

void parse() {
    current_token = get_next_token();
    if (current_token.tipo == TOKEN_ERROR_LEXICO) {
        lexico_error("Erro léxico detectado no primeiro token");
    }
    programa_enhanced();
    if (current_token.tipo != TOKEN_EOF) {
        syntax_error("esperado fim de arquivo");
    }
    
    if (num_erros_sintaticos == 0 && num_erros_lexicos == 0) {
        printf("\033[1;32mAnálise concluída sem erros.\033[0m\n");
    } else {
        printf("\033[1;33mResumo:\033[0m %d erro(s) sintático(s), %d erro(s) léxico(s)\n",
            num_erros_sintaticos, num_erros_lexicos);
    }
}

Token get_next_token() {
    static int inicializado = 0;
    if (!inicializado) {
        inicializarTabelaReservadas();
        inicializado = 1;
    }

    if (!fonte) {
        fonte = fopen("input.pl0", "r");
        if (!fonte) exit(1);
    }

    Token resultado = { .tipo = TOKEN_EOF, .status = 0, .linha = linha_num };
    strcpy(resultado.lexema, "<EOF>");

    while (1) {
        if (linha[pos] == '\0') {
            if (!carregar_linha()) return resultado;
        }

        // Atualizar posição atual para marcação visual
        pos_atual = pos;

        char c = linha[pos];
        if (isspace(c)) { pos++; continue; }

        if (c == '{') {
            int avancar = automatoComentario(linha, pos, linha_num);
            pos += avancar;
            continue;
        }

        if (isSymbolChar(c)) {
            char next = linha[pos+1] ? linha[pos+1] : '\0';
            Token tok = automatoSymbol(&linha[pos], next, linha_num);
            pos += strlen(tok.lexema);
            return tok;
        }

        if (isalpha(c)) {
            int err, len = tamanhoTermo(linha, pos, &err);
            char termo[100];
            int sz = len<99 ? len : 99;
            strncpy(termo, linha+pos, sz); termo[sz]=0;
            pos += len;
            Token id = automatoIdentificador(termo, err, len, linha_num);
            return id;
        }

        if (isdigit(c)) {
            int err, len = tamanhoNumero(linha, pos, &err);
            char termo[100];
            int sz = len<99 ? len : 99;
            strncpy(termo, linha+pos, sz); termo[sz]=0;
            pos += len;
            Token num = automatoNumero(termo, err, len, linha_num);
            return num;
        }

        // lex error - usar sistema de relatório aprimorado
        Token erro = { .tipo = TOKEN_ERROR_LEXICO, .linha = linha_num, .status = 1 };
        erro.lexema[0] = c; erro.lexema[1] = '\0';
        pos++;
        return erro;
    }
}

int sincronizar(TokenType sincronizadores[], int n) {
    while (1) {
        for (int i = 0; i < n; i++) {
            if (current_token.tipo == sincronizadores[i] || current_token.tipo == TOKEN_EOF) {
                return i;  // Token sincronizador encontrado
            }
        }
        current_token = get_next_token();  // Descartar token inválido
    }
}

