#ifndef CFORGE_ANALYZER_H
#define CFORGE_ANALYZER_H

#include "ast.h"

typedef struct {
    int loop_count;   /* total number of for loops */
    int max_depth;    /* deepest nesting of for loops */
} AnalysisResult;

AnalysisResult analyze_ast(const AST *root);
void print_analysis(const AnalysisResult *result);

#endif