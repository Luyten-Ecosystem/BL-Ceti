
#include "lexer.h"

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
        return NULL;
    }

    long size = ftell(file);

    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
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

    if (source == NULL) return 1;

    BlLexResult result = {0};
    bool success = bl_lex(source, length, &result);

    if (!success) {
        fprintf(stderr, "%s:%zu:%zu: error %s: %s\n",
                path,
                result.error.line,
                result.error.column,
                result.error.code,
                result.error.message);
        bl_lex_result_destroy(&result);
        free(source);
        return 1;
    }

    printf("BL-Ceti: analisis lexico correcto: %s\n", path);
    printf("Tokens reconocidos: %zu\n",
           result.count > 0 ? result.count - 1 : 0);

    bl_lex_result_destroy(&result);
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
