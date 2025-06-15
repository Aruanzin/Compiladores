#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexico.h"
#include "sintatico.h"
#include "colors.h"
#include "statistics.h"
#include "suggestions.h"
#include "visualizer.h"
#include "utils.h"

// Global variable definitions
ErrorRecoveryStack* error_stack = NULL;
ContextFrame* current_context = NULL;
Token current_token;
int num_erros_sintaticos = 0;
char linha_atual[256];
int pos_atual = 0;

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

void init_error_recovery_stack() {
    error_stack = malloc(sizeof(ErrorRecoveryStack));
    error_stack->top = -1;
    error_stack->max_depth = MAX_CONTEXT_DEPTH;
}

void push_context_enhanced(ParseContext context, const char* name, 
                          TokenType* sync_tokens, int sync_count) {
    if (error_stack->top >= MAX_CONTEXT_DEPTH - 1) {
        fprintf(stderr, "Stack overflow: contexto muito profundo\n");
        return;
    }
    
    ContextFrame* frame = malloc(sizeof(ContextFrame));
    frame->context = context;
    frame->context_name = string_duplicate(name);
    frame->start_line = current_token.linha;
    frame->start_column = current_token.coluna;
    frame->sync_tokens = sync_tokens;
    frame->sync_count = sync_count;
    
    // Obter conjunto FOLLOW
    frame->followers = get_follow_set(context, &frame->followers_count);
    
    // Conectar na pilha
    frame->parent = (error_stack->top >= 0) ? error_stack->contexts[error_stack->top] : NULL;
    error_stack->contexts[++error_stack->top] = frame;
}

void pop_context_enhanced() {
    if (error_stack->top < 0) return;
    
    ContextFrame* frame = error_stack->contexts[error_stack->top];
    free(frame->context_name);
    free(frame);
    error_stack->top--;
}

// Recuperação de erro usando pilha de contexto
int recover_from_error(const char* error_msg) {
    if (error_stack->top < 0) {
        syntax_error("Erro sem contexto de recuperação");
        return 0;
    }
    
    // Mostrar erro com contexto apenas se show_hints estiver ativo
    ContextFrame* current = error_stack->contexts[error_stack->top];
    if (show_hints) {
        printf("ERRO SINTÁTICO: %s\n", error_msg);
        printf("Contexto: %s (linha %d, coluna %d)\n", 
            current->context_name, current->start_line, current->start_column);
    }
    
    // Tentar recuperação em múltiplos níveis
    for (int level = error_stack->top; level >= 0; level--) {
        ContextFrame* frame = error_stack->contexts[level];
        
        // Tentar sincronização com tokens do contexto atual
        if (try_sync_with_tokens(frame->sync_tokens, frame->sync_count)) {
            if (show_hints) {
                printf("Recuperação bem-sucedida no contexto: %s\n", frame->context_name);
            }
            return 1;
        }
        
        // Tentar sincronização com conjunto FOLLOW
        if (try_sync_with_tokens(frame->followers, frame->followers_count)) {
            if (show_hints) {
                printf("Recuperação usando FOLLOW do contexto: %s\n", frame->context_name);
            }
            return 1;
        }
    }
    
    // Recuperação de último recurso - mais conservadora
    return panic_mode_recovery();
}

// Tentar sincronização com conjunto de tokens
int try_sync_with_tokens(TokenType* tokens, int count) {
    for (int i = 0; i < count; i++) {
        if (current_token.tipo == tokens[i]) {
            return 1;
        }
    }
    
    // Avançar tokens até encontrar um de sincronização
    int max_skip = 50; // Limite para evitar loop infinito
    for (int skip = 0; skip < max_skip; skip++) {
        if (current_token.tipo == TOKEN_EOF) return 0;
        
        for (int i = 0; i < count; i++) {
            if (current_token.tipo == tokens[i]) {
                return 1;
            }
        }
        current_token = get_next_token();
    }
    
    return 0;
}


void mostrar_erro_visual(const char* msg, const char* dica) {
    if (show_hints) {
        printf("\n" BOLD RED SYMBOL_ERROR " Erro sintático" RESET_COLOR " na linha %d:\n", current_token.linha);
        printf("   %s\n", msg);
        
        // Mostrar a linha original se disponível
        if (strlen(linha_atual) > 0) {
            print_error_context(linha_atual, pos_atual, current_token.linha);
        }
        
        printf(CYAN SYMBOL_HINT " Token encontrado: " RESET_COLOR "'" BOLD CYAN "%s" RESET_COLOR "' (tipo: %s)\n", 
               current_token.lexema, tokenTypeNames[current_token.tipo]);
        
        // Add smart suggestions
        print_smart_suggestions("syntax_error", current_token.lexema, TOKEN_IDENTIFIER);
        
    } else {
        printf("Erro sintático na linha %d: %s (token: '%s')\n",
               current_token.linha, msg, current_token.lexema);
    }
    
    // Update statistics
    update_error_stats(current_token.linha, msg);
    stats.syntax_errors++;
    num_erros_sintaticos++;
}

