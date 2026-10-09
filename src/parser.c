#include "parser.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    const BlToken *tokens;
    size_t count;
    size_t position;
    BlParseError *error;
    bool failed;
} Parser;

typedef struct {
    bool identifier;
    bool call;
} ExprInfo;

static const BlToken *current(Parser *parser)
{
    if (parser->position >= parser->count) {
        return &parser->tokens[parser->count - 1];
    }

    return &parser->tokens[parser->position];
}

static bool check(Parser *parser, BlTokenType type)
{
    return current(parser)->type == type;
}

static const BlToken *advance(Parser *parser)
{
    const BlToken *token = current(parser);

    if (token->type != BL_EOF) {
        parser->position++;
    }

    return token;
}

static bool fail_at(Parser *parser, const BlToken *token,
                    const char *code, const char *message)
{
    if (!parser->failed) {
        parser->failed = true;

        snprintf(parser->error->code, sizeof parser->error->code,
                 "%s", code);
        snprintf(parser->error->message, sizeof parser->error->message,
                 "%s", message);

        parser->error->line = token->line;
        parser->error->column = token->column;
    }

    return false;
}

static bool expect(Parser *parser, BlTokenType type,
                   const char *message)
{
    if (check(parser, type)) {
        advance(parser);
        return true;
    }

    return fail_at(parser, current(parser), "E1001", message);
}

static bool parse_expression(Parser *parser, ExprInfo *info);
static bool parse_statement(Parser *parser);
static bool parse_block(Parser *parser);

static bool parse_type(Parser *parser, bool allow_void)
{
    BlTokenType type = current(parser)->type;

    if (type == BL_INT_TYPE ||
        type == BL_FLOAT_TYPE ||
        type == BL_BOOL_TYPE ||
        type == BL_STRING_TYPE) {
        advance(parser);
    return true;
        }

        if (allow_void && type == BL_VOID_TYPE) {
            advance(parser);
            return true;
        }

        return fail_at(parser, current(parser), "E1001",
                       "Se esperaba un tipo valido.");
}

static bool parse_primary(Parser *parser, ExprInfo *info)
{
    const BlToken *token = current(parser);

    info->identifier = false;
    info->call = false;

    switch (token->type) {
        case BL_INTEGER:
        case BL_FLOAT:
        case BL_STRING:
        case BL_TRUE:
        case BL_FALSE:
            advance(parser);
            return true;

        case BL_IDENTIFIER:
            advance(parser);

            if (check(parser, BL_LPAREN)) {
                advance(parser);

                if (!check(parser, BL_RPAREN)) {
                    ExprInfo argument;

                    if (!parse_expression(parser, &argument)) {
                        return false;
                    }

                    while (check(parser, BL_COMMA)) {
                        advance(parser);

                        if (check(parser, BL_RPAREN)) {
                            break;
                        }

                        if (!parse_expression(parser, &argument)) {
                            return false;
                        }
                    }
                }

                if (!expect(parser, BL_RPAREN,
                    "Se esperaba ')' al cerrar la llamada.")) {
                    return false;
                    }

                    info->call = true;
                return true;
            }

            info->identifier = true;
            return true;

        case BL_LPAREN: {
            advance(parser);

            ExprInfo inner;

            if (!parse_expression(parser, &inner)) {
                return false;
            }

            if (!expect(parser, BL_RPAREN,
                "Se esperaba ')' en la expresion.")) {
                return false;
                }

                return true;
        }

        default:
            return fail_at(parser, token, "E1001",
                           "Se esperaba una expresion.");
    }
}

static bool parse_unary(Parser *parser, ExprInfo *info)
{
    if (check(parser, BL_MINUS) || check(parser, BL_NOT)) {
        advance(parser);

        if (!parse_unary(parser, info)) {
            return false;
        }

        info->identifier = false;
        info->call = false;
        return true;
    }

    return parse_primary(parser, info);
}

static bool parse_cast(Parser *parser, ExprInfo *info)
{
    if (!parse_unary(parser, info)) {
        return false;
    }

    while (check(parser, BL_AS)) {
        advance(parser);

        if (!parse_type(parser, false)) {
            return false;
        }

        info->identifier = false;
        info->call = false;
    }

    return true;
}

static bool parse_multiplication(Parser *parser, ExprInfo *info)
{
    if (!parse_cast(parser, info)) {
        return false;
    }

    while (check(parser, BL_STAR) ||
        check(parser, BL_SLASH) ||
        check(parser, BL_PERCENT)) {
        advance(parser);

    ExprInfo right;

    if (!parse_cast(parser, &right)) {
        return false;
    }

    info->identifier = false;
    info->call = false;
        }

        return true;
}

