#include <stdio.h>
#include <stdlib.h>
#include "fact.h"

/*
 * Calls the fact.s assembly function and prints the result. Takes 
 * a single integer command-line argument to pass to the factorial
 * function. 
 */
int main(int argc, char *argv[]) {
    int arg;
    
    if (argc != 2) {
        fprintf(stderr, "Number of command-line arguments must be 1. \n");
        return 1;
    }
    arg = atoi(argv[1]);
    if (arg < 0) {
        fprintf(stderr, "The command-line argument must be non-negative. \n");
        return 1;
    }
    printf("%d\n", fact(arg));
    return 0;
}
