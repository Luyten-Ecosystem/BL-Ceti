
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char *word;
    BlTokenType type;
} Keyword;

static const Keyword keywords[] = {
    {"fn", BL_FN}, {"let", BL_LET}, {"var", BL_VAR},
    {"if", BL_IF}, {"else", BL_ELSE}, {"while", BL_WHILE},
    {"break", BL_BREAK}, {"continue", BL_CONTINUE},
    {"return", BL_RETURN}, {"true", BL_TRUE},
    {"false", BL_FALSE}, {"as", BL_AS},
    {"Int", BL_INT_TYPE}, {"Float", BL_FLOAT_TYPE},
    {"Bool", BL_BOOL_TYPE}, {"String", BL_STRING_TYPE},
    {"Void", BL_VOID_TYPE}
};

static const char *const token_names[] = {
    "EOF", "identifier", "integer", "float", "string",
    "fn", "let", "var", "if", "else", "while",
    "break", "continue", "return", "true", "false", "as",
    "Int", "Float", "Bool", "String", "Void", "reserved",
    "(", ")", "{", "}", ",", ":", ";", "->",
    "+", "-", "*", "/", "%",
    "=", "+=", "-=", "*=", "/=", "%=",
    "==", "!=", "<", "<=", ">", ">=", "&&", "||", "!"
};

typedef struct {
    const char *source;
    size_t length;
    size_t position;
    size_t line;
    size_t column;
    BlLexResult *result;
} Lexer;

static char peek(const Lexer *lexer)
{
    if (lexer->position >= lexer->length) return '\0';
    return lexer->source[lexer->position];
}

static char peek_next(const Lexer *lexer)
{
    if (lexer->position + 1 >= lexer->length) return '\0';
    return lexer->source[lexer->position + 1];
}

