
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *source =
        "int main(void) {\n"
        "  int n = 10;\n"
        "  for (int i = 0; i < n; i++) {\n"
        "    n = n + 1;\n"
        "  }\n"
        "  return 0;\n"
        "}\n";

    TokenList tokens;
    if (!lex_source(source, &tokens))
        return 1;

    AST *tree = parse_program(&tokens);

    if (!tree) {
        free_tokens(&tokens);
        return 1;
    }

    ast_print(tree, 0);
    ast_free(tree);
    free_tokens(&tokens);
    return 0;
}
