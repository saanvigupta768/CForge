#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "analyzer.h"
#include "recommender.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: cforge <source.c>\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");

    if (!file) {
        perror("Could not open source file");
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 1;
    }

    long size = ftell(file);

    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 1;
    }

    char *source = malloc((size_t)size + 1);

    if (!source) {
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(source, 1, (size_t)size, file);
    fclose(file);

    if (bytes_read != (size_t)size) {
        fprintf(stderr, "Could not read source file completely.\n");
        free(source);
        return 1;
    }

    source[bytes_read] = '\0';

    TokenList tokens;

    if (!lex_source(source, &tokens)) {
        fprintf(stderr, "Lexing failed.\n");
        free(source);
        return 1;
    }

    AST *tree = parse_program(&tokens);

    if (!tree) {
        fprintf(stderr, "Parsing failed.\n");
        free_tokens(&tokens);
        free(source);
        return 1;
    }

    printf("CForge: parsing successful!\n\n");
    ast_print(tree, 0);

    AnalysisResult analysis = analyze_ast(tree);
    print_analysis(&analysis);
    print_recommendations(tree);

    ast_free(tree);
    free_tokens(&tokens);
    free(source);

    return 0;
}