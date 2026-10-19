#ifndef CFORGE_RECOMMENDER_H
#define CFORGE_RECOMMENDER_H

#include "ast.h"

/* Prints suggestions for recognized inefficient patterns.
   Returns how many patterns were found. */
int print_recommendations(const AST *root);

#endif