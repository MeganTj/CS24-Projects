/*
 * A test program that creates multiple threads that run for different
 * lengths of time.
 */
#include <stdint.h>
/*
 * A test program that creates four threads that each run for a different
 * length of time and then terminates the thread  by returning from the
 * thread-function.
 */
#include <stdio.h>
#include "sthread.h"

/*
 * A thread function that takes an argument specifying how many times to
 * to loop before terminating.
 */
static void argf1(void * arg) {
    intptr_t num = (intptr_t) (arg);
    int x = 0;
    while(x < *((int *)  num)) {
        fprintf(stdout, "Alive for %d loops.\n", x + 1);
        x++;
        sthread_yield();
    }
}

/* 
 * Creates four threads that run for different lengths of time.
 */
int main(int argc, char **argv) {
    int x1 = 100;
    int x2 = 200;
    int x3 = 500;
    int x4 = 1000;
    sthread_create(argf1, (void *) &x1);
    sthread_create(argf1, (void *) &x2);
    sthread_create(argf1, (void *) &x3);
    sthread_create(argf1, (void *) &x4);
    sthread_start();
    return 0;
}