static bool parse_addition(Parser *parser, ExprInfo *info)
{
    if (!parse_multiplication(parser, info)) {
        return false;
    }

    while (check(parser, BL_PLUS) || check(parser, BL_MINUS)) {
        advance(parser);

        ExprInfo right;

        if (!parse_multiplication(parser, &right)) {
            return false;
        }

        info->identifier = false;
        info->call = false;
    }

    return true;
}

static bool is_relational(BlTokenType type)
{
    return type == BL_LESS ||
    type == BL_LESS_EQUAL ||
    type == BL_GREATER ||
    type == BL_GREATER_EQUAL;
}

static bool parse_relation(Parser *parser, ExprInfo *info)
{
    if (!parse_addition(parser, info)) {
        return false;
    }

    if (is_relational(current(parser)->type)) {
        advance(parser);

        ExprInfo right;

        if (!parse_addition(parser, &right)) {
            return false;
        }

        info->identifier = false;
        info->call = false;

        if (is_relational(current(parser)->type)) {
            return fail_at(parser, current(parser), "E1005",
                           "Las comparaciones relacionales no se pueden encadenar.");
        }
    }

    return true;
}

static bool parse_equality(Parser *parser, ExprInfo *info)
{
    if (!parse_relation(parser, info)) {
        return false;
    }

    if (check(parser, BL_EQUAL) || check(parser, BL_NOT_EQUAL)) {
        advance(parser);

        ExprInfo right;

        if (!parse_relation(parser, &right)) {
            return false;
        }

        info->identifier = false;
        info->call = false;

        if (check(parser, BL_EQUAL) || check(parser, BL_NOT_EQUAL)) {
            return fail_at(parser, current(parser), "E1005",
                           "Las comparaciones de igualdad no se pueden encadenar.");
        }
    }

    return true;
}

static bool parse_and(Parser *parser, ExprInfo *info)
{
    if (!parse_equality(parser, info)) {
        return false;
    }

    while (check(parser, BL_AND)) {
        advance(parser);

        ExprInfo right;

        if (!parse_equality(parser, &right)) {
            return false;
        }

        info->identifier = false;
        info->call = false;
    }

    return true;
}

static bool parse_expression(Parser *parser, ExprInfo *info)
{
    if (!parse_and(parser, info)) {
        return false;
    }

    while (check(parser, BL_OR)) {
        advance(parser);

        ExprInfo right;

        if (!parse_and(parser, &right)) {
            return false;
        }

        info->identifier = false;
        info->call = false;
    }

    return true;
}

static bool is_assignment_operator(BlTokenType type)
{
    return type == BL_ASSIGN ||
    type == BL_PLUS_ASSIGN ||
    type == BL_MINUS_ASSIGN ||
    type == BL_STAR_ASSIGN ||
    type == BL_SLASH_ASSIGN ||
    type == BL_PERCENT_ASSIGN;
}

static bool parse_variable(Parser *parser)
{
    advance(parser); /* let o var */

    if (!expect(parser, BL_IDENTIFIER,
        "Se esperaba un nombre de variable.")) {
        return false;
        }

        if (check(parser, BL_COLON)) {
            advance(parser);

            if (!parse_type(parser, false)) {
                return false;
            }
        }

        if (!expect(parser, BL_ASSIGN,
            "Se esperaba '=' para inicializar la variable.")) {
            return false;
            }

            ExprInfo value;

        if (!parse_expression(parser, &value)) {
            return false;
        }

        return expect(parser, BL_SEMICOLON,
                      "Se esperaba ';' al final de la declaracion.");
}

static bool parse_if(Parser *parser)
{
    advance(parser); /* if */

    ExprInfo condition;

    if (!parse_expression(parser, &condition)) {
        return false;
    }

    if (!parse_block(parser)) {
        return false;
    }

    if (check(parser, BL_ELSE)) {
        advance(parser);

        if (check(parser, BL_IF)) {
            return parse_if(parser);
        }

        return parse_block(parser);
    }

    return true;
}

static bool parse_while(Parser *parser)
{
    advance(parser); /* while */

    ExprInfo condition;

    if (!parse_expression(parser, &condition)) {
        return false;
    }

    return parse_block(parser);
}

