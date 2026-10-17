#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parser state */
static const TokenList *input;
static size_t pos;
static int failed;

/* ---------- AST utilities ---------- */

AST *ast_create(ASTType type, const char *text) {
    AST *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    node->type = type;

    if (text) {
        size_t length = strlen(text) + 1;
        node->text = malloc(length);
        if (!node->text) {
            free(node);
            return NULL;
        }
        memcpy(node->text, text, length);
    }
    return node;
}

int ast_add_child(AST *parent, AST *child) {
    if (!parent || !child) return 0;

    AST **items = realloc(
        parent->children,
        (parent->child_count + 1) * sizeof(*items)
    );

    if (!items) return 0;

    parent->children = items;
    parent->children[parent->child_count++] = child;
    return 1;
}

void ast_free(AST *node) {
    if (!node) return;

    ast_free(node->left);
    ast_free(node->right);
    ast_free(node->third);

    for (size_t i = 0; i < node->child_count; i++)
        ast_free(node->children[i]);

    free(node->children);
    free(node->text);
    free(node);
}

void ast_print(const AST *node, int depth) {
    if (!node) return;

    static const char *names[] = {
        "Number", "Variable", "Binary", "Declaration",
        "Assignment", "Return", "Block", "For",
        "String", "Call", "ExprStmt"
    };

    for (int i = 0; i < depth; i++)
        printf("  ");

    printf("%s", names[node->type]);

    if (node->text)
        printf(": %s", node->text);

    putchar('\n');

    ast_print(node->left, depth + 1);
    ast_print(node->right, depth + 1);
    ast_print(node->third, depth + 1);

    for (size_t i = 0; i < node->child_count; i++)
        ast_print(node->children[i], depth + 1);
}

/* ---------- Token helpers ---------- */

static const Token *current(void) {
    if (pos >= input->count)
        return &input->tokens[input->count - 1];

    return &input->tokens[pos];
}

static int is(const char *text) {
    return strcmp(current()->text, text) == 0;
}

static int accept(const char *text) {
    if (is(text)) {
        pos++;
        return 1;
    }
    return 0;
}

static int expect(const char *text) {
    if (accept(text)) return 1;

    fprintf(stderr, "Expected '%s' at line %zu\n",
            text, current()->line);
    failed = 1;
    return 0;
}

/* ---------- Expressions ---------- */

static AST *expression(void);
static AST *statement(void);
static AST *block(void);

static AST *primary(void) {
    if (current()->type == TOK_NUMBER) {
        AST *node = ast_create(AST_NUMBER, current()->text);
        pos++;
        return node;
    }

    if (current()->type == TOK_STRING) {
        AST *node = ast_create(AST_STRING, current()->text);
        pos++;
        return node;
    }

    if (current()->type == TOK_IDENTIFIER) {
        /* name(...) is a function call; a bare name is a variable. */
        if (pos + 1 < input->count &&
            strcmp(input->tokens[pos + 1].text, "(") == 0 &&
            input->tokens[pos + 1].type == TOK_PUNCTUATION) {
            AST *call = ast_create(AST_CALL, current()->text);
            if (!call) return NULL;
            pos += 2; /* skip name and '(' */

            if (!is(")")) {
                do {
                    AST *arg = expression();
                    if (!arg || !ast_add_child(call, arg)) {
                        ast_free(arg);
                        ast_free(call);
                        return NULL;
                    }
                } while (accept(","));
            }

            if (!expect(")")) {
                ast_free(call);
                return NULL;
            }
            return call;
        }

        AST *node = ast_create(AST_VARIABLE, current()->text);
        pos++;
        return node;
    }

    if (accept("(")) {
        AST *node = expression();

        if (!node || !expect(")")) {
            ast_free(node);
            return NULL;
        }
        return node;
    }

    fprintf(stderr, "Expected expression at line %zu\n",
            current()->line);
    failed = 1;
    return NULL;
}

static int precedence(const char *op) {
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0)
        return 1;

    if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
        strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0)
        return 2;

    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0)
        return 3;

    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 ||
        strcmp(op, "%") == 0)
        return 4;

    return 0;
}

