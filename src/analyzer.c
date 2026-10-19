#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analyzer.h"

/* ---------- Small helpers ---------- */

typedef enum { VAL_CONST, VAL_INPUT, VAL_UNKNOWN } ValueKind;

/* CONST: only numbers. INPUT: uses variables. UNKNOWN: has a function call. */
static ValueKind classify_value(const AST *node) {
    if (!node) return VAL_UNKNOWN;

    if (node->type == AST_CALL)
        return VAL_UNKNOWN;

    ValueKind result = (node->type == AST_VARIABLE) ? VAL_INPUT : VAL_CONST;

    const AST *parts[3] = { node->left, node->right, node->third };

    for (int i = 0; i < 3; i++) {
        if (!parts[i]) continue;

        ValueKind k = classify_value(parts[i]);

        if (k == VAL_UNKNOWN) return VAL_UNKNOWN;
        if (k == VAL_INPUT) result = VAL_INPUT;
    }

    for (size_t i = 0; i < node->child_count; i++) {
        ValueKind k = classify_value(node->children[i]);

        if (k == VAL_UNKNOWN) return VAL_UNKNOWN;
        if (k == VAL_INPUT) result = VAL_INPUT;
    }

    return result;
}

static int is_var(const AST *node, const char *name) {
    return node && node->type == AST_VARIABLE &&
           node->text && strcmp(node->text, name) == 0;
}

static int is_binary(const AST *node, const char *op) {
    return node && node->type == AST_BINARY &&
           node->text && strcmp(node->text, op) == 0;
}

/* ---------- Complexity arithmetic ---------- */

static Complexity cx_mul(Complexity a, Complexity b) {
    Complexity r;
    r.n_power = a.n_power + b.n_power;
    r.log_power = a.log_power + b.log_power;
    r.unknown = a.unknown || b.unknown;
    return r;
}

static Complexity cx_max(Complexity a, Complexity b) {
    Complexity r;

    if (a.n_power > b.n_power ||
        (a.n_power == b.n_power && a.log_power >= b.log_power))
        r = a;
    else
        r = b;

    r.unknown = a.unknown || b.unknown;
    return r;
}

static void format_complexity(Complexity c, char *buf, size_t size) {
    if (c.n_power == 0 && c.log_power == 0) {
        snprintf(buf, size, "O(1)");
        return;
    }

    char n_part[16] = "";
    char log_part[16] = "";

    if (c.n_power == 1)
        snprintf(n_part, sizeof(n_part), "n");
    else if (c.n_power > 1)
        snprintf(n_part, sizeof(n_part), "n^%d", c.n_power);

    if (c.log_power == 1)
        snprintf(log_part, sizeof(log_part), "log n");
    else if (c.log_power > 1)
        snprintf(log_part, sizeof(log_part), "log^%d n", c.log_power);

    snprintf(buf, size, "O(%s%s%s)", n_part,
             (n_part[0] && log_part[0]) ? " " : "", log_part);
}

/* ---------- Analyzing one for loop header ---------- */

static Complexity unknown_loop(LoopInfo *info, const char *reason) {
    Complexity c = {0, 0, 1};
    snprintf(info->cost, sizeof(info->cost), "unknown");
    snprintf(info->reason, sizeof(info->reason), "%s", reason);
    return c;
}