void syntax_error(const char* msg) {
    mostrar_erro_visual(msg, "");
}
const char* token_to_string(TokenType token) {
    if (token >= 0) {
        return tokenTypeNames[token];
    }
    return "UNKNOWN_TOKEN";
}

// Implementar panic_mode_recovery
int panic_mode_recovery() {
    printf("Entrando em modo pânico - saltando tokens até encontrar ponto de sincronização\n");
    
    TokenType panic_tokens[] = {
        TOKEN_SEMICOLON, TOKEN_DOT, TOKEN_BEGIN, TOKEN_END, 
        TOKEN_VAR, TOKEN_CONST, TOKEN_PROCEDURE, TOKEN_EOF
    };
    int panic_count = 8;
    
    int max_skip = 50;
    for (int i = 0; i < max_skip && current_token.tipo != TOKEN_EOF; i++) {
        for (int j = 0; j < panic_count; j++) {
            if (current_token.tipo == panic_tokens[j]) {
                printf("Sincronização encontrada em: %s\n", token_to_string(current_token.tipo));
                return 1;
            }
        }
        // Avançar para o próximo token (você precisa implementar esta função)
        current_token = get_next_token();
    }
    
    return 0; // Não conseguiu recuperar
}

int declaracao_const_enhanced() {
    // Implementação básica - você pode expandir depois
    if (current_token.tipo != TOKEN_CONST) {
        return 0;
    }
    
    current_token = get_next_token(); // Consumir CONST
    
    // Processar declarações de constantes
    do {
        if (current_token.tipo != TOKEN_IDENTIFIER) {
            syntax_error("Esperado identificador após CONST");
            return 0;
        }
        current_token = get_next_token();
        
        if (current_token.tipo != TOKEN_EQUAL) {
            syntax_error("Esperado '=' após identificador da constante");
            return 0;
        }
        current_token = get_next_token();
        
        if (current_token.tipo != TOKEN_NUMBER) {
            syntax_error("Esperado número após '='");
            return 0;
        }
        current_token = get_next_token();
        
        if (current_token.tipo == TOKEN_COMMA) {
            current_token = get_next_token();
        }
    } while (current_token.tipo != TOKEN_SEMICOLON && current_token.tipo != TOKEN_EOF);
    
    if (current_token.tipo == TOKEN_SEMICOLON) {
        current_token = get_next_token();
    }
    
    return 1;
}

int declaracao_var_enhanced() {
    if (current_token.tipo != TOKEN_VAR) {
        return 0;
    }
    
    current_token = get_next_token(); // Consumir VAR
    
    // Processar declarações de variáveis
    do {
        if (current_token.tipo != TOKEN_IDENTIFIER) {
            syntax_error_with_hint("Esperado identificador após VAR", 
                                 "Liste os nomes das variáveis separados por vírgula");
            return 0;
        }
        current_token = get_next_token();
        
        if (current_token.tipo == TOKEN_COMMA) {
            current_token = get_next_token();
        } else if (current_token.tipo == TOKEN_IDENTIFIER) {
            // Erro comum: esqueceu vírgula entre identificadores
            syntax_error_with_hint("Esperado ',' entre identificadores de variáveis", 
                                 "Separe os nomes das variáveis com vírgulas");
            // Continuar assumindo que era uma vírgula esquecida
        }
    } while (current_token.tipo == TOKEN_IDENTIFIER);
    
    // Verificar ponto e vírgula obrigatório
    if (current_token.tipo != TOKEN_SEMICOLON) {
        if (current_token.tipo == TOKEN_BEGIN) {
            syntax_error_with_hint("Esperado ';' após declaração de variáveis", 
                                 "Declarações VAR devem terminar com ';'");
        } else {
            syntax_error_with_hint("Esperado ';' para finalizar declaração VAR", 
                                 "Formato: VAR nome1, nome2, ...;");
        }
        return 0;
    }
    
    current_token = get_next_token(); // Consumir ';'
    return 1;
}