static bool parse_return(Parser *parser)
{
    advance(parser); /* return */

    if (!check(parser, BL_SEMICOLON)) {
        ExprInfo value;

        if (!parse_expression(parser, &value)) {
            return false;
        }
    }

    return expect(parser, BL_SEMICOLON,
                  "Se esperaba ';' despues de return.");
}

static bool parse_simple_statement(Parser *parser)
{
    ExprInfo expression;

    if (!parse_expression(parser, &expression)) {
        return false;
    }

    if (is_assignment_operator(current(parser)->type)) {
        if (!expression.identifier) {
            return fail_at(parser, current(parser), "E1006",
                           "El lado izquierdo de una asignacion debe ser una variable.");
        }

        advance(parser);

        ExprInfo value;

        if (!parse_expression(parser, &value)) {
            return false;
        }
    } else if (!expression.call) {
        return fail_at(parser, current(parser), "E1007",
                       "Una sentencia de expresion debe ser una llamada a funcion.");
    }

    return expect(parser, BL_SEMICOLON,
                  "Se esperaba ';' al final de la sentencia.");
}

static bool parse_statement(Parser *parser)
{
    switch (current(parser)->type) {
        case BL_LET:
        case BL_VAR:
            return parse_variable(parser);

        case BL_IF:
            return parse_if(parser);

        case BL_WHILE:
            return parse_while(parser);

        case BL_BREAK:
        case BL_CONTINUE:
            advance(parser);
            return expect(parser, BL_SEMICOLON,
                          "Se esperaba ';' despues de la sentencia.");

        case BL_RETURN:
            return parse_return(parser);

        case BL_LBRACE:
            return parse_block(parser);

        case BL_IDENTIFIER:
        case BL_INTEGER:
        case BL_FLOAT:
        case BL_STRING:
        case BL_TRUE:
        case BL_FALSE:
        case BL_LPAREN:
        case BL_MINUS:
        case BL_NOT:
            return parse_simple_statement(parser);

        default:
            return fail_at(parser, current(parser), "E1001",
                           "Sentencia no valida.");
    }
}

static bool parse_block(Parser *parser)
{
    if (!expect(parser, BL_LBRACE,
        "Se esperaba '{' para comenzar el bloque.")) {
        return false;
        }

        while (!check(parser, BL_RBRACE) &&
            !check(parser, BL_EOF) &&
            !parser->failed) {
            if (!parse_statement(parser)) {
                return false;
            }
            }

            return expect(parser, BL_RBRACE,
                          "Se esperaba '}' para cerrar el bloque.");
}

static bool parse_function(Parser *parser)
{
    advance(parser); /* fn */

    if (!expect(parser, BL_IDENTIFIER,
        "Se esperaba el nombre de la funcion.")) {
        return false;
        }

        if (!expect(parser, BL_LPAREN,
            "Se esperaba '(' despues del nombre de la funcion.")) {
            return false;
            }

            if (!check(parser, BL_RPAREN)) {
                for (;;) {
                    if (!expect(parser, BL_IDENTIFIER,
                        "Se esperaba el nombre del parametro.")) {
                        return false;
                        }

                        if (!expect(parser, BL_COLON,
                            "Se esperaba ':' despues del nombre del parametro.")) {
                            return false;
                            }

                            if (!parse_type(parser, false)) {
                                return false;
                            }

                            if (!check(parser, BL_COMMA)) {
                                break;
                            }

                            advance(parser);

                            if (check(parser, BL_RPAREN)) {
                                break;
                            }
                }
            }

            if (!expect(parser, BL_RPAREN,
                "Se esperaba ')' despues de los parametros.")) {
                return false;
                }

                if (check(parser, BL_ARROW)) {
                    advance(parser);

                    if (!parse_type(parser, true)) {
                        return false;
                    }
                }

                return parse_block(parser);
}

bool bl_parse(const BlToken *tokens, size_t count, BlParseError *error)
{
    if (tokens == NULL || count == 0 || error == NULL) {
        return false;
    }

    memset(error, 0, sizeof *error);

    Parser parser = {
        .tokens = tokens,
        .count = count,
        .position = 0,
        .error = error,
        .failed = false
    };

    while (!check(&parser, BL_EOF) && !parser.failed) {
        if (!check(&parser, BL_FN)) {
            return fail_at(&parser, current(&parser), "E1001",
                           "En el nivel superior solo se permiten funciones.");
        }

        if (!parse_function(&parser)) {
            return false;
        }
    }

    return !parser.failed && check(&parser, BL_EOF);
}