static Complexity analyze_header(const AST *loop, LoopInfo *info) {
    const AST *init = loop->left;
    const AST *cond = loop->right;
    const AST *update = loop->third;

    info->var[0] = '\0';

    /* Initializer: int i = value;  or  i = value; */
    if (!init || !init->text ||
        (init->type != AST_DECLARATION && init->type != AST_ASSIGNMENT))
        return unknown_loop(info, "no simple loop variable in initializer");

    snprintf(info->var, sizeof(info->var), "%s", init->text);
    const char *var = init->text;
    const AST *start = init->left;

    if (!start)
        return unknown_loop(info, "loop variable has no starting value");

    /* Condition: var < bound, var <= bound, var > bound, ... */
    if (!cond || cond->type != AST_BINARY)
        return unknown_loop(info, "missing or unsupported condition");

    const char *op = cond->text;
    const AST *bound;
    int cond_dir; /* +1: runs while var is below bound, -1: above, 0: != */

    if (is_var(cond->left, var)) {
        bound = cond->right;

        if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0) cond_dir = 1;
        else if (strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) cond_dir = -1;
        else if (strcmp(op, "!=") == 0) cond_dir = 0;
        else return unknown_loop(info, "unsupported comparison");
    } else if (is_var(cond->right, var)) {
        bound = cond->left; /* bound < var  means var is above bound */

        if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0) cond_dir = -1;
        else if (strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) cond_dir = 1;
        else if (strcmp(op, "!=") == 0) cond_dir = 0;
        else return unknown_loop(info, "unsupported comparison");
    } else {
        return unknown_loop(info, "condition does not test the loop variable");
    }

    /* Update: var = var + c, var - c, var * c, var / c */
    if (!update || update->type != AST_ASSIGNMENT || !update->text ||
        strcmp(update->text, var) != 0 || !update->left ||
        update->left->type != AST_BINARY)
        return unknown_loop(info, "update is not a simple change of the variable");

    const AST *expr = update->left;
    const AST *number = NULL;
    int multiplicative = 0;
    int update_dir = 0; /* +1 grows, -1 shrinks */

    if (is_binary(expr, "+") || is_binary(expr, "*")) {
        if (is_var(expr->left, var) && expr->right &&
            expr->right->type == AST_NUMBER)
            number = expr->right;
        else if (is_var(expr->right, var) && expr->left &&
                 expr->left->type == AST_NUMBER)
            number = expr->left;

        multiplicative = is_binary(expr, "*");
        update_dir = 1;
    } else if (is_binary(expr, "-") || is_binary(expr, "/")) {
        if (is_var(expr->left, var) && expr->right &&
            expr->right->type == AST_NUMBER)
            number = expr->right;

        multiplicative = is_binary(expr, "/");
        update_dir = -1;
    }

    if (!number)
        return unknown_loop(info, "update is not 'variable (+ - * /) number'");

    long step = strtol(number->text, NULL, 10);

    if (step <= 0 || (multiplicative && step < 2))
        return unknown_loop(info, "update never makes progress (infinite loop?)");

    if (cond_dir != 0 && cond_dir != update_dir)
        return unknown_loop(info,
                            "update moves away from the bound (infinite loop?)");

    ValueKind start_kind = classify_value(start);
    ValueKind bound_kind = classify_value(bound);

    if (start_kind == VAL_UNKNOWN || bound_kind == VAL_UNKNOWN)
        return unknown_loop(info, "start or bound contains a function call");

    Complexity c = {0, 0, 0};

    if (start_kind == VAL_CONST && bound_kind == VAL_CONST) {
        snprintf(info->cost, sizeof(info->cost), "O(1)");
        snprintf(info->reason, sizeof(info->reason),
                 "start and bound are constants");
        return c;
    }

    if (multiplicative) {
        if (start->type == AST_NUMBER && strtol(start->text, NULL, 10) <= 0 &&
            update_dir == 1)
            return unknown_loop(info,
                                "multiplying a value that starts at 0 never grows");

        c.log_power = 1;
        snprintf(info->cost, sizeof(info->cost), "O(log n)");
        snprintf(info->reason, sizeof(info->reason),
                 "multiplies or divides by a constant each step");
        return c;
    }

    c.n_power = 1;
    snprintf(info->cost, sizeof(info->cost), "O(n)");
    snprintf(info->reason, sizeof(info->reason),
             "steps by a constant towards an input-dependent bound");
    return c;
}

/* ---------- Walking the tree ---------- */

/* Returns the complexity of 'node', treating statements as a sequence. */
static Complexity walk(const AST *node, int depth, AnalysisResult *result) {
    Complexity total = {0, 0, 0};

    if (!node) return total;

    if (node->type == AST_FOR || node->type == AST_WHILE) {
        depth++;
        result->loop_count++;

        if (depth > result->max_depth)
            result->max_depth = depth;

        LoopInfo scratch;
        LoopInfo *info = &scratch;

        if (result->info_count < MAX_LOOP_INFO)
            info = &result->loops[result->info_count++];

        Complexity own;
        Complexity body = {0, 0, 0};

        if (node->type == AST_FOR) {
            own = analyze_header(node, info);

            for (size_t i = 0; i < node->child_count; i++)
                body = cx_max(body, walk(node->children[i], depth, result));
        } else {
            snprintf(info->var, sizeof(info->var), "while");
            own = unknown_loop(info, "while loops are not analyzed yet");
            body = walk(node->right, depth, result);
        }

        return cx_mul(own, body);
    }

    total = cx_max(total, walk(node->left, depth, result));
    total = cx_max(total, walk(node->right, depth, result));
    total = cx_max(total, walk(node->third, depth, result));

    for (size_t i = 0; i < node->child_count; i++)
        total = cx_max(total, walk(node->children[i], depth, result));

    return total;
}

AnalysisResult analyze_ast(const AST *root) {
    AnalysisResult result;
    memset(&result, 0, sizeof(result));

    result.complexity = walk(root, 0, &result);
    return result;
}

/* ---------- Report ---------- */

void print_analysis(const AnalysisResult *result) {
    printf("\n--- CForge analysis ---\n");
    printf("Loops detected: %d\n", result->loop_count);
    printf("Maximum loop nesting: %d\n", result->max_depth);

    for (int i = 0; i < result->info_count; i++) {
        const LoopInfo *info = &result->loops[i];

        printf("  Loop %d (%s): %s - %s\n", i + 1,
               info->var[0] ? info->var : "?", info->cost, info->reason);
    }

    if (result->loop_count == 0) {
        printf("Estimated time complexity: O(1) loop-related work\n");
    } else if (result->complexity.unknown) {
        printf("Estimated time complexity: Unknown; at least one loop "
               "cannot be analyzed yet.\n");
    } else {
        char text[48];
        format_complexity(result->complexity, text, sizeof(text));

        if (result->complexity.n_power == 0 &&
            result->complexity.log_power == 0)
            printf("Estimated time complexity: approximately %s "
                   "(all loops have constant bounds)\n", text);
        else
            printf("Estimated time complexity: approximately %s\n", text);
    }

    printf("\nAssumption: any variable used in a loop bound is treated as "
           "the input size n.\n");
}