int declaracao_proc_enhanced() {
    if (current_token.tipo != TOKEN_PROCEDURE) {
        return 0;
    }
    
    TokenType sync_proc[] = {TOKEN_SEMICOLON, TOKEN_BEGIN, TOKEN_PROCEDURE, TOKEN_EOF};
    push_context_enhanced(CONTEXT_DECLARACAO_PROC, "DECLARACAO_PROC", sync_proc, 4);
    
    current_token = get_next_token(); // Consumir PROCEDURE
    
    if (current_token.tipo != TOKEN_IDENTIFIER) {
        if (current_token.tipo == TOKEN_ERROR_LEXICO) {
            syntax_error_with_hint("Esperado nome do procedimento", 
                                 "PROCEDURE deve ser seguido por um identificador válido");
        } else {
            syntax_error_with_hint("Esperado nome do procedimento", 
                                 "PROCEDURE deve ser seguido por um identificador válido");
        }
        
        // Tentar recuperar procurando por ';' que indica fim da declaração do nome
        TokenType sync_after_error[] = {TOKEN_SEMICOLON, TOKEN_BEGIN, TOKEN_VAR, TOKEN_CONST, TOKEN_PROCEDURE, TOKEN_EOF};
        if (try_sync_with_tokens(sync_after_error, 6)) {
            if (current_token.tipo == TOKEN_SEMICOLON) {
                current_token = get_next_token(); // Consumir ';'
                // Continuar processando como se o nome fosse válido
            } else {
                pop_context_enhanced();
                return 0;
            }
        } else {
            pop_context_enhanced();
            return 0;
        }
    } else {
        current_token = get_next_token();
    }
    
    if (current_token.tipo != TOKEN_SEMICOLON) {
        syntax_error_with_hint("Esperado ';' após nome do procedimento", 
                             "Formato: PROCEDURE nome; bloco;");
        // Tentar recuperar
        TokenType sync_after_name[] = {TOKEN_SEMICOLON, TOKEN_BEGIN, TOKEN_VAR, TOKEN_CONST};
        if (!try_sync_with_tokens(sync_after_name, 4)) {
            pop_context_enhanced();
            return 0;
        }
        if (current_token.tipo == TOKEN_SEMICOLON) {
            current_token = get_next_token();
        }
    } else {
        current_token = get_next_token(); // Consumir ';'
    }
    
    // Verificar se há um comando direto (sem BEGIN/END) - erro comum
    ContextFrame* frame = error_stack->contexts[error_stack->top];
    if (current_token.tipo == TOKEN_IDENTIFIER && frame) {
        syntax_error_with_hint("Comando fora de bloco BEGIN/END", 
                             "Procedimentos devem ter seus comandos dentro de BEGIN...END");
        // Consumir o comando incorreto e continuar
        current_token = get_next_token(); // identifier
        if (current_token.tipo == TOKEN_ASSIGN) {
            current_token = get_next_token(); // :=
            expressao(); // consumir a expressão
        }
        // Não processar bloco, já que não há BEGIN
        pop_context_enhanced();
        return 0;
    }
    
    // Processar bloco do procedimento
    if (!bloco_enhanced()) {
        syntax_error_with_hint("Erro no corpo do procedimento", 
                             "Procedimento deve ter um bloco válido");
        advanced_synchronize();
        pop_context_enhanced();
        return 0;
    }
    
    // Verificar ';' após procedimento - OBRIGATÓRIO
    if (current_token.tipo != TOKEN_SEMICOLON) {
        syntax_error_with_hint("Esperado ';' após declaração de procedimento", 
                             "Procedimentos devem terminar com ';'");
        // Tentar encontrar o ';' ou próximo elemento válido
        TokenType sync_end_proc[] = {TOKEN_SEMICOLON, TOKEN_PROCEDURE, TOKEN_BEGIN, TOKEN_EOF};
        if (try_sync_with_tokens(sync_end_proc, 4)) {
            if (current_token.tipo == TOKEN_SEMICOLON) {
                current_token = get_next_token();
            }
        } else {
            pop_context_enhanced();
            return 0;
        }
    } else {
        current_token = get_next_token(); // Consumir ';'
    }
    
    pop_context_enhanced();
    return 1;
}