static AST *binary_expression(int min_precedence) {
    AST *left = primary();
    if (!left) return NULL;

    while (current()->type == TOK_OPERATOR) {
        int prec = precedence(current()->text);

        if (prec < min_precedence)
            break;

        char op[8];
        snprintf(op, sizeof(op), "%s", current()->text);
        pos++;

        AST *right = binary_expression(prec + 1);

        if (!right) {
            ast_free(left);
            return NULL;
        }

        AST *node = ast_create(AST_BINARY, op);

        if (!node) {
            ast_free(left);
            ast_free(right);
            return NULL;
        }

        node->left = left;
        node->right = right;
        left = node;
    }

    return left;
}

static AST *expression(void) {
    return binary_expression(1);
}

/* ---------- Statements ---------- */

static AST *block(void) {
    if (!expect("{")) return NULL;

    AST *node = ast_create(AST_BLOCK, NULL);
    if (!node) return NULL;

    while (!failed && !is("}") &&
           current()->type != TOK_EOF) {
        AST *child = statement();

        if (!child || !ast_add_child(node, child)) {
            ast_free(child);
            ast_free(node);
            return NULL;
        }
    }

    if (!expect("}")) {
        ast_free(node);
        return NULL;
    }

    return node;
}

static AST *make_increment(const char *variable, int decrement) {
    AST *assignment = ast_create(AST_ASSIGNMENT, variable);
    AST *binary = ast_create(AST_BINARY, decrement ? "-" : "+");
    AST *var = ast_create(AST_VARIABLE, variable);
    AST *one = ast_create(AST_NUMBER, "1");

    if (!assignment || !binary || !var || !one) {
        ast_free(assignment);
        ast_free(binary);
        ast_free(var);
        ast_free(one);
        return NULL;
    }

    binary->left = var;
    binary->right = one;
    assignment->left = binary;

    return assignment;
}

