
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 20000
#define MAX_TEXT 128
#define MAX_BLOCKS 2048

typedef struct {
    char text[MAX_TEXT];
    int line;
} Token;

typedef struct {
    int loop_depth;
    int unknown_depth;
} Block;

static Token tokens[MAX_TOKENS];
static int token_count = 0;
static int had_error = 0;

/* ---------- Tokenizer ---------- */

static int add_token(const char *start, size_t length, int line) {
    if (token_count >= MAX_TOKENS || length >= MAX_TEXT) {
        fprintf(stderr,
                "Input too large or token too long (line %d).\n", line);
        had_error = 1;
        return 0;
    }

    Token *t = &tokens[token_count++];
    memcpy(t->text, start, length);
    t->text[length] = '\0';
    t->line = line;
    return 1;
}

static int is_word_start(int c) {
    return isalpha((unsigned char)c) || c == '_';
}

static int is_word_char(int c) {
    return isalnum((unsigned char)c) || c == '_';
}

static int tokenize(FILE *file) {
    int c, line = 1;

    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            line++;
            continue;
        }

        if (isspace((unsigned char)c))
            continue;

        /* Skip line comments. */
        if (c == '/') {
            int next = fgetc(file);

            if (next == '/') {
                while ((c = fgetc(file)) != EOF && c != '\n')
                    ;
                if (c == '\n')
                    line++;
                continue;
            }

            /* Skip block comments. */
            if (next == '*') {
                int previous = 0;
                int closed = 0;

                while ((c = fgetc(file)) != EOF) {
                    if (c == '\n')
                        line++;

                    if (previous == '*' && c == '/') {
                        closed = 1;
                        break;
                    }
                    previous = c;
                }

                if (!closed) {
                    fprintf(stderr, "Unclosed comment.\n");
                    had_error = 1;
                    return 0;
                }
                continue;
            }

            if (next != EOF)
                ungetc(next, file);

            if (!add_token("/", 1, line))
                return 0;
            continue;
        }

        /* Skip strings and character literals. */
        if (c == '"' || c == '\'') {
            int quote = c;
            int escaped = 0;
            int closed = 0;

            while ((c = fgetc(file)) != EOF) {
                if (c == '\n')
                    line++;

                if (!escaped && c == quote) {
                    closed = 1;
                    break;
                }

                if (!escaped && c == '\\')
                    escaped = 1;
                else
                    escaped = 0;
            }

            if (!closed) {
                fprintf(stderr, "Unclosed string or character literal.\n");
                had_error = 1;
                return 0;
            }
            continue;
        }

        /* Identifiers and keywords. */
        if (is_word_start(c)) {
            char word[MAX_TEXT];
            size_t len = 0;
            word[len++] = (char)c;

            while ((c = fgetc(file)) != EOF && is_word_char(c)) {
                if (len + 1 >= sizeof word) {
                    fprintf(stderr, "Identifier too long.\n");
                    had_error = 1;
                    return 0;
                }
                word[len++] = (char)c;
            }

            if (c != EOF)
                ungetc(c, file);

            if (!add_token(word, len, line))
                return 0;
            continue;
        }

        /* Numeric literals: enough for basic loop-bound analysis. */
        if (isdigit((unsigned char)c)) {
            char number[MAX_TEXT];
            size_t len = 0;
            number[len++] = (char)c;

            while ((c = fgetc(file)) != EOF &&
                   (isalnum((unsigned char)c) || c == '.')) {
                if (len + 1 >= sizeof number) {
                    fprintf(stderr, "Number too long.\n");
                    had_error = 1;
                    return 0;
                }
                number[len++] = (char)c;
            }

            if (c != EOF)
                ungetc(c, file);

            if (!add_token(number, len, line))
                return 0;
            continue;
        }

        /* Keep common two-character operators together. */
        char op[3] = {(char)c, '\0', '\0'};
        int next = fgetc(file);

        if (next != EOF) {
            if ((c == '+' && next == '+') ||
                (c == '-' && next == '-') ||
                (c == '<' && next == '=') ||
                (c == '>' && next == '=') ||
                (c == '=' && next == '=') ||
                (c == '!' && next == '=') ||
                (c == '+' && next == '=') ||
                (c == '-' && next == '=') ||
                (c == '&' && next == '&') ||
                (c == '|' && next == '|')) {
                op[1] = (char)next;
            } else {
                ungetc(next, file);
            }
        }

        if (!add_token(op, strlen(op), line))
            return 0;
    }

    return 1;
}

/* ---------- Loop analysis ---------- */

/*
 * Return 1 for a recognizable constant-time loop,
 * 0 for a recognizable variable-bound loop,
 * and -1 when the pattern is unknown.
 *
 * This is a heuristic, not a proof of complexity.
 */