static char advance(Lexer *lexer)
{
    char c = peek(lexer);

    if (lexer->position < lexer->length) {
        lexer->position++;
        if (c == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
    }

    return c;
}

static bool fail(Lexer *lexer, const char *code, const char *message,
                 size_t line, size_t column)
{
    lexer->result->failed = true;
    snprintf(lexer->result->error.code,
             sizeof lexer->result->error.code, "%s", code);
    snprintf(lexer->result->error.message,
             sizeof lexer->result->error.message, "%s", message);
    lexer->result->error.line = line;
    lexer->result->error.column = column;
    return false;
}

static bool add_token(Lexer *lexer, BlTokenType type,
                      size_t start, size_t line, size_t column)
{
    BlLexResult *result = lexer->result;

    if (result->count == result->capacity) {
        size_t capacity = result->capacity == 0 ? 64 : result->capacity * 2;

        if (capacity < result->capacity ||
            capacity > (size_t)-1 / sizeof *result->tokens) {
            return fail(lexer, "E0001", "Memoria insuficiente.",
                        line, column);
            }

            BlToken *tokens = realloc(result->tokens,
                                      capacity * sizeof *tokens);
            if (tokens == NULL) {
                return fail(lexer, "E0001", "Memoria insuficiente.",
                            line, column);
            }

            result->tokens = tokens;
            result->capacity = capacity;
    }

    result->tokens[result->count++] = (BlToken) {
        .type = type,
        .text = lexer->source + start,
        .length = lexer->position - start,
        .line = line,
        .column = column
    };

    return true;
}

static bool is_identifier_start(char c)
{
    return (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_identifier_part(char c)
{
    return is_identifier_start(c) || (c >= '0' && c <= '9');
}

static BlTokenType keyword_type(const char *word, size_t length)
{
    for (size_t i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        if (strlen(keywords[i].word) == length &&
            memcmp(word, keywords[i].word, length) == 0) {
            return keywords[i].type;
            }
    }

    static const char *const reserved[] = {
        "import", "module", "pub", "struct", "enum", "for",
        "in", "loop", "match", "const", "unsafe", "extern",
        "type", "impl", "trait", "static", "_"
    };

    for (size_t i = 0; i < sizeof reserved / sizeof reserved[0]; i++) {
        if (strlen(reserved[i]) == length &&
            memcmp(word, reserved[i], length) == 0) {
            return BL_RESERVED;
            }
    }

    return BL_IDENTIFIER;
}

static bool scan_number(Lexer *lexer, size_t start,
                        size_t line, size_t column)
{
    bool floating = false;

    while (isdigit((unsigned char)peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    if (peek(lexer) == '.' && isdigit((unsigned char)peek_next(lexer))) {
        floating = true;
        advance(lexer);

        while (isdigit((unsigned char)peek(lexer)) || peek(lexer) == '_') {
            advance(lexer);
        }
    }

    if (peek(lexer) == 'e' || peek(lexer) == 'E') {
        floating = true;
        advance(lexer);

        if (peek(lexer) == '+' || peek(lexer) == '-') advance(lexer);

        if (!isdigit((unsigned char)peek(lexer))) {
            return fail(lexer, "E0006", "Exponente numerico invalido.",
                        line, column);
        }

        while (isdigit((unsigned char)peek(lexer)) || peek(lexer) == '_') {
            advance(lexer);
        }
    }

    if (is_identifier_start(peek(lexer))) {
        while (is_identifier_part(peek(lexer))) advance(lexer);
        return fail(lexer, "E0006", "Literal numerico mal formado.",
                    line, column);
    }

    return add_token(lexer, floating ? BL_FLOAT : BL_INTEGER,
                     start, line, column);
}

static bool scan_string(Lexer *lexer, size_t start,
                        size_t line, size_t column)
{
    while (peek(lexer) != '"' && peek(lexer) != '\0') {
        if (peek(lexer) == '\n') {
            return fail(lexer, "E0004",
                        "Cadena sin cerrar antes del salto de linea.",
                        line, column);
        }

        if (peek(lexer) == '\\') {
            advance(lexer);
            char escaped = peek(lexer);

            if (escaped == '\0') break;

            if (escaped == 'x') {
                advance(lexer);
                for (int i = 0; i < 2; i++) {
                    if (!isxdigit((unsigned char)peek(lexer))) {
                        return fail(lexer, "E0005",
                                    "Escape hexadecimal invalido.",
                                    lexer->line, lexer->column);
                    }
                    advance(lexer);
                }
                continue;
            }

            if (escaped == 'u') {
                advance(lexer);
                if (peek(lexer) != '{') {
                    return fail(lexer, "E0005",
                                "Escape Unicode invalido.",
                                lexer->line, lexer->column);
                }
                advance(lexer);

                size_t digits = 0;
                while (isxdigit((unsigned char)peek(lexer))) {
                    advance(lexer);
                    digits++;
                }

                if (digits == 0 || peek(lexer) != '}') {
                    return fail(lexer, "E0005",
                                "Escape Unicode invalido.",
                                lexer->line, lexer->column);
                }
                advance(lexer);
                continue;
            }

            if (escaped != 'n' && escaped != 'r' &&
                escaped != 't' && escaped != '0' &&
                escaped != '\\' && escaped != '"') {
                return fail(lexer, "E0005", "Escape desconocido.",
                            lexer->line, lexer->column);
                }

                advance(lexer);
            continue;
        }

        advance(lexer);
    }

    if (peek(lexer) != '"') {
        return fail(lexer, "E0004", "Cadena sin cerrar.", line, column);
    }

    advance(lexer);
    return add_token(lexer, BL_STRING, start, line, column);
}

static bool scan_token(Lexer *lexer)
{
    size_t start = lexer->position;
    size_t line = lexer->line;
    size_t column = lexer->column;
    char c = advance(lexer);

    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return true;

    if (c == '/' && peek(lexer) == '/') {
        while (peek(lexer) != '\n' && peek(lexer) != '\0') advance(lexer);
        return true;
    }

    if (c == '/' && peek(lexer) == '*') {
        advance(lexer);
        size_t depth = 1;

        while (depth > 0 && peek(lexer) != '\0') {
            if (peek(lexer) == '/' && peek_next(lexer) == '*') {
                advance(lexer);
                advance(lexer);
                depth++;
            } else if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                advance(lexer);
                advance(lexer);
                depth--;
            } else {
                advance(lexer);
            }
        }

        if (depth != 0) {
            return fail(lexer, "E0003", "Comentario sin cerrar.",
                        line, column);
        }

        return true;
    }

    if (is_identifier_start(c)) {
        while (is_identifier_part(peek(lexer))) advance(lexer);

        BlTokenType type = keyword_type(lexer->source + start,
                                        lexer->position - start);

        if (type == BL_RESERVED) {
            return fail(lexer, "E1002",
                        "Palabra reservada no disponible en BL-Ceti 0.1.0.",
                        line, column);
        }

        return add_token(lexer, type, start, line, column);
    }

    if (isdigit((unsigned char)c)) {
        if (c == '0' && (peek(lexer) == 'x' || peek(lexer) == 'X' ||
            peek(lexer) == 'b' || peek(lexer) == 'B')) {
            char base = advance(lexer);
        bool hexadecimal = base == 'x' || base == 'X';
        size_t digits = 0;

        while (isxdigit((unsigned char)peek(lexer)) ||
            (hexadecimal && peek(lexer) == '_')) {
            if (peek(lexer) == '_' && !hexadecimal) break;
            if (peek(lexer) != '_') digits++;
            advance(lexer);
            }

            if (digits == 0) {
                return fail(lexer, "E0006",
                            "Literal hexadecimal o binario invalido.",
                            line, column);
            }

            return add_token(lexer, BL_INTEGER, start, line, column);
            }

            if (c == '0' && isdigit((unsigned char)peek(lexer))) {
                return fail(lexer, "E0006",
                            "Un entero decimal no puede comenzar con cero.",
                            line, column);
            }

            return scan_number(lexer, start, line, column);
    }

    if (c == '"') return scan_string(lexer, start, line, column);

    #define ONE(ch, token) \
    case ch: return add_token(lexer, token, start, line, column)

        switch (c) {
            ONE('(', BL_LPAREN);
            ONE(')', BL_RPAREN);
            ONE('{', BL_LBRACE);
            ONE('}', BL_RBRACE);
            ONE(',', BL_COMMA);
            ONE(':', BL_COLON);
            ONE(';', BL_SEMICOLON);

            case '+':
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return add_token(lexer, BL_PLUS_ASSIGN, start, line, column);
                }
                return add_token(lexer, BL_PLUS, start, line, column);

            case '-':
                if (peek(lexer) == '>') {
                    advance(lexer);
                    return add_token(lexer, BL_ARROW, start, line, column);
                }
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return add_token(lexer, BL_MINUS_ASSIGN, start, line, column);
                }
                return add_token(lexer, BL_MINUS, start, line, column);

            case '*':
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return add_token(lexer, BL_STAR_ASSIGN, start, line, column);
                }
                return add_token(lexer, BL_STAR, start, line, column);

            case '/':
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return add_token(lexer, BL_SLASH_ASSIGN, start, line, column);
                }
                return add_token(lexer, BL_SLASH, start, line, column);

            case '%':
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return add_token(lexer, BL_PERCENT_ASSIGN, start, line, column);
                }
                return add_token(lexer, BL_PERCENT, start, line, column);

            case '=':
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return add_token(lexer, BL_EQUAL, start, line, column);
                }
                return add_token(lexer, BL_ASSIGN, start, line, column);

            case '!':
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return add_token(lexer, BL_NOT_EQUAL, start, line, column);
                }
                return add_token(lexer, BL_NOT, start, line, column);

            case '<':
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return add_token(lexer, BL_LESS_EQUAL, start, line, column);
                }
                return add_token(lexer, BL_LESS, start, line, column);

            case '>':
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return add_token(lexer, BL_GREATER_EQUAL, start, line, column);
                }
                return add_token(lexer, BL_GREATER, start, line, column);

            case '&':
                if (peek(lexer) == '&') {
                    advance(lexer);
                    return add_token(lexer, BL_AND, start, line, column);
                }
                break;

            case '|':
                if (peek(lexer) == '|') {
                    advance(lexer);
                    return add_token(lexer, BL_OR, start, line, column);
                }
                break;

            default:
                break;
        }

        #undef ONE

        return fail(lexer, "E0002", "Caracter no reconocido.",
                    line, column);
}

