#ifndef COLORS_H
#define COLORS_H

#define RESET_COLOR    "\033[0m"
#define BOLD           "\033[1m"
#define DIM            "\033[2m"
#define ITALIC         "\033[3m"
#define UNDERLINE      "\033[4m"

#define BLACK          "\033[30m"
#define RED            "\033[31m"
#define GREEN          "\033[32m"
#define YELLOW         "\033[33m"
#define BLUE           "\033[34m"
#define MAGENTA        "\033[35m"
#define CYAN           "\033[36m"
#define WHITE          "\033[37m"

#define BRIGHT_BLACK   "\033[90m"
#define BRIGHT_RED     "\033[91m"
#define BRIGHT_GREEN   "\033[92m"
#define BRIGHT_YELLOW  "\033[93m"
#define BRIGHT_BLUE    "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN    "\033[96m"
#define BRIGHT_WHITE   "\033[97m"

#define BG_BLACK       "\033[40m"
#define BG_RED         "\033[41m"
#define BG_GREEN       "\033[42m"
#define BG_YELLOW      "\033[43m"
#define BG_BLUE        "\033[44m"
#define BG_MAGENTA     "\033[45m"
#define BG_CYAN        "\033[46m"
#define BG_WHITE       "\033[47m"

#define SYMBOL_ERROR   "🚨"
#define SYMBOL_WARNING "⚠️"
#define SYMBOL_INFO    "💡"
#define SYMBOL_SUCCESS "✅"
#define SYMBOL_HINT    "🔍"
#define SYMBOL_CODE    "📄"
#define SYMBOL_ARROW   "→"
#define SYMBOL_CHECK   "✓"
#define SYMBOL_CROSS   "✗"
#define SYMBOL_GEAR    "⚙️"
#define SYMBOL_MAGIC   "✨"
#define SYMBOL_BOOK    "📖"
#define SYMBOL_FIRE    "🔥"
#define SYMBOL_ROCKET  "🚀"

#define PRINT_ERROR(msg) printf(BOLD RED SYMBOL_ERROR " ERRO: " RESET_COLOR msg "\n")
#define PRINT_WARNING(msg) printf(BOLD YELLOW SYMBOL_WARNING " AVISO: " RESET_COLOR msg "\n")
#define PRINT_SUCCESS(msg) printf(BOLD GREEN SYMBOL_SUCCESS " SUCESSO: " RESET_COLOR msg "\n")
#define PRINT_INFO(msg) printf(BOLD CYAN SYMBOL_INFO " INFO: " RESET_COLOR msg "\n")

#endif // COLORS_H
