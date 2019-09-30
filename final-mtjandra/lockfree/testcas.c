/* Tests to ensure compare_and_swap works properly. */

#include <stdio.h>
#include "cmp_swap.h"

/* Test 1 verifies that *target has changed when *target and the old parameter
 * are the same, and that compare_and_swap return 1, which means that the value
 * of *target has been modified. Test 2 verifies that when *target and old are
 * not the same, *target is not modified and the test returns 0 to indicate
 * this. 
 */
int main() {
    uint32_t target = 5;
    uint32_t old_val = 5;
    uint32_t new_val = 3;
    printf("Test 1: Value of target is %d and value of old is %d. Testing that"
           " the value of target has changed to %d. \n", (int) target, \
           (int) old_val, (int) new_val);
    int test1 = compare_and_swap(&target, old_val, new_val);
    if (target == new_val) {
        printf("Pass: Value of target was successfully replaced.\n");
    }
    else {
        fprintf(stderr, "Error: Value of target was not successfully replaced."
                " target's value is %d, expected is %d. \n", \
                (int) target, (int) new_val);
    }
    if (test1 == 1) {
        printf("Pass: Return value of compare and swap is correct.\n");
    }
    else {
        fprintf(stderr, "Error: Return value of compare and swap is %d, "
                    "expected is 1.\n", test1);
    }
    target = 9;
    old_val = 7;
    new_val = 4;
    printf("\nTest 2: Value of target is %d and value of old is %d. Testing "
           "that the value of target does not change to %d.\n", (int) target, \
           (int) old_val, (int) new_val);
    int test2 = compare_and_swap(&target, old_val, new_val);
    if (target == 9) {
        printf("Pass: Value of target was kept the same.\n");
    }
    else {
        fprintf(stderr, "Error: Value of target was not kept the same. "
                "target's value is %d, expected is 9. \n", (int) target);
    }
    if (test2 == 0) {
        printf("Pass: Return value of compare and swap is correct.\n");
    }
    else {
        fprintf(stderr, "Error: Return value of compare and swap is %d, "
                    "expected is 0.\n", test2);
    }
    return 0; 
}
