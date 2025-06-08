#!/bin/bash
clear
make all
for file in tests/*.txt; do
  echo "==== Testando $file ===="
  ./pl0_lexer "$file"
  echo ""
done
