#ifndef CFORGE_ANALYZER_H
#define CFORGE_ANALYZER_H

#include "ast.h"

#define MAX_LOOP_INFO 64

/* Complexity of the form  n^n_power * (log n)^log_power */
typedef struct {
    int n_power;
    int log_power;
    int unknown;      /* 1 if some loop could not be analyzed */
} Complexity;

typedef struct {
    char var[64];     /* loop variable, e.g. "i" */
    char cost[16];    /* "O(1)", "O(n)", "O(log n)" or "unknown" */
    char reason[96];  /* short explanation */
} LoopInfo;

typedef struct {
    int loop_count;   /* total number of for loops */
    int max_depth;    /* deepest nesting of for loops */
    Complexity complexity;
    LoopInfo loops[MAX_LOOP_INFO];
    int info_count;
} AnalysisResult;

AnalysisResult analyze_ast(const AST *root);
void print_analysis(const AnalysisResult *result);

#endif