
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main(void) {
    const char *source =
        "int main(void) {\n"
        "  // sample loop\n"
        "  for (int i = 0; i < 10; i++) { }\n"
        "  return 0;\n"
        "}\n";

    TokenList list;

    if (!lex_source(source, &list))
        return 1;

    for (size_t i = 0; i < list.count; i++) {
        Token *t = &list.tokens[i];
        printf("%-12s %-8s line %zu\n",
               token_type_name(t->type), t->text, t->line);
    }

    free_tokens(&list);
    return 0;
}
