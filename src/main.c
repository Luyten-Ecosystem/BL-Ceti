#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path, size_t *length)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "BL: no se pudo abrir '%s'.\n", path);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        fprintf(stderr, "BL: no se pudo leer el archivo.\n");
        return NULL;
    }

    long size = ftell(file);

    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        fprintf(stderr, "BL: no se pudo determinar el tamaño del archivo.\n");
        return NULL;
    }

    char *buffer = malloc((size_t)size + 1);

    if (buffer == NULL) {
        fclose(file);
        fprintf(stderr, "BL: memoria insuficiente.\n");
        return NULL;
    }

    size_t read_count = fread(buffer, 1, (size_t)size, file);

    if (read_count != (size_t)size && ferror(file)) {
        fprintf(stderr, "BL: error leyendo '%s'.\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[read_count] = '\0';
    *length = read_count;
    fclose(file);

    return buffer;
}

static int analyze_file(const char *path)
{
    size_t length = 0;
    char *source = read_file(path, &length);

    if (source == NULL) {
        return 1;
    }

    BlLexResult lex_result = {0};

    if (!bl_lex(source, length, &lex_result)) {
        fprintf(stderr, "%s:%zu:%zu: error %s: %s\n",
                path,
                lex_result.error.line,
                lex_result.error.column,
                lex_result.error.code,
                lex_result.error.message);

        bl_lex_result_destroy(&lex_result);
        free(source);
        return 1;
    }

    BlParseError parse_error = {0};

    if (!bl_parse(lex_result.tokens, lex_result.count, &parse_error)) {
        fprintf(stderr, "%s:%zu:%zu: error %s: %s\n",
                path,
                parse_error.line,
                parse_error.column,
                parse_error.code,
                parse_error.message);

        bl_lex_result_destroy(&lex_result);
        free(source);
        return 1;
    }

    printf("BL-Ceti 0.1.0: analisis correcto: %s\n", path);
    printf("Tokens reconocidos: %zu\n",
           lex_result.count > 0 ? lex_result.count - 1 : 0);
    printf("Sintaxis: correcta\n");

    bl_lex_result_destroy(&lex_result);
    free(source);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], ".C") == 0) {
        return analyze_file(argv[2]);
    }

    if (argc == 2) {
        return analyze_file(argv[1]);
    }

    fprintf(stderr,
            "BL-Ceti 0.1.0\n"
            "Uso:\n"
            "  BL .C archivo.bl\n"
            "  BL archivo.bl\n");

    return 2;
}
