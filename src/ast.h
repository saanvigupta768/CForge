#ifndef CFORGE_AST_H
#define CFORGE_AST_H

#include <stddef.h>

typedef enum {
    AST_NUMBER,
    AST_VARIABLE,
    AST_BINARY,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_RETURN,
    AST_BLOCK,
    AST_FOR,
    AST_STRING,     /* string literal; text holds the characters with quotes */
    AST_CALL,       /* function call; text = name, children = arguments */
    AST_EXPR_STMT,  /* expression used as a statement; left = expression */
    AST_IF,         /* left = condition, right = then, third = else (or NULL) */
    AST_WHILE,      /* left = condition, right = body */
    AST_INDEX,      /* array[index]; left = array, right = index */
    AST_INDEX_ASSIGN /* array[index] = value; left = target, right = value */
} ASTType;

typedef struct AST AST;

struct AST {
    ASTType type;
    char *text;
    AST *left;
    AST *right;
    AST *third;
    AST **children;
    size_t child_count;
};

AST *ast_create(ASTType type, const char *text);
int ast_add_child(AST *parent, AST *child);
void ast_free(AST *node);
void ast_print(const AST *node, int depth);

#endif