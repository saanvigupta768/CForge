
#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *keywords[] = {
    "int", "char", "void", "return", "if", "else",
    "for", "while", "break", "continue", "const",
    "long", "short", "float", "double", "sizeof"
};

static int is_keyword(const char *s) {
    size_t n = sizeof(keywords) / sizeof(keywords[0]);

    for (size_t i = 0; i < n; i++)
        if (strcmp(s, keywords[i]) == 0)
            return 1;

    return 0;
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_IDENTIFIER:   return "IDENTIFIER";
        case TOK_NUMBER:       return "NUMBER";
        case TOK_KEYWORD:      return "KEYWORD";
        case TOK_OPERATOR:     return "OPERATOR";
        case TOK_PUNCTUATION:  return "PUNCTUATION";
        case TOK_EOF:          return "EOF";
        case TOK_STRING:       return "STRING";
        default:               return "ERROR";
    }
}

static int add_token(TokenList *list, TokenType type,
                     const char *start, size_t length,
                     size_t line, size_t column) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity ? list->capacity * 2 : 32;
        Token *p = realloc(list->tokens, new_capacity * sizeof(Token));

        if (!p)
            return 0;

        list->tokens = p;
        list->capacity = new_capacity;
    }

    char *text = malloc(length + 1);
    if (!text)
        return 0;

    memcpy(text, start, length);
    text[length] = '\0';

    list->tokens[list->count++] =
        (Token){type, text, line, column};

    return 1;
}

static int is_two_char_operator(char a, char b) {
    return (a == '+' && (b == '+' || b == '=')) ||
           (a == '-' && (b == '-' || b == '=')) ||
           (a == '=' && b == '=') ||
           (a == '!' && b == '=') ||
           (a == '<' && (b == '=' || b == '<')) ||
           (a == '>' && (b == '=' || b == '>')) ||
           (a == '&' && b == '&') ||
           (a == '|' && b == '|') ||
           (a == '*' && b == '=') ||
           (a == '/' && b == '=') ||
           (a == '%' && b == '=');
}

int lex_source(const char *source, TokenList *result) {
    memset(result, 0, sizeof(*result));

    size_t i = 0, line = 1, column = 1;

    while (source[i]) {
        char c = source[i];

        if (isspace((unsigned char)c)) {
            if (c == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            i++;
            continue;
        }

        /* Comments */
        if (c == '/' && source[i + 1] == '/') {
            while (source[i] && source[i] != '\n') {
                i++;
                column++;
            }
            continue;
        }

        if (c == '/' && source[i + 1] == '*') {
            size_t start_line = line, start_column = column;
            i += 2;
            column += 2;

            while (source[i] &&
                   !(source[i] == '*' && source[i + 1] == '/')) {
                if (source[i] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
                i++;
            }

            if (!source[i]) {
                fprintf(stderr, "Unclosed comment at %zu:%zu\n",
                        start_line, start_column);
                free_tokens(result);
                return 0;
            }

            i += 2;
            column += 2;
            continue;
        }

        size_t start = i, start_line = line, start_column = column;
        TokenType type;

        if (isalpha((unsigned char)c) || c == '_') {
            i++;
            column++;

            while (isalnum((unsigned char)source[i]) ||
                   source[i] == '_') {
                i++;
                column++;
            }

            size_t length = i - start;
            char *word = malloc(length + 1);

            if (!word) {
                free_tokens(result);
                return 0;
            }

            memcpy(word, source + start, length);
            word[length] = '\0';

            type = is_keyword(word) ? TOK_KEYWORD : TOK_IDENTIFIER;
            free(word);
        } else if (isdigit((unsigned char)c)) {
            i++;
            column++;

            while (isalnum((unsigned char)source[i]) ||
                   source[i] == '.') {
                i++;
                column++;
            }

            type = TOK_NUMBER;
        } else if (c == '"' || c == '\'') {
            char quote = c;
            i++;
            column++;
            int closed = 0;

            while (source[i]) {
                if (source[i] == '\n') {
                    fprintf(stderr, "Unclosed literal at %zu:%zu\n",
                            start_line, start_column);
                    free_tokens(result);
                    return 0;
                }

                if (source[i] == '\\' && source[i + 1]) {
                    i += 2;
                    column += 2;
                } else if (source[i] == quote) {
                    i++;
                    column++;
                    closed = 1;
                    break;
                } else {
                    i++;
                    column++;
                }
            }

            if (!closed) {
                fprintf(stderr, "Unclosed literal at %zu:%zu\n",
                        start_line, start_column);
                free_tokens(result);
                return 0;
            }

                       /* "text" is a string literal; 'c' stays number-like for now. */
            type = (quote == '"') ? TOK_STRING : TOK_NUMBER;
            
        } else if (strchr("(){}[];,", c)) {
            i++;
            column++;
            type = TOK_PUNCTUATION;
        } else if (strchr("+-*/%=!<>|&^~", c)) {
            i++;
            column++;

            if (source[i] && is_two_char_operator(c, source[i])) {
                i++;
                column++;
            }

            type = TOK_OPERATOR;
        } else {
            fprintf(stderr, "Unexpected character '%c' at %zu:%zu\n",
                    c, line, column);
            free_tokens(result);
            return 0;
        }

        if (!add_token(result, type, source + start, i - start,
                       start_line, start_column)) {
            free_tokens(result);
            return 0;
        }
    }

    if (!add_token(result, TOK_EOF, "", 0, line, column)) {
        free_tokens(result);
        return 0;
    }

    return 1;
}

void free_tokens(TokenList *list) {
    for (size_t i = 0; i < list->count; i++)
        free(list->tokens[i].text);

    free(list->tokens);
    memset(list, 0, sizeof(*list));
}
