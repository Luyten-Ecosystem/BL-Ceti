
#ifndef BL_CETI_LEXER_H
#define BL_CETI_LEXER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    BL_EOF,
    BL_IDENTIFIER,
    BL_INTEGER,
    BL_FLOAT,
    BL_STRING,

    BL_FN, BL_LET, BL_VAR,
    BL_IF, BL_ELSE, BL_WHILE,
    BL_BREAK, BL_CONTINUE, BL_RETURN,
    BL_TRUE, BL_FALSE, BL_AS,

    BL_INT_TYPE, BL_FLOAT_TYPE,
    BL_BOOL_TYPE, BL_STRING_TYPE, BL_VOID_TYPE,

    BL_RESERVED,

    BL_LPAREN, BL_RPAREN,
    BL_LBRACE, BL_RBRACE,
    BL_COMMA, BL_COLON, BL_SEMICOLON,
    BL_ARROW,

    BL_PLUS, BL_MINUS, BL_STAR, BL_SLASH, BL_PERCENT,
    BL_ASSIGN, BL_PLUS_ASSIGN, BL_MINUS_ASSIGN,
    BL_STAR_ASSIGN, BL_SLASH_ASSIGN, BL_PERCENT_ASSIGN,

    BL_EQUAL, BL_NOT_EQUAL,
    BL_LESS, BL_LESS_EQUAL,
    BL_GREATER, BL_GREATER_EQUAL,

    BL_AND, BL_OR, BL_NOT
} BlTokenType;

typedef struct {
    BlTokenType type;
    const char *text;
    size_t length;
    size_t line;
    size_t column;
} BlToken;

typedef struct {
    char code[6];
    char message[160];
    size_t line;
    size_t column;
} BlLexError;

typedef struct {
    BlToken *tokens;
    size_t count;
    size_t capacity;
    BlLexError error;
    bool failed;
} BlLexResult;

bool bl_lex(const char *source, size_t length, BlLexResult *result);
void bl_lex_result_destroy(BlLexResult *result);
const char *bl_token_type_name(BlTokenType type);

#endif
