
#include <stdio.h>
#include <string.h>
#include "recommender.h"

void recommend_improvements(const char *source) {
    if (strstr(source, "for") != NULL &&
        strstr(source, "if") != NULL) {
        printf("\nCForge Recommendation:\n");
        printf("- Review repeated comparisons inside loops.\n");
        printf("- If searching for duplicates, consider a hash set.\n");
        printf("- Expected average time: O(n), with O(n) extra space.\n");
        printf("- This suggestion requires suitable input and hash behavior.\n");
    } else {
        printf("\nNo matching optimization rule found yet.\n");
    }
}
