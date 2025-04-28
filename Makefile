all:
	gcc -o pl0_lexer src/*.c -Wall -g -lm

run: all
	./pl0_lexer

debug:
	gdb ./pl0_lexer

# Executar um teste específico
test_file:
	@echo "Executando teste com arquivo específico"
	@read -p "Digite o nome do arquivo de teste: " test_file; \
	echo $$test_file | ./pl0_lexer > output/resultado_atual.txt; \
	echo "Resultado salvo em output/resultado_atual.txt"

# Executar teste automático com test1.txt
test1: all
	@echo "Executando teste com test1.txt"
	@mkdir -p output
	@echo "tests/test1.txt" | ./pl0_lexer > output/test1_resultado.txt
	@echo "Resultado salvo em output/test1_resultado.txt"

# Executar teste automático com test2.txt
test2: all
	@echo "Executando teste com test2.txt"
	@mkdir -p output
	@echo "tests/test2.txt" | ./pl0_lexer > output/test2_resultado.txt
	@if diff -q output/test2_resultado.txt tests/test2.out > /dev/null; then \
		echo "TESTE PASSOU! Resultado corresponde ao esperado."; \
	else \
		echo "TESTE FALHOU! Veja as diferenças abaixo:"; \
		diff -y output/test2_resultado.txt tests/test2.out; \
	fi

test3: all
	@echo "Executando teste com test3.txt"
	@mkdir -p output
	@echo "tests/test3.txt" | ./pl0_lexer > output/test3_resultado.txt
	@echo "Resultado salvo em output/test3_resultado.txt"

# Executar todos os testes disponíveis
test_all: test1 test2
	@echo "Todos os testes foram executados"

# Adicionar um novo arquivo de teste
new_test:
	@echo "Criando novo arquivo de teste"
	@read -p "Digite o nome para o novo teste (sem extensão): " test_name; \
	read -p "Digite o conteúdo do teste (termine com Ctrl+D): " test_content; \
	echo "$$test_content" > tests/$$test_name.txt; \
	echo "Arquivo de teste tests/$$test_name.txt criado"

valgrind:
	valgrind --leak-check=full ./pl0_lexer

clean:
	rm -f pl0_lexer
	rm -f output/*.txt
