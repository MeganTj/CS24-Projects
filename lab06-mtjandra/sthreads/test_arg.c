/*
 * A test program that attempts to pass an argument to the thread-functions
 * to ensure that the argument is positioned properly in the thread's machine
 * context.
 * 
 */
#include <stdio.h>
#include "sthread.h"


/*
 * A thread function that takes in a string argument and loops
 * forever.
 */
static void argf1(void *arg) {
    while(1) {
        fprintf(stdout, "Hello, %s \n", ((char *) arg));
        sthread_yield();
    }
}

/*
 * A thread function that takes in a string argument and loops
 * forever
 */
static void argf2(void *arg) {
    while(1) {
        fprintf(stdout, "Greeting is %s \n", ((char *) arg));
        sthread_yield();
    }
}

/* 
 * The main function starts the two loops in separate threads.
 */
int main(int argc, char **argv) {
    char *test1 = "Bob";
    char *test2 = "hello";
    sthread_create(argf1, (void *) test1); 
    sthread_create(argf2, (void *) test2);
    sthread_start();
    return 0;
}
