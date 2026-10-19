#include <stdio.h>
#include <string.h>

#include "recommender.h"

#define MAX_LOOPS 16

/* The loop variables of the for loops that enclose the current node. */
typedef struct {
    const char *vars[MAX_LOOPS];
    int count;
} LoopStack;

static int in_stack(const LoopStack *loops, const char *name) {
    for (int i = 0; i < loops->count; i++)
        if (strcmp(loops->vars[i], name) == 0)
            return 1;

    return 0;
}

/* arr[...] -> "arr" (only plain variable arrays are recognized) */
static const char *array_name(const AST *element) {
    if (element && element->type == AST_INDEX &&
        element->left && element->left->type == AST_VARIABLE)
        return element->left->text;

    return NULL;
}

/* arr[i] -> "i" (only a plain variable index is recognized) */
static const char *index_name(const AST *element) {
    if (element && element->type == AST_INDEX &&
        element->right && element->right->type == AST_VARIABLE)
        return element->right->text;

    return NULL;
}

/* Looks for  arr[a] == arr[b]  inside a condition, also when it is joined
   with other tests by && (for example: i != j && arr[i] == arr[j]).
   a and b must be different variables of two enclosing loops. */
static int find_duplicate_comparison(const AST *cond, const LoopStack *loops,
                                     const char **array, const char **a,
                                     const char **b) {
    if (!cond || cond->type != AST_BINARY || !cond->text)
        return 0;

    if (strcmp(cond->text, "&&") == 0)
        return find_duplicate_comparison(cond->left, loops, array, a, b) ||
               find_duplicate_comparison(cond->right, loops, array, a, b);

    if (strcmp(cond->text, "==") != 0)
        return 0;

    const char *name1 = array_name(cond->left);
    const char *name2 = array_name(cond->right);
    const char *idx1 = index_name(cond->left);
    const char *idx2 = index_name(cond->right);

    if (!name1 || !name2 || !idx1 || !idx2)
        return 0;

    if (strcmp(name1, name2) != 0 || strcmp(idx1, idx2) == 0)
        return 0;

    if (!in_stack(loops, idx1) || !in_stack(loops, idx2))
        return 0;

    *array = name1;
    *a = idx1;
    *b = idx2;
    return 1;
}

static void report_duplicate_search(const char *array, const char *a,
                                    const char *b) {
    printf("\nPattern: duplicate search on array '%s'\n", array);
    printf("  Where: nested loops over '%s' and '%s' compare %s[%s] == %s[%s]\n",
           a, b, array, a, array, b);
    printf("  Why it can be slow: every pair of elements is compared. If both "
           "loops run about n times, that is about O(n^2) comparisons.\n");

    printf("  Option 1: sort a copy of the array, then compare neighbours.\n");
    printf("    Time: O(n log n)   Extra memory: O(n)\n");
    printf("    Trade-off: the sort must not disturb the original order if "
           "later code needs it.\n");

    printf("  Option 2: keep a hash set of values seen so far.\n");
    printf("    Time: O(n) on average   Extra memory: O(n)\n");
    printf("    Trade-off: C has no built-in hash set, so you must write or "
           "import one; worst case is slower.\n");

    printf("  Assumptions and limits:\n");
    printf("    - Only suitable if the loop body just needs to know whether "
           "(or which) equal values exist.\n");
    printf("    - If the code must visit every equal pair, the output itself "
           "can be O(n^2) and no faster method exists.\n");
    printf("    - CForge has NOT verified that a replacement keeps the same "
           "behaviour as your loop body.\n");
    printf("    - For small n, the original loops may be just as fast in "
           "practice.\n");
}

static void walk(const AST *node, LoopStack *loops, int *found) {
    if (!node) return;

    int pushed = 0;

    if (node->type == AST_FOR && node->left && node->left->text &&
        loops->count < MAX_LOOPS) {
        loops->vars[loops->count++] = node->left->text;
        pushed = 1;
    }

    if (node->type == AST_IF) {
        const char *array = NULL, *a = NULL, *b = NULL;

        if (find_duplicate_comparison(node->left, loops, &array, &a, &b)) {
            report_duplicate_search(array, a, b);
            (*found)++;
        }
    }

    walk(node->left, loops, found);
    walk(node->right, loops, found);
    walk(node->third, loops, found);

    for (size_t i = 0; i < node->child_count; i++)
        walk(node->children[i], loops, found);

    if (pushed)
        loops->count--;
}

int print_recommendations(const AST *root) {
    LoopStack loops;
    loops.count = 0;

    int found = 0;

    printf("\n--- CForge recommendations ---");
    walk(root, &loops, &found);

    if (found == 0)
        printf("\nNo known inefficient patterns detected "
               "(only duplicate search is recognized so far).\n");

    return found;
}