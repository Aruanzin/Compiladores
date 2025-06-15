#include <stdlib.h>
#include <string.h>
#include "utils.h"

// Helper function for string duplication
char* string_duplicate(const char* str) {
    int len = strlen(str);
    char* copy = malloc(len + 1);
    strcpy(copy, str);
    return copy;
}
