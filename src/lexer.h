#ifndef CFORGE_LEXER_H
#define CFORGE_LEXER_H

#include <stddef.h>

typedef enum {
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_KEYWORD,
    TOK_OPERATOR,
    TOK_PUNCTUATION,
    TOK_EOF,
    TOK_ERROR,
    TOK_STRING
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    size_t line;
    size_t column;
} Token;

typedef struct {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenList;

int lex_source(const char *source, TokenList *result);
void free_tokens(TokenList *list);
const char *token_type_name(TokenType type);

#endif