#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "ffunc.h"
#define BASE_CASE 1

/* This function takes an array of single-precision floating point values,
 * and computes a sum in the order of the inputs.  Very simple.
 */
float fsum(FloatArray *floats) {
    float sum = 0;
    int i;

    for (i = 0; i < floats->count; i++)
        sum += floats->values[i];

    return sum;
}

/*
 * Helper function that computes the sum of floats using pairwise
 * summation.
 */
float fsum_helper(FloatArray *floats, int start, int end) {
    int n = end - start;
    float sum = 0;
    float *arr = floats->values;
    int split;
    int i;
    /*
     * Base Case: Sums up floats within the array naively if the range of
     * indexes is small enough (in this case, the range is 1).
     */
    if (n <= BASE_CASE)
    {
        sum = arr[start];
        for (i = start + 1; i < end; i++)
        {
            sum += arr[i];
        }
    }
    /*
     * Recursive case: Computes midpoint of the range, splits the range
     * in half, and recursively computes the sums over the two halves.
     */
    else
    {
        split = n / 2;
        sum = fsum_helper(floats, start, start + split) + \
            fsum_helper(floats, start + split, end);
    }
    return sum;
}


/* TODO:  IMPLEMENT my_fsum().  MAKE SURE TO EXPLAIN YOUR CODE IN COMMENTS. */
float my_fsum(FloatArray *floats) {
    return fsum_helper(floats, 0, floats->count);
}


int main() {
    FloatArray floats;
    float sum1, sum2, sum3, my_sum;

    load_floats(stdin, &floats);
    printf("Loaded %d floats from stdin.\n", floats.count);

    /* Compute a sum, in the order of input. */
    sum1 = fsum(&floats);

    /* Use my_fsum() to compute a sum of the values.  Ideally, your
     * summation function won't be affected by the order of the input floats.
     */
    my_sum = my_fsum(&floats);

    /* Compute a sum, in order of increasing magnitude. */
    sort_incmag(&floats);
    sum2 = fsum(&floats);

    /* Compute a sum, in order of decreasing magnitude. */
    sort_decmag(&floats);
    sum3 = fsum(&floats);

    /* %e prints the floating-point value in full precision,
     * using scientific notation.
     */
    printf("Sum computed in order of input:  %e\n", sum1);
    printf("Sum computed in order of increasing magnitude:  %e\n", sum2);
    printf("Sum computed in order of decreasing magnitude:  %e\n", sum3);

    /* TODO:  UNCOMMENT WHEN READY! */
    printf("My sum:  %e\n", my_sum);

    free(floats.values);
    return 0;
}