int comando_composto_enhanced() {
    if (current_token.tipo != TOKEN_BEGIN) {
        syntax_error("Esperado BEGIN");
        return 0;
    }
    current_token = get_next_token();
    
    int comandos_executados = 0;
    
    // Processar primeiro comando se não for END
    if (current_token.tipo != TOKEN_END && current_token.tipo != TOKEN_EOF) {
        if (current_token.tipo == TOKEN_DOT) {
            syntax_error_with_hint("Ponto encontrado antes de END", 
                                 "O ponto '.' deve vir apenas após END, no final do programa");
            return 0;
        }
        
        if (comando_enhanced()) {
            comandos_executados++;
            
            // Verificar se o comando terminou sem ponto e vírgula
            if (current_token.tipo == TOKEN_END) {
                // Comando sem ponto e vírgula antes de END - avisar mas aceitar
                if (show_hints) {
                    printf("💡 \033[1;33mAviso:\033[0m Último comando sem ';' - recomendado adicionar\n");
                }
            } else if (current_token.tipo != TOKEN_SEMICOLON && 
                      current_token.tipo != TOKEN_EOF && 
                      current_token.tipo != TOKEN_DOT) {
                syntax_error_with_hint("Esperado ';' após comando", 
                                     "Comandos devem terminar com ponto e vírgula");
                // Tentar recuperar
                TokenType sync_after_cmd[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_EOF};
                if (!try_sync_with_tokens(sync_after_cmd, 3)) {
                    return 0;
                }
            }
        } else {
            // Se comando falhou, tentar sincronizar
            TokenType sync_cmd[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_EOF};
            if (!try_sync_with_tokens(sync_cmd, 3)) {
                return 0; // Não conseguiu recuperar
            }
        }
    }
    
    // Processar comandos separados por ponto e vírgula
    while (current_token.tipo == TOKEN_SEMICOLON || 
           (current_token.tipo == TOKEN_IDENTIFIER && comandos_executados > 0)) {
        
        // Se encontrou identificador sem ';' precedente
        if (current_token.tipo == TOKEN_IDENTIFIER) {
            syntax_error_with_hint("Esperado ';' entre comandos", 
                                 "Comandos devem ser separados por ponto e vírgula");
        } else {
            current_token = get_next_token(); // Consumir ';'
        }
        
        if (current_token.tipo == TOKEN_END) {
            // Ponto e vírgula antes de END é permitido mas desnecessário
            if (show_hints) {
                printf("💡 \033[1;33mAviso:\033[0m Ponto e vírgula desnecessário antes de 'END'\n");
            }
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
            comandos_executados++;
        } else {
            // Se comando falhou, tentar sincronizar
            TokenType sync_cmd[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_EOF};
            if (!try_sync_with_tokens(sync_cmd, 3)) {
                break; // Não conseguiu recuperar
            }
        }
    }
    
    if (current_token.tipo != TOKEN_END) {
        if (current_token.tipo == TOKEN_EOF) {
            syntax_error_with_hint("Bloco BEGIN não foi fechado", 
                                 "Adicione 'END' para fechar o bloco BEGIN");
        } else if (current_token.tipo == TOKEN_DOT) {
            syntax_error_with_hint("Bloco BEGIN não foi fechado antes do fim do programa", 
                                 "Adicione 'END' antes do '.' final");
        } else if (current_token.tipo == TOKEN_IDENTIFIER && strcmp(current_token.lexema, "EN") == 0) {
            syntax_error_with_hint("Token inválido 'EN' - possível erro de digitação", 
                                 "Verifique se era para ser 'END'");
            // Tentar recuperar assumindo que era END
            current_token = get_next_token(); // Consumir 'EN'
            if (current_token.tipo == TOKEN_SEMICOLON) {
                // Parece que era mesmo END;
                current_token = get_next_token(); // Consumir ';'
                return 1; // Aceitar como se fosse END;
            }
        } else {
            syntax_error_with_hint("Token inesperado em bloco BEGIN", 
                                 "Esperado 'END' para fechar o bloco");
        }
        return 0;
    }
    
    current_token = get_next_token(); // Consumir END
    return 1; // Sempre retorna sucesso se chegou até END
}

