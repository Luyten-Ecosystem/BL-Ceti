#ifndef BL_CETI_PARSER_H
#define BL_CETI_PARSER_H

#include "lexer.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char code[6];
    char message[160];
    size_t line;
    size_t column;
} BlParseError;

bool bl_parse(const BlToken *tokens, size_t count, BlParseError *error);

#endif
