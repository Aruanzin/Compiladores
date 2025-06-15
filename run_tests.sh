#!/bin/bash
clear
make all
shopt -s globstar nullglob
for file in tests/**/*.txt; do
  echo "==== Testando $file ===="
  ./pl0_lexer "$file"
  echo
done