int detect_missing_var_declaration() {
    // Look for pattern: identifier followed by comma or identifier followed by semicolon
    // This suggests a variable list without VAR keyword
    if (current_token.tipo == TOKEN_IDENTIFIER) {
        // Verificar se parece com uma lista de variáveis
        // Salvar estado atual
        int saved_pos = pos;
        char saved_line[256];
        strcpy(saved_line, linha);
        
        // Fazer lookahead simples
        Token next_token = get_next_token();
        
        // Restaurar estado
        pos = saved_pos;
        strcpy(linha, saved_line);
        
        // Verificar padrões típicos de declaração de variáveis
        if (next_token.tipo == TOKEN_COMMA || 
            next_token.tipo == TOKEN_SEMICOLON ||
            (next_token.tipo == TOKEN_IDENTIFIER)) {
            return 1; // Provável declaração de variáveis sem VAR
        }
    }
    return 0;
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
        printf("Erro léxico, linha %d - %s\n", current_token.linha, msg);
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
            lexico_error("Erro léxico encontrado");
            return 0; // Error
        }
        return 1; // Success
    } else {
        char msg[256];
        
        // Sugerir correção específica baseada no contexto
        const char* sugestao = sugerir_correcao(esperado, current_token.tipo);
        
        if (sugestao) {
            snprintf(msg, sizeof(msg), "Esperado '%s', mas encontrado '%s'", 
                     tokenTypeNames[esperado], tokenTypeNames[current_token.tipo]);
            syntax_error_with_hint(msg, sugestao);
        } else {
            snprintf(msg, sizeof(msg), "Esperado '%s', mas encontrado '%s'", 
                     tokenTypeNames[esperado], tokenTypeNames[current_token.tipo]);
            syntax_error(msg);
        }
        
        // Perform advanced synchronization for general errors
        TokenType sync_generic[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_DOT, TOKEN_EOF};
        push_context_enhanced(context, "RECUPERACAO", sync_generic, 4);
        int consumed = advanced_synchronize();
        pop_context_enhanced();
        
        if (show_hints && consumed > 0) {
            printf("🔧 \033[1;32mContinuando análise...\033[0m\n\n");
        }
        
        return 0; // Error, synchronization occurred
    }
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
    TokenType sync_programa[] = {TOKEN_DOT, TOKEN_EOF};
    push_context_enhanced(CONTEXT_PROGRAMA, "PROGRAMA", sync_programa, 2);
    
    // Inicializar pilha de recuperação de erros
    if (!error_stack) {
        init_error_recovery_stack();
    }
    
    // Analisar o bloco principal
    int parse_success = bloco_enhanced();
    
    // Verificar término do programa
    if (current_token.tipo == TOKEN_DOT) {
        current_token = get_next_token();
        
        // Verificar se há tokens extras após o ponto
        if (current_token.tipo != TOKEN_EOF) {
            syntax_error_with_hint("Token inesperado após '.' final", 
                                 "O programa deve terminar com '.'");
        }
    } else if (current_token.tipo == TOKEN_SEMICOLON) {
        // Verificar se realmente é final do programa ou se é de um procedimento
        // Se chegamos aqui após processar o bloco principal, é erro de final
        if (parse_success) {
            syntax_error_with_hint("Esperado '.' no final do programa, mas encontrado ';'", 
                                 "Programas devem terminar com '.' (não ';')");
        } else {
            // Se houve erro no parsing, pode ser que não conseguimos processar tudo
            syntax_error_with_hint("Erro na estrutura do programa", 
                                 "Verifique se todos os procedimentos estão corretos e o programa termina com '.'");
        }
        
        // Tentar recuperar procurando por '.' ou aceitando fim
        TokenType sync_final[] = {TOKEN_DOT, TOKEN_EOF};
        if (try_sync_with_tokens(sync_final, 2)) {
            if (current_token.tipo == TOKEN_DOT) {
                current_token = get_next_token();
            }
        }
    } else if (current_token.tipo == TOKEN_EOF) {
        if (!parse_success) {
            syntax_error_with_hint("Programa incompleto", 
                                 "Verifique se o programa está completo e termina com '.'");
        } else {
            syntax_error_with_hint("Programa terminou sem o ponto final", 
                                 "Todo programa deve terminar com '.'");
        }
    } else {
        if (!parse_success) {
            syntax_error_with_hint("Estrutura de programa inválida", 
                                 "Verifique se o programa segue a estrutura: [CONST...] [VAR...] [PROCEDURE...] BEGIN...END.");
        } else {
            syntax_error_with_hint("Esperado '.' no final do programa", 
                                 "O programa deve terminar com um ponto após END");
        }
        
        // Tentar encontrar o ponto final
        TokenType sync_dot[] = {TOKEN_DOT, TOKEN_EOF};
        if (try_sync_with_tokens(sync_dot, 2)) {
            if (current_token.tipo == TOKEN_DOT) {
                current_token = get_next_token();
            }
        }
    }
    
    pop_context_enhanced();
}
// Enhanced bloco function with special error handling
int bloco_enhanced() {
    TokenType sync_bloco[] = {TOKEN_BEGIN, TOKEN_END, TOKEN_DOT, TOKEN_SEMICOLON};
    push_context_enhanced(CONTEXT_BLOCO, "BLOCO", sync_bloco, 4);
    
    int success = 1;
    
    // Declarações de constantes
    if (current_token.tipo == TOKEN_CONST) {
        if (!declaracao_const_enhanced()) {
            success = 0;
            if (!recover_from_error("Erro na declaração de constantes")) {
                pop_context_enhanced();
                return 0;
            }
        }
    }
    
    // Declarações de variáveis - melhorar detecção de erro
    if (current_token.tipo == TOKEN_VAR) {
        if (!declaracao_var_enhanced()) {
            success = 0;
            if (!recover_from_error("Erro na declaração de variáveis")) {
                pop_context_enhanced();
                return 0;
            }
        }
    } else if (current_token.tipo == TOKEN_IDENTIFIER) {
        // Possível erro: palavra-chave mal digitada seguida de lista de variáveis
        if (detect_missing_var_declaration()) {
            syntax_error_with_hint("Possível erro de digitação em 'VAR'", 
                                 "Verifique se 'VAR' está escrito corretamente");
            // Tentar recuperar assumindo que era uma declaração VAR
            advanced_synchronize();
            // Continuar para encontrar BEGIN
        }
    }
    
    // Declarações de procedimentos - MELHORADA
    while (current_token.tipo == TOKEN_PROCEDURE) {
        if (!declaracao_proc_enhanced()) {
            // Se a declaração falhou, verificar se conseguimos recuperar
            if (current_token.tipo == TOKEN_PROCEDURE || current_token.tipo == TOKEN_BEGIN) {
                // Conseguimos recuperar e encontrar próximo procedimento ou BEGIN
                if (show_hints) {
                    printf("🔄 Recuperação: Continuando após erro em procedimento\n");
                }
                // Não marcar como falha total se conseguimos recuperar
                continue;
            } else {
                success = 0;
                if (!recover_from_error("Erro na declaração de procedimento")) {
                    // Tentar continuar - pode haver mais procedimentos
                    TokenType sync_next_proc[] = {TOKEN_PROCEDURE, TOKEN_BEGIN, TOKEN_EOF};
                    if (try_sync_with_tokens(sync_next_proc, 3)) {
                        continue; // Tentar próximo procedimento ou BEGIN
                    } else {
                        pop_context_enhanced();
                        return 0;
                    }
                }
            }
        }
    }
    
    // Se ainda não encontrou BEGIN, tentar sincronizar
    if (current_token.tipo != TOKEN_BEGIN) {
        if (show_hints) {
            printf("🔍 \033[1;33mBuscando BEGIN...\033[0m\n");
        }
        TokenType sync_begin[] = {TOKEN_BEGIN, TOKEN_EOF};
        if (try_sync_with_tokens(sync_begin, 2)) {
            if (show_hints) {
                printf("✅ \033[1;32mBEGIN encontrado, continuando análise\033[0m\n");
            }
        }
    }
    
    // Comando composto - OBRIGATÓRIO para programa principal
    if (current_token.tipo == TOKEN_BEGIN) {
        if (!comando_composto_enhanced()) {
            success = 0;
            if (!recover_from_error("Erro no comando composto")) {
                pop_context_enhanced();
                return 0;
            }
        }
    } else {
        syntax_error_with_hint("Esperado BEGIN para iniciar bloco de comandos", 
                             "Todo programa deve ter um bloco BEGIN...END");
        success = 0;
    }
    
    pop_context_enhanced();
    return success;
}

