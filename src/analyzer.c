#include <stdio.h>

#include "analyzer.h"

/* Walk the whole tree. 'depth' = how many for loops enclose this node. */
static void walk(const AST *node, int depth, AnalysisResult *result) {
    if (!node) return;

    if (node->type == AST_FOR) {
        depth++;
        result->loop_count++;

        if (depth > result->max_depth)
            result->max_depth = depth;
    }

    walk(node->left, depth, result);
    walk(node->right, depth, result);
    walk(node->third, depth, result);

    for (size_t i = 0; i < node->child_count; i++)
        walk(node->children[i], depth, result);
}

AnalysisResult analyze_ast(const AST *root) {
    AnalysisResult result = {0, 0};
    walk(root, 0, &result);
    return result;
}

void print_analysis(const AnalysisResult *result) {
    printf("\n--- CForge analysis ---\n");
    printf("Loops detected: %d\n", result->loop_count);
    printf("Maximum loop nesting: %d\n", result->max_depth);

    if (result->loop_count == 0)
        printf("Estimated time complexity: O(1) loop-related work\n");
    else if (result->max_depth == 1)
        printf("Estimated time complexity: approximately O(n)\n");
    else
        printf("Estimated time complexity: approximately O(n^%d)\n",
               result->max_depth);

    printf("\nAssumption: every loop is treated as running about n times. "
           "Loop bounds are not analyzed yet.\n");
}