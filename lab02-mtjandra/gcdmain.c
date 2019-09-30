#include <stdio.h>
#include <stdlib.h>
#include "gcd.h"

/*
 * Calls the gcd.s assembly function and prints the result. Takes 
 * in two non-negative integer command-line arguments to pass to the gcd
 * function. 
 */
int main(int argc, char *argv[]) {
    int arg1;
    int arg2;
    if (argc != 3) {
        fprintf(stderr, "Number of command-line arguments must be 2. \n");
        return 1;
    }
    arg1 = atoi(argv[1]);
    arg2 = atoi(argv[2]);
    if (arg1 < 0 || arg2 < 0) {
        fprintf(stderr, "Both command-line arguments must be non-negative. \n");
        return 1;
    }
    if (arg1 > arg2) {
        printf("%d\n", gcd(arg1, arg2));
    }
    else {
        printf("%d\n", gcd(arg2, arg1));
    }
    return 0;
}