static AST *statement(void) {
    /* return expression; */
    if (accept("return")) {
        AST *value = expression();

        if (!value || !expect(";")) {
            ast_free(value);
            return NULL;
        }

        AST *node = ast_create(AST_RETURN, NULL);

        if (!node) {
            ast_free(value);
            return NULL;
        }

        node->left = value;
        return node;
    }

    /* int variable [= expression]; */
    if (accept("int")) {
        if (current()->type != TOK_IDENTIFIER) {
            fprintf(stderr, "Expected variable name at line %zu\n",
                    current()->line);
            failed = 1;
            return NULL;
        }

        AST *node = ast_create(AST_DECLARATION, current()->text);
        pos++;

        if (!node) return NULL;

        if (accept("=")) {
            node->left = expression();

            if (!node->left) {
                ast_free(node);
                return NULL;
            }
        }

        if (!expect(";")) {
            ast_free(node);
            return NULL;
        }

        return node;
    }

    /* for (initializer; condition; update) statement */
    if (accept("for")) {
        AST *node = ast_create(AST_FOR, NULL);
        if (!node) return NULL;

        if (!expect("(")) {
            ast_free(node);
            return NULL;
        }

        /* Initializer */
        if (!accept(";")) {
            if (accept("int")) {
                if (current()->type != TOK_IDENTIFIER) {
                    failed = 1;
                    ast_free(node);
                    return NULL;
                }

                node->left = ast_create(
                    AST_DECLARATION, current()->text
                );
                pos++;

                if (!node->left) {
                    ast_free(node);
                    return NULL;
                }

                if (accept("=")) {
                    node->left->left = expression();

                    if (!node->left->left) {
                        ast_free(node);
                        return NULL;
                    }
                }
            } else if (current()->type == TOK_IDENTIFIER &&
                       pos + 1 < input->count &&
                       strcmp(input->tokens[pos + 1].text, "=") == 0) {
                node->left = ast_create(
                    AST_ASSIGNMENT, current()->text
                );
                pos += 2;

                if (!node->left) {
                    ast_free(node);
                    return NULL;
                }

                node->left->left = expression();

                if (!node->left->left) {
                    ast_free(node);
                    return NULL;
                }
            } else {
                node->left = expression();

                if (!node->left) {
                    ast_free(node);
                    return NULL;
                }
            }

            if (!expect(";")) {
                ast_free(node);
                return NULL;
            }
        }

        /* Condition */
        if (!accept(";")) {
            node->right = expression();

            if (!node->right || !expect(";")) {
                ast_free(node);
                return NULL;
            }
        }

        /* Update */
        if (!is(")")) {
            if (current()->type == TOK_IDENTIFIER &&
                pos + 1 < input->count &&
                (strcmp(input->tokens[pos + 1].text, "++") == 0 ||
                 strcmp(input->tokens[pos + 1].text, "--") == 0)) {
                char variable[256];

                snprintf(variable, sizeof(variable), "%s",
                         current()->text);

                int decrement =
                    strcmp(input->tokens[pos + 1].text, "--") == 0;

                pos += 2;

                node->third = make_increment(variable, decrement);

                if (!node->third) {
                    ast_free(node);
                    return NULL;
                }
            } else if (current()->type == TOK_IDENTIFIER &&
                       pos + 1 < input->count &&
                       strcmp(input->tokens[pos + 1].text, "=") == 0) {
                node->third = ast_create(
                    AST_ASSIGNMENT, current()->text
                );
                pos += 2;

                if (!node->third) {
                    ast_free(node);
                    return NULL;
                }

                node->third->left = expression();

                if (!node->third->left) {
                    ast_free(node);
                    return NULL;
                }
            } else {
                node->third = expression();

                if (!node->third) {
                    ast_free(node);
                    return NULL;
                }
            }
        }

        if (!expect(")")) {
            ast_free(node);
            return NULL;
        }

        AST *body = statement();

        if (!body || !ast_add_child(node, body)) {
            ast_free(body);
            ast_free(node);
            return NULL;
        }

        return node;
    }

    /* Nested block */
    if (is("{"))
        return block();

    /* variable = expression; */
    if (current()->type == TOK_IDENTIFIER &&
        pos + 1 < input->count &&
        strcmp(input->tokens[pos + 1].text, "=") == 0) {
        AST *node = ast_create(AST_ASSIGNMENT, current()->text);

        if (!node) return NULL;

        pos += 2;
        node->left = expression();

        if (!node->left || !expect(";")) {
            ast_free(node);
            return NULL;
        }

        return node;
    }

    /* expression;  (for example a function call: printf("hi");) */
    if (current()->type == TOK_IDENTIFIER ||
        current()->type == TOK_NUMBER ||
        current()->type == TOK_STRING ||
        is("(")) {
        AST *value = expression();

        if (!value || !expect(";")) {
            ast_free(value);
            return NULL;
        }

        AST *node = ast_create(AST_EXPR_STMT, NULL);

        if (!node) {
            ast_free(value);
            return NULL;
        }

        node->left = value;
        return node;
    }

    fprintf(stderr, "Unsupported statement at line %zu\n",
            current()->line);
    failed = 1;
    return NULL;
}

/* ---------- Program parser ---------- */

AST *parse_program(const TokenList *tokens) {
    if (!tokens || !tokens->tokens || tokens->count == 0)
        return NULL;

    input = tokens;
    pos = 0;
    failed = 0;

    AST *root = ast_create(AST_BLOCK, "Program");
    if (!root) return NULL;

    while (!failed && current()->type != TOK_EOF) {
        if (!expect("int") || !expect("main")) {
            ast_free(root);
            return NULL;
        }

        if (!expect("(")) {
            ast_free(root);
            return NULL;
        }

        /* Support main(void) and main(). */
        if (!accept("void") && !is(")")) {
            fprintf(stderr, "Expected 'void' or ')' at line %zu\n",
                    current()->line);
            ast_free(root);
            return NULL;
        }

        if (!expect(")")) {
            ast_free(root);
            return NULL;
        }

        AST *body = block();

        if (!body || !ast_add_child(root, body)) {
            ast_free(body);
            ast_free(root);
            return NULL;
        }
    }

    if (failed) {
        ast_free(root);
        return NULL;
    }

    return root;
}