bool bl_lex(const char *source, size_t length, BlLexResult *result)
{
    if (source == NULL || result == NULL) return false;

    memset(result, 0, sizeof *result);

    Lexer lexer = {
        .source = source,
        .length = length,
        .position = 0,
        .line = 1,
        .column = 1,
        .result = result
    };

    if (length >= 3 &&
        (unsigned char)source[0] == 0xEF &&
        (unsigned char)source[1] == 0xBB &&
        (unsigned char)source[2] == 0xBF) {
        lexer.position = 3;
        }

        while (lexer.position < lexer.length && !result->failed) {
            if (!scan_token(&lexer)) break;
        }

        if (!result->failed) {
            if (result->count == result->capacity) {
                size_t capacity = result->capacity == 0 ? 1 : result->capacity + 1;
                BlToken *tokens = realloc(result->tokens,
                                          capacity * sizeof *tokens);
                if (tokens == NULL) {
                    return fail(&lexer, "E0001", "Memoria insuficiente.",
                                lexer.line, lexer.column);
                }
                result->tokens = tokens;
                result->capacity = capacity;
            }

            result->tokens[result->count++] = (BlToken) {
                .type = BL_EOF,
                .text = source + length,
                .length = 0,
                .line = lexer.line,
                .column = lexer.column
            };
        }

        return !result->failed;
}

void bl_lex_result_destroy(BlLexResult *result)
{
    if (result == NULL) return;

    free(result->tokens);
    memset(result, 0, sizeof *result);
}

const char *bl_token_type_name(BlTokenType type)
{
    size_t count = sizeof token_names / sizeof token_names[0];

    if ((size_t)type >= count) return "unknown";
    return token_names[type];
}
