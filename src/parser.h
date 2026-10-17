
#ifndef CFORGE_PARSER_H
#define CFORGE_PARSER_H

#include "ast.h"
#include "lexer.h"

AST *parse_program(const TokenList *tokens);

#endif