static int estimate_for_loop(int start, int end) {
    int first_semicolon = -1;
    int second_semicolon = -1;

    for (int i = start; i <= end; i++) {
        if (strcmp(tokens[i].text, ";") == 0) {
            if (first_semicolon == -1)
                first_semicolon = i;
            else {
                second_semicolon = i;
                break;
            }
        }
    }

    if (first_semicolon == -1 || second_semicolon == -1)
        return -1;

    int has_variable_bound = 0;
    int has_numeric_bound = 0;
    int has_comparison = 0;

    for (int i = first_semicolon + 1; i < second_semicolon; i++) {
        const char *s = tokens[i].text;

        if (strcmp(s, "<") == 0 || strcmp(s, "<=") == 0 ||
            strcmp(s, ">") == 0 || strcmp(s, ">=") == 0) {
            has_comparison = 1;

            if (i + 1 < second_semicolon) {
                if (isdigit((unsigned char)tokens[i + 1].text[0]))
                    has_numeric_bound = 1;
                else if (is_word_start(tokens[i + 1].text[0]))
                    has_variable_bound = 1;
            }
        }
    }

    if (!has_comparison)
        return -1;

    /* Recognize common increment/decrement forms. */
    int has_step = 0;

    for (int i = second_semicolon + 1; i <= end; i++) {
        const char *s = tokens[i].text;

        if (strcmp(s, "++") == 0 || strcmp(s, "--") == 0 ||
            strcmp(s, "+=") == 0 || strcmp(s, "-=") == 0) {
            has_step = 1;
        }
    }

    if (!has_step)
        return -1;

    if (has_variable_bound)
        return 0;

    if (has_numeric_bound)
        return 1;

    return -1;
}

static int analyze(void) {
    Block stack[MAX_BLOCKS];
    int blocks = 0;
    int pending_loop = 0;
    int pending_unknown = 0;
    int max_loop_depth = 0;
    int max_unknown_depth = 0;
    int loops = 0;
    int unknown_loops = 0;

    for (int i = 0; i < token_count; i++) {
        const char *s = tokens[i].text;

        if (strcmp(s, "for") == 0 || strcmp(s, "while") == 0) {
            loops++;

            if (strcmp(s, "for") == 0) {
                if (i + 1 >= token_count ||
                    strcmp(tokens[i + 1].text, "(") != 0) {
                    unknown_loops++;
                    pending_loop = 1;
                    pending_unknown = 1;
                    continue;
                }

                int parens = 0;
                int end = -1;

                for (int j = i + 1; j < token_count; j++) {
                    if (strcmp(tokens[j].text, "(") == 0)
                        parens++;
                    else if (strcmp(tokens[j].text, ")") == 0) {
                        parens--;
                        if (parens == 0) {
                            end = j;
                            break;
                        }
                    }
                }

                if (end == -1) {
                    fprintf(stderr, "Unclosed for-loop header at line %d.\n",
                            tokens[i].line);
                    return 0;
                }

                int estimate = estimate_for_loop(i + 2, end - 1);

                if (estimate == -1) {
                    unknown_loops++;
                    pending_unknown = 1;
                } else {
                    pending_unknown = 0;
                }

                pending_loop = 1;
            } else {
                /* While-loop termination cannot be inferred reliably yet. */
                unknown_loops++;
                pending_loop = 1;
                pending_unknown = 1;
            }
        }

        if (strcmp(s, "{") == 0) {
            if (blocks >= MAX_BLOCKS) {
                fprintf(stderr, "Block nesting limit exceeded.\n");
                return 0;
            }

            int parent_depth = blocks ? stack[blocks - 1].loop_depth : 0;
            int parent_unknown = blocks ? stack[blocks - 1].unknown_depth : 0;

            stack[blocks].loop_depth = parent_depth + pending_loop;
            stack[blocks].unknown_depth = parent_unknown + pending_unknown;
            blocks++;

            if (stack[blocks - 1].loop_depth > max_loop_depth)
                max_loop_depth = stack[blocks - 1].loop_depth;

            if (stack[blocks - 1].unknown_depth > max_unknown_depth)
                max_unknown_depth = stack[blocks - 1].unknown_depth;

            pending_loop = 0;
            pending_unknown = 0;
        } else if (strcmp(s, "}") == 0) {
            if (blocks == 0) {
                fprintf(stderr, "Unmatched closing brace at line %d.\n",
                        tokens[i].line);
                return 0;
            }
            blocks--;
        }
    }

    if (blocks != 0) {
        fprintf(stderr, "Unmatched opening brace.\n");
        return 0;
    }

    printf("Loops detected: %d\n", loops);
    printf("Maximum loop nesting: %d\n", max_loop_depth);

    if (loops == 0) {
        printf("Estimated time complexity: O(1) loop-related work\n");
    } else if (unknown_loops > 0 || max_unknown_depth > 0) {
        printf("Estimated time complexity: Unknown; some loop patterns "
               "need deeper analysis.\n");
    } else if (max_loop_depth == 1) {
        printf("Estimated time complexity: approximately O(n) "
               "for variable-bound loops.\n");
    } else {
        printf("Estimated time complexity: approximately O(n^%d) "
               "for equally scaling nested loops.\n", max_loop_depth);
    }

    printf("\nLimitations: this prototype does not fully parse C, "
           "analyze unbraced loop bodies, prove loop termination, "
           "or understand data-dependent bounds.\n");

    return 1;
}

/* ---------- Entry point ---------- */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: analyzer <source-file>\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "r");

    if (!file) {
        perror("Could not open source file");
        return 1;
    }

    int ok = tokenize(file);
    fclose(file);

    if (!ok || had_error)
        return 1;

    if (!analyze())
        return 1;

    return 0;
}
