# 🖥️ IDE Visual PL/0 - Documentação

## Visão Geral

A IDE Visual PL/0 é uma ferramenta avançada de visualização e navegação de erros para o compilador PL/0. Ela oferece uma interface visual rica para identificar, entender e corrigir erros em código PL/0.

## 🚀 Como Usar

### Modo Básico
```bash
./pl0_lexer arquivo.pl0
```

### Modo IDE Visual
```bash
./pl0_lexer --ide arquivo.pl0
```

### Outros Modos
```bash
./pl0_lexer --hint arquivo.pl0          # Com dicas coloridas
./pl0_lexer --interactive arquivo.pl0   # Modo interativo
./pl0_lexer --help                      # Ajuda completa
```

## 🎨 Funcionalidades da IDE

### 1. **Cabeçalho Visual**
- Nome do arquivo sendo analisado
- Contagem de linhas e erros
- Design visual atrativo com bordas Unicode

### 2. **Visualização do Código com Destaque**
- Numeração de linhas
- Destaque de sintaxe colorido:
  - 🔵 **Azul**: Palavras-chave (BEGIN, END, IF, WHILE, etc.)
  - 🟡 **Amarelo**: Números
  - 🟣 **Magenta**: Operadores e símbolos
  - ⚪ **Branco**: Identificadores
  - ⚫ **Cinza**: Comentários
- Marcação visual de erros com símbolos `▶` e `▲`

### 3. **Destaque de Erros**
- **Ponteiro Visual**: Setas apontando exatamente onde o erro ocorreu
- **Código de Cores**: 
  - 🔴 **Vermelho**: Erros críticos
  - 🟡 **Amarelo**: Avisos
  - 🔵 **Azul**: Informações
- **Contexto**: Mostra as linhas ao redor do erro

### 4. **Resumo de Erros**
- Tabela organizada com todos os erros
- Categorização por tipo (LÉXICO, SINTÁTICO, SEMÂNTICO)
- Numeração para navegação rápida

### 5. **Navegação Interativa**
Comandos disponíveis durante a navegação:

- `n` ou `N` - Próximo erro
- `p` ou `P` - Erro anterior  
- `1-9` - Ir para erro específico (1º, 2º, etc.)
- `s` ou `S` - Mostrar resumo de erros
- `c` ou `C` - Mostrar código completo novamente
- `h` ou `H` - Mostrar ajuda
- `q` ou `Q` - Sair da navegação

### 6. **Contexto do Erro**
Para cada erro, a IDE mostra:
- **Linha e Coluna** exatas
- **Tipo de Erro** (ERROR, WARNING, INFO)
- **Categoria** (LÉXICO, SINTÁTICO, SEMÂNTICO)
- **Mensagem Descritiva**
- **Contexto Visual** com linhas ao redor

## 📋 Tipos de Erros Suportados

### Erros Léxicos
- Caracteres inválidos (ex: `#`, `@`, `%`)
- Números mal formados (ex: `3.14159`)
- Tokens não reconhecidos

### Erros Sintáticos
- Estruturas de controle mal formadas
- Falta de símbolos obrigatórios (`;`, `.`, etc.)
- Palavras-chave incorretas (ex: `TEN` em vez de `THEN`)
- Ordem incorreta de comandos

### Avisos
- Ponto e vírgula desnecessário
- Comandos sem terminação adequada
- Práticas não recomendadas

## 🎯 Exemplos de Uso

### Exemplo 1: Erro Léxico
```pl0
VAR x;
BEGIN
  x := 3 + #;   { Caractere inválido }
END.
```

**Saída da IDE:**
```
📝 CÓDIGO FONTE COM DESTAQUE DE ERROS:
│   3 ▶   x := 3 + #;   { Caractere inválido }
│                ▲
│                │
│                └─ LÉXICO: Caractere inválido '#'
```

### Exemplo 2: Erro Sintático
```pl0
VAR x;
BEGIN
  IF x > 10 TEN    { THEN escrito errado }
    x := 5
END.
```

**Saída da IDE:**
```
📝 CÓDIGO FONTE COM DESTAQUE DE ERROS:
│   3 ▶   IF x > 10 TEN
│                   ▲
│                   │
│                   └─ SINTÁTICO: Esperado 'THEN' após condição IF
```

## 🔧 Recursos Avançados

### Integração com Estatísticas
A IDE funciona em conjunto com o sistema de estatísticas, mostrando:
- Densidade de erros por linha
- Tipos de erros mais comuns
- Métricas de qualidade do código

### Sugestões Inteligentes
- Correções automáticas para erros comuns
- Fuzzy matching para palavras-chave
- Dicas contextuais baseadas no tipo de erro

### Recuperação de Erros
- Sincronização automática após erros
- Continuação da análise para encontrar mais erros
- Relatório de recuperações bem-sucedidas

## 💡 Dicas de Uso

1. **Use sempre `--ide`** para análise visual de erros
2. **Navegue sequencialmente** pelos erros com `n` e `p`
3. **Use números** (1-9) para ir direto a erros específicos
4. **Consulte o resumo** (`s`) para visão geral
5. **Use `c`** para rever o código completo

## 🚨 Limitações Conhecidas

- Máximo de 100 erros por arquivo
- Navegação limitada aos primeiros 9 erros via teclado numérico
- Funciona melhor em terminais com suporte a cores Unicode

## 🔮 Futuras Melhorias

- [ ] Exportação de relatórios em HTML
- [ ] Suporte a múltiplos arquivos
- [ ] Integração com editores externos
- [ ] Modo de correção automática
- [ ] Plugins para IDEs populares

---

**Desenvolvido com 💚 para a disciplina de Compiladores**
