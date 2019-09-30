/*! \file
 *
 * This file contains definitions for an Arithmetic/Logic Unit of an
 * emulated processor.
 */


#include <stdio.h>
#include <stdlib.h>   /* malloc(), free() */
#include <string.h>   /* memset() */

#include "alu.h"
#include "instruction.h"


/*!
 * This function dynamically allocates and initializes the state for a new ALU
 * instance.  If allocation fails, the program is terminated.
 */
ALU * build_alu() {
    /* Try to allocate the ALU struct.  If this fails, report error then exit. */
    ALU *alu = malloc(sizeof(ALU));
    if (!alu) {
        fprintf(stderr, "Out of memory building an ALU!\n");
        exit(11);
    }

    /* Initialize all values in the ALU struct to 0. */
    memset(alu, 0, sizeof(ALU));
    return alu;
}


/*! This function frees the dynamically allocated ALU instance. */
void free_alu(ALU *alu) {
    free(alu);
}


/*!
 * This function implements the logic of the ALU.  It reads the inputs and
 * opcode, then sets the output accordingly.  Note that if the ALU does not
 * recognize the opcode, it should simply produce a zero result.
 */
void alu_eval(ALU *alu) {
    uint32_t A, B, aluop;
    uint32_t result;
    uint32_t mask;

    A = pin_read(alu->in1);
    B = pin_read(alu->in2);
    aluop = pin_read(alu->op);

    result = 0;

    /*======================================*/
    /* TODO:  Implement the ALU logic here. */
    /*======================================*/

    switch (aluop) {
        
    case ALUOP_ADD:
        /* Adds two inputs. */
        result = A + B;
        break;

    case ALUOP_INV:
        /* Finds the inverse of the input. */
        result = ~A;
        break;

    case ALUOP_SUB:
        /* Subtracts two inputs. */
        result = A - B;
        break;
        
    case ALUOP_XOR:
        /* Finds the XOR of two inputs. */
        /* result = (~A & B) | (A & ~B);*/
        result = A ^ B;
        break;

    case ALUOP_OR:
        /* Does OR on two inputs. */
        result = A | B;
        break;

    case ALUOP_INCR:
        /* Increments the input. */
        result = ++A;
        break;

    case ALUOP_AND:
        /* Does AND on two inputs. */
        result = A & B;
        break;

    case ALUOP_SRA:
        /* Shifts the input one bit to the right and maintains
         * its most significant bit.
         */
        mask = A & (1 << 31);
        result = (A >> 1) | mask;
        break;

    case ALUOP_SRL:
        /* Shifts the input one bit to the right and sets its most
         * significant bit to 0. 
         */
        result = A >> 1;
        break;

    case ALUOP_SLA:
        /* Shifts the input one bit to the left. */
        result = A << 1;
        break;

    case ALUOP_SLL:
        /* Shifts the input one bit to the left. */
        result = A << 1;
        break;
        
    default:
        /* Sets the output to 0 if the operation is not recognized. */
        result = 0;
        break;
    }
    pin_set(alu->out, result);
}