// Diagnóstico baseado na pilha de contexto
void diagnose_error_with_stack() {
    if (error_stack->top < 0) return;
    
    ContextFrame* frame = error_stack->contexts[error_stack->top];
    
    printf("\n=== DIAGNÓSTICO DE ERRO ===\n");
    printf("Token atual: %s (linha %d)\n", 
           token_to_string(current_token.tipo), current_token.linha);
    
    // Mostrar hierarquia de contextos
    printf("Hierarquia de contextos:\n");
    for (int i = error_stack->top; i >= 0; i--) {
        ContextFrame* ctx = error_stack->contexts[i];
        printf("  %d. %s (linha %d)\n", 
               error_stack->top - i + 1, ctx->context_name, ctx->start_line);
    }
    
    // Sugerir correções baseadas no contexto
    suggest_corrections_for_context(frame);
}


void suggest_corrections_for_context(ContextFrame* frame) {
    printf("\nSugestões de correção:\n");
    
    switch (frame->context) {
        case CONTEXT_PROGRAMA:
            printf("  - Verifique se o programa começa corretamente\n");
            printf("  - Deve ter estrutura: PROGRAM nome; bloco.\n");
            break;
            
        case CONTEXT_BLOCO:
            if (current_token.tipo == TOKEN_IDENTIFIER) {
                printf("  - Talvez você esqueceu de declarar a variável?\n");
                printf("  - Verifique se não falta um ';' antes desta linha\n");
            }
            break;
            
        case CONTEXT_DECLARACAO_CONST:
            printf("  - Formato: CONST nome = valor;\n");
            printf("  - Valores devem ser números inteiros\n");
            break;
            
        case CONTEXT_DECLARACAO_VAR:
            printf("  - Formato: VAR nome1, nome2, ...;\n");
            printf("  - Nomes devem ser identificadores válidos\n");
            break;
            
        case CONTEXT_DECLARACAO_PROC:
            printf("  - Formato: PROCEDURE nome; bloco;\n");
            printf("  - Procedimento deve ter BEGIN...END\n");
            break;
            
        case CONTEXT_COMANDO:
            printf("  - Tokens esperados: ");
            for (int i = 0; i < frame->followers_count; i++) {
                printf("%s ", token_to_string(frame->followers[i]));
            }
            printf("\n");
            break;
            
        case CONTEXT_COMANDO_COMPOSTO:
            printf("  - Comandos devem estar entre BEGIN e END\n");
            printf("  - Separar comandos com ';'\n");
            break;
            
        case CONTEXT_COMANDO_IF:
            printf("  - Formato: IF condição THEN comando\n");
            printf("  - Pode ter ELSE comando\n");
            break;
            
        case CONTEXT_COMANDO_WHILE:
            printf("  - Formato: WHILE condição DO comando\n");
            break;
            
        case CONTEXT_COMANDO_FOR:
            printf("  - Formato: FOR var := inicio TO fim DO comando\n");
            break;
            
        case CONTEXT_CONDICAO:
            printf("  - Use operadores: =, <>, <, <=, >, >=\n");
            printf("  - Formato: expressão operador expressão\n");
            break;
            
        case CONTEXT_EXPRESSAO:
            printf("  - Verifique se os parênteses estão balanceados\n");
            printf("  - Operadores devem estar entre operandos\n");
            break;
            
        case CONTEXT_TERMO:
            printf("  - Use *, / entre fatores\n");
            break;
            
        case CONTEXT_FATOR:
            printf("  - Deve ser: número, identificador ou (expressão)\n");
            break;
    }
}

// Enhanced comando function with better error detection
int comando_enhanced() {
    TokenType sync_comando[] = {TOKEN_SEMICOLON, TOKEN_END, TOKEN_ELSE, TOKEN_DOT};
    push_context_enhanced(CONTEXT_COMANDO, "COMANDO", sync_comando, 4);
    int result = 1;
    
    if (current_token.tipo == TOKEN_IDENTIFIER) {
        // Verificar se é uma palavra-chave mal digitada
        if (strcmp(current_token.lexema, "EN") == 0) {
            syntax_error_with_hint("Token inválido 'EN' - possível erro de digitação", 
                                 "Verifique se era para ser 'END'");
            pop_context_enhanced();
            return 0;
        }
        
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
                pop_context_enhanced();
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
        // Comando composto - não verificar múltiplos BEGINs aqui, pois é contexto válido
        TokenType sync_composto[] = {TOKEN_END, TOKEN_SEMICOLON, TOKEN_EOF};
        push_context_enhanced(CONTEXT_COMANDO_COMPOSTO, "COMANDO_COMPOSTO", sync_composto, 3);
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
                // Ponto e vírgula antes de END é permitido mas desnecessário
                if (show_hints) {
                    printf("💡 \033[1;33mAviso:\033[0m Ponto e vírgula desnecessário antes de 'END'\n");
                }
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
        
        // Verificar se há comando sem ';' antes do próximo comando
        if (current_token.tipo != TOKEN_END && current_token.tipo != TOKEN_SEMICOLON && 
            current_token.tipo != TOKEN_EOF && current_token.tipo != TOKEN_DOT) {
            
            // Verificar se é um identificador (possível próximo comando)
            if (current_token.tipo == TOKEN_IDENTIFIER) {
                syntax_error_with_hint("Esperado ';' entre comandos", 
                                     "Comandos devem ser separados por ponto e vírgula");
                // Continuar análise assumindo que o ';' foi esquecido
                if (comando_enhanced()) {
                    comandos_processados++;
                }
                
                // Após processar comando sem ';', verificar novamente por ';'
                while (current_token.tipo == TOKEN_SEMICOLON) {
                    eat_advanced(TOKEN_SEMICOLON, CONTEXT_COMANDO_COMPOSTO);
                    if (current_token.tipo == TOKEN_END) {
                        break;
                    }
                    if (comando_enhanced()) {
                        comandos_processados++;
                    } else {
                        break;
                    }
                }
            }
        }
        
        // Verificar se há comando sem ';' antes do próximo comando
        
        
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
        pop_context_enhanced();

    } else if (current_token.tipo == TOKEN_IF) {
        TokenType sync_if[] = {TOKEN_THEN, TOKEN_ELSE, TOKEN_SEMICOLON, TOKEN_END};
        push_context_enhanced(CONTEXT_COMANDO_IF, "COMANDO_IF", sync_if, 4);
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
        pop_context_enhanced();
    } else if (current_token.tipo == TOKEN_WHILE) {
        TokenType sync_while[] = {TOKEN_DO, TOKEN_SEMICOLON, TOKEN_END};
        push_context_enhanced(CONTEXT_COMANDO_WHILE, "COMANDO_WHILE", sync_while, 3);
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
        pop_context_enhanced();
    } else if (current_token.tipo == TOKEN_FOR) {
        TokenType sync_for[] = {TOKEN_DO, TOKEN_SEMICOLON, TOKEN_END};
        push_context_enhanced(CONTEXT_COMANDO_FOR, "COMANDO_FOR", sync_for, 3);
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
        pop_context_enhanced();
    } else if (current_token.tipo == TOKEN_END || current_token.tipo == TOKEN_ELSE || 
               current_token.tipo == TOKEN_DOT || current_token.tipo == TOKEN_SEMICOLON) {
        // comando vazio: epsilon - isso é válido
        result = 1; // Comando vazio é válido em alguns contextos
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
    
    pop_context_enhanced();
    return result;
}

void condicao() {
    TokenType sync_condicao[] = {TOKEN_THEN, TOKEN_DO, TOKEN_SEMICOLON};
    push_context_enhanced(CONTEXT_CONDICAO, "CONDICAO", sync_condicao, 3);
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
    pop_context_enhanced();
}

void expressao() {
    TokenType sync_expressao[] = {TOKEN_SEMICOLON, TOKEN_THEN, TOKEN_DO, TOKEN_RPAREN, TOKEN_COMMA};
    push_context_enhanced(CONTEXT_EXPRESSAO, "EXPRESSAO", sync_expressao, 5);
    if (current_token.tipo == TOKEN_PLUS || current_token.tipo == TOKEN_MINUS) {
        eat_advanced(current_token.tipo, CONTEXT_EXPRESSAO);
    }
    termo();
    while (current_token.tipo == TOKEN_PLUS || current_token.tipo == TOKEN_MINUS) {
        eat_advanced(current_token.tipo, CONTEXT_EXPRESSAO);
        termo();
    }
    pop_context_enhanced();
}

void termo() {
    TokenType sync_termo[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_SEMICOLON, TOKEN_THEN, TOKEN_DO, TOKEN_RPAREN};
    push_context_enhanced(CONTEXT_TERMO, "TERMO", sync_termo, 6);
    fator();
    while (current_token.tipo == TOKEN_MULT || current_token.tipo == TOKEN_DIV) {
        eat_advanced(current_token.tipo, CONTEXT_TERMO);
        fator();
    }
    pop_context_enhanced();
}

void fator() {
    TokenType sync_fator[] = {TOKEN_MULT, TOKEN_DIV, TOKEN_PLUS, TOKEN_MINUS, TOKEN_SEMICOLON, TOKEN_RPAREN};
    push_context_enhanced(CONTEXT_FATOR, "FATOR", sync_fator, 6);
    
    if (current_token.tipo == TOKEN_IDENTIFIER) {
        eat_advanced(TOKEN_IDENTIFIER, CONTEXT_FATOR);
    } else if (current_token.tipo == TOKEN_NUMBER) {
        eat_advanced(TOKEN_NUMBER, CONTEXT_FATOR);
    } else if (current_token.tipo == TOKEN_LPAREN) {
        eat_advanced(TOKEN_LPAREN, CONTEXT_FATOR);
        expressao();
        if (current_token.tipo != TOKEN_RPAREN) {
            syntax_error_with_hint("Esperado ')' para fechar expressão", 
                                 "Parênteses devem estar balanceados");
        } else {
            eat_advanced(TOKEN_RPAREN, CONTEXT_FATOR);
        }
    } else if (current_token.tipo == TOKEN_ERROR_LEXICO) {
        // Tratar erro léxico explicitamente
        lexico_error("Token inválido encontrado na expressão");
        advanced_synchronize();        } else {
            if (show_hints) {
                const char* dica = NULL;
                if (current_token.tipo == TOKEN_RPAREN) {
                    dica = "Expressão vazia dentro dos parênteses";
                } else if (current_token.tipo == TOKEN_SEMICOLON) {
                    dica = "Expressão incompleta antes do ';'";
                } else if (current_token.tipo == TOKEN_END) {
                    dica = "Expressão incompleta antes do 'END'";
                } else if (strlen(current_token.lexema) > 0) {
                    // Verificar erros de digitação comuns
                    if (strcmp(current_token.lexema, "EN") == 0) {
                        dica = "Possível erro de digitação: 'EN' deveria ser 'END'";
                    } else if (strcmp(current_token.lexema, "BEGI") == 0) {
                        dica = "Possível erro de digitação: 'BEGI' deveria ser 'BEGIN'";
                    } else if (strcmp(current_token.lexema, "PROCEDUR") == 0) {
                        dica = "Possível erro de digitação: 'PROCEDUR' deveria ser 'PROCEDURE'";
                    } else {
                        // Verificar se contém caracteres inválidos
                        int has_invalid = 0;
                        for (int i = 0; current_token.lexema[i]; i++) {
                            char c = current_token.lexema[i];
                            if (c == '#' || c == '@' || c == '$' || c == '%' || c == '&' || c == '!') {
                                has_invalid = 1;
                                break;
                            }
                        }
                        if (has_invalid) {
                            dica = "Caractere inválido na expressão";
                        }
                    }
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
    pop_context_enhanced();
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
    // Initialize statistics and print banner
    init_statistics();
    start_parsing_timer();
    
    if (show_hints) {
        print_banner();
        printf(BOLD CYAN SYMBOL_GEAR " Iniciando análise..." RESET_COLOR "\n\n");
    }
    
    current_token = get_next_token();
    if (current_token.tipo == TOKEN_ERROR_LEXICO) {
        lexico_error("Erro léxico detectado no primeiro token");
    }
    
    // Update token statistics
    update_token_stats(current_token.tipo);
    
    programa_enhanced();
    
    if (current_token.tipo != TOKEN_EOF) {
        syntax_error("esperado fim de arquivo");
    }
    
    // End timing and print results
    end_parsing_timer();
    stats.total_lines = linha_num;
    
    if (show_hints) {
        print_detailed_statistics();
        print_performance_metrics();
        print_code_quality_metrics();
    }
    
    if (num_erros_sintaticos == 0 && num_erros_lexicos == 0) {
        if (show_hints) {
            printf("\n");
            PRINT_SUCCESS("Análise concluída sem erros!");
            print_summary_box(0, 0, stats.parsing_time);
        } else {
            printf("\033[1;32mAnálise concluída sem erros.\033[0m\n");
        }
    } else {
        if (show_hints) {
            print_summary_box(num_erros_sintaticos + num_erros_lexicos, 0, stats.parsing_time);
        }
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
