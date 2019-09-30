/*! \file
 * Implementation of a simple memory allocator.  The allocator manages a small
 * pool of memory, provides memory chunks on request, and reintegrates freed
 * memory back into the pool.
 *
 * This implementation uses the stop-and-copy algorithm, which divides the
 * memory pool into half and copies over Values that are not garbage to the to
 * pool when the program runs out of memory.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2010.
 * All rights reserved.
 */

#include "alloc.h"

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "eval.h"


//// MODULE-LOCAL STATE ////


/*!
 * Specifies the size of the memory pool.  This is a static local variable;
 * the value is specified in the call to init_alloc(). 
 */
static int MEMORY_SIZE;

/*
 * Specififies how large half the size pool is.
 */
static int HALF_MEMORY;

/*!
 * This is the starting address of the memory pool used in the implicit
 * allocator.  The pool is allocated within init_alloc().
 */
static unsigned char *mem;


/*!
 * The implicit allocator uses an external "free-pointer" to track where free
 * memory starts.  We can get away with this approach because our allocator
 * compacts memory towards the start of the pool during garbage collection.
 */
static unsigned char *freeptr;

/*!
 * Pointer to the from pool of the memory.
 */
static unsigned char *fromptr;

/*!
 * Pointer to the to pool of the memory.
 */
static unsigned char *toptr;

/*! 
 * Keeps track of where we're currently at in the to space while copying 
 * Values from the from space.
 */
static unsigned char *allocptr;


/*!
 * This is the "reference table."  However, it is really just an array that
 * records where each Value starts in the pool.  References are just indexes
 * into this table.  An unused slot is indicated by storing NULL for the
 * Value pointer.  (Since it's an array of pointers, it's a pointer to a
 * pointer.)
 */
static struct Value **ref_table;

/*!
 * This is the number of references currently in the table.  Valid entries
 * are in the range 0 .. num_refs - 1.
 */
static int num_refs;

/*! This is the actual size of the ref_table. */
static int max_refs;


//// LOCAL HELPER FUNCTIONS ////


Reference make_reference();


//// FUNCTION DEFINITIONS ////


/*!
 * This function initializes both the allocator state, and the memory pool.  It
 * must be called before myalloc() or myfree() will work at all.
 *
 * Note that we allocate the entire memory pool using malloc().  This is so we
 * can create different memory-pool sizes for testing.  Obviously, in a real
 * allocator, this memory pool would either be a fixed memory region, or the
 * allocator would request a memory region from the operating system (see the
 * C standard function sbrk(), for example).
 */
void mm_init(int memory_size) {
    /*
     * Allocate the entire memory pool, from which our simple allocator will
     * serve allocation requests.
     */
    assert(memory_size > 0);
    MEMORY_SIZE = memory_size;
    mem = malloc(MEMORY_SIZE);

    if (mem == NULL) {
        fprintf(stderr,
                "init_malloc: could not get %d bytes from the system\n",
                MEMORY_SIZE);
        abort();
    }

    /* 
     * Modification to split the memory into the from and to pools. At first,
     * the from pool is the first half of the memory and the to pool is the
     * second. However, these pointers will swap when the from pool is
     * full.
     */
    HALF_MEMORY = MEMORY_SIZE / 2;
    fromptr = mem;
    freeptr = fromptr;
    toptr = mem + HALF_MEMORY;
    allocptr = toptr;

    /* Start out with no references in our reference-table. */
    ref_table = NULL;
    num_refs = 0;
    max_refs = 0;


}


/*! Returns true if the specified address is within the memory pool. */
bool is_pool_address(void *addr) {
    return ((unsigned char *) addr >= mem &&
            (unsigned char *) addr < mem + MEMORY_SIZE);
}


/*! Returns true if the pool has the requested amount of space available. */
bool has_space_available(int requested) {
    /*
     * Checks if the freeptr + requested doesn't exceed the position of the
     * to pointer if the from pool is at lower addresses compared to the
     * to pool. Otherwise checks if free + requested doesn't exceed the
     * bounds of memory.
     */
    if (fromptr < toptr) {
        return (freeptr + requested <= toptr);
    }
    return (freeptr + requested <= mem + MEMORY_SIZE);
}


/*!
 * Attempt to allocate a chunk of memory of "size" bytes.  Return 0 if
 * allocation fails.
 */
Value * mm_malloc(ValueType type, int data_size) {
    // Actually, this should always be > 0 since even empty strings need a
    // NUL-terminator.  But, we will stick with >= 0 for now.
    assert(data_size >= 0);

    if (type == VAL_INTEGER) {
        data_size = sizeof(IntegerValue) - sizeof(struct Value);
    } else if (type == VAL_FLOAT) {
        data_size = sizeof(FloatValue) - sizeof(struct Value);
    } else if (type == VAL_LIST_NODE) {
        data_size = sizeof(ListValue) - sizeof(struct Value);
    } else if (type == VAL_DICT_NODE) {
        data_size = sizeof(DictValue) - sizeof(struct Value);
    }

    int requested = sizeof(struct Value) + data_size;
    Value *new_value = NULL;

    // If we don't have space, this might work.
    if (!has_space_available(requested))
        collect_garbage();

    if (has_space_available(requested)) {

        /* Initialize the new Value in the bytes beginning at freeptr. */
        new_value = (Value *) freeptr;

        /* Assign a Reference to it; the Value will know its Reference. */
        make_reference(new_value);

        new_value->type = type;
        new_value->data_size = data_size;

        /* Set the data area to a pattern so that it's easier to debug. */
        memset(new_value + 1, 0xCC, data_size);

        /* Update the free pointer to point past the new Value. */
        freeptr += requested;
    } else {
        /* fromptr was previously mem */
        fprintf(stderr, "mm_malloc: cannot service request of size %d with"
                " %d bytes allocated\n", requested, (int) (freeptr - fromptr));
        exit(1);
    }

    return new_value;
}


/*! Allocates an available reference in the ref_table. */
Reference make_reference(Value *value) {
    int i;
    Reference ref;
    Value **new_table;

    assert(value != NULL);

    /* If we don't have a reference table yet, allocate one. */
    if (ref_table == NULL) {
        ref_table = malloc(sizeof(Value *) * INITIAL_SIZE);
        max_refs = INITIAL_SIZE;

        // Set all new reference entries to NULL, just to be safe/clean.
        for (i = 0; i < max_refs; i++) {
            ref_table[i] = NULL;
        }
    }

    /* Scan through the reference table to see if we have any unused slots
     * that we can use for this value.
     */
    for (i = 0; i < num_refs; i++) {
        if (ref_table[i] == NULL) {
            ref = (Reference) i;  // Probably unnecessary, but clearer.
            ref_table[i] = value;
            value->ref = ref;
            return ref;
        }
    }

    /* If we got here, we don't have any available slots.  Find out if
     * this is because we ran out of space in the reference table.
     */

    if (num_refs == max_refs) {
        /* Double the size of the reference table. */
        max_refs *= 2;
        new_table = realloc(ref_table, sizeof(Value *) * max_refs);
        if (new_table == NULL) {
            error("out of memory");
            exit(1);
        }
        ref_table = new_table;

        // Set all new reference entries to NULL, just to be safe/clean.
        for (i = num_refs; i < max_refs; i++) {
            ref_table[i] = NULL;
        }
    }

    /* This becomes the new reference. */
    ref = (Reference) num_refs;  // Probably unnecessary, but clearer.
    num_refs++;

    ref_table[ref] = value;
    value->ref = ref;
    return ref;
}


/*!
 * Dereferences a Reference into a Value-pointer so the value can be
 * accessed.
 *
 * A Reference of NULL_REF will cause this function to return NULL.
 */
Value * deref(Reference ref) {
    Value *pval = NULL;

    if (ref == NULL_REF)
        return NULL;

    // Make sure the reference is actually a valid index.
    assert(ref >= 0 && ref < num_refs);

    // Make sure the reference refers to a valid entry.  Unused entries
    // will be set to NULL.
    pval = ref_table[ref];
    assert(pval != NULL);

    // Make sure the reference's value is within the pool!
    assert(is_pool_address(pval));

    return pval;
}


/*! Get the amount of in-use memory. */
int memuse() {
    /* fromptr was previously mem */
    return freeptr - fromptr;
}


/*! Print all allocated objects and free regions in the pool. */
void memdump() {
    unsigned char *curr = fromptr;
    /*unsigned char *curr = toptr;*/
    while (curr < freeptr) {
        Value *curr_value = (Value *) curr;
        int data_size = curr_value->data_size;
        int value_size = sizeof(Value) + data_size;
        Reference ref = curr_value->ref;
        int seen = curr_value->seen;

        fprintf(stdout, "Value 0x%08x; size %d; ref %d; seen %d; ",
                (int) (curr - mem), (int) sizeof(Value) + data_size, ref, seen);

        switch (curr_value->type) {
            case VAL_NONE:
                fprintf(stdout, "type = VAL_NONE; value = None\n");
                break;

            case VAL_BOOL:
                fprintf(stdout, "type = VAL_BOOL; value = %s\n",
                            ref_is_true(ref) ? "True" : "False");
                break;

            case VAL_INTEGER:
                fprintf(stdout, "type = VAL_INTEGER: value = %d\n",
                    ((IntegerValue *) curr_value)->integer_value);
                break;

            case VAL_FLOAT:
                fprintf(stdout, "type = VAL_FLOAT; value = %f\n",
                    ((FloatValue *) curr_value)->float_value);
                break;

            case VAL_STRING:
                fprintf(stdout, "type = VAL_STRING; value = \"%s\"\n",
                    ((StringValue *) curr_value)->string_value);
                break;

            case VAL_LIST_NODE: {
                ListValue *lv = (ListValue *) curr_value;
                fprintf(stdout,
                    "type = VAL_LIST_NODE; value_ref = %d; next_ref = %d\n",
                    lv->list_node.value, lv->list_node.next);
                break;
            }

            case VAL_DICT_NODE: {
                DictValue *dv = (DictValue *) curr_value;
                fprintf(stdout,
                    "type = VAL_DICT_NODE; key_ref = %d; value_ref = %d; "
                    "next_ref = %d\n",
                    dv->dict_node.key, dv->dict_node.value, dv->dict_node.next);
                break;
            }

            default:
                fprintf(stdout,
                        "type = UNKNOWN; the memory pool is probably corrupt\n");
        }

        curr += value_size;
    }
    /* fromptr was previously mem */
    fprintf(stdout, "Free  0x%08x; size %lu\n", (int) (freeptr - fromptr),
        HALF_MEMORY - (freeptr - fromptr));
}


//// GARBAGE COLLECTOR ////

/*!
 * Takes in a Value and returns its size.
 */
int get_size(Value *val) {
    return sizeof(Value) + val->data_size;
}

/*!
 * Copies a Value to where allocptr points to. Checks whether the Value
 * should be copied to the to pool. After copying, updates the reference
 * table and increments allocptr by the size of the value.
 *
 * Returns -1 if the Value was not copied over to the to pool and 0 if
 * copying occurred.
 *
 * We use memcpy and not memmove because memcpy always copies addresses
 * in the same order. On the other hand, memmove checks whether the
 * destination overlaps with the source and copies in the other
 * direction if this is is the case. Since we know our to and from
 * pools do not overlap, we use memcpy, which is more efficient
 * since it does not check for overlap. 
 */
int copy_wrapper(Value *val) {
    /* To break cycles. */
    int value_size = get_size(val);
    Reference ref = val->ref;
    /* 
     * Checks if the address of the Value is in the to pool. If it is,
     * it is not copied in order to prevent cycles.
     */
    if (toptr > fromptr && (unsigned char *) ref_table[ref] > toptr) {
        return -1;
    }
    if (toptr < fromptr && (unsigned char *) ref_table[ref] < fromptr) {
        return -1;
    }
    /* Checks that we're not trying to copy over something 
     * that's past the freeptr.
     */
    if ((unsigned char *) ref_table[ref] > freeptr) {
        return -1;
    }
    memcpy(allocptr, ref_table[ref], value_size);
    val->seen = 1;
    ref_table[ref] = (Value *) allocptr;
    allocptr += value_size;
    return 0;
}

/*!
 * Copies over a Value.
 */
void copy_val(Value *val);

/*!
 * Copies a list from start to end, including all of its elements
 * and its values. To copy a ListValue, we first copy the Value. 
 * We then copy the actual value contained in the ListNode and 
 * check if there is a next element.
 */
void copy_list(Value *val) {
    ListValue *curr_val = (ListValue *) val;
    ListNode curr_node = curr_val->list_node;

    /* Checks if the Value was able to be copied to the to pool. */
    if (copy_wrapper((Value *)curr_val) == -1) {
        return;
    }
    Reference val_ref = curr_node.value;
    
    /* Checks if the value in the ListNode can be copied.*/
    if (val_ref != NULL_REF) {
        copy_val((Value *) deref(val_ref));
    }
    Reference next = curr_node.next;
    
    /* If there is a next ListValue, copy it as well. */
    if (next != NULL_REF) {
        copy_list((Value *) deref(next));
    }
}


/*!
 * Copies a dictionary from start to end, including all of its
 * elements and its values. To copy a DictValue, we first copy the Value. 
 * We then copy the key and actual value contained in the DictNode and 
 * check if there is a next element.
 */
void copy_dict(Value *start) {
    DictValue *curr_val = (DictValue *) start;
    DictNode curr_node = curr_val->dict_node;

    /* Checks if the Value was able to be copied to the to pool. */
    if (copy_wrapper((Value *)curr_val) == -1) {
        return;
    }
    Reference key = curr_node.key;

    /* Checks if the key in the DictNode can be copied.*/
    if (key != NULL_REF) {
        copy_val((Value *)deref(key));
    }
    Reference value  = curr_node.value;

    /* Checks if the value in the DictNode can be copied.*/
    if (value != NULL_REF) {
        copy_val((Value *)deref(value));
    }
    Reference next = curr_node.next;

    /* If there is a next ListValue, copy it as well. */
    if (next != NULL_REF) {
        copy_dict((Value *) deref(next));
    }
}

/*!
 * Copies over a Value. Calls recursive functions for ListValue's
 * and DictValue's.
 */
void copy_val(Value *val) {
    ValueType type = val->type;
    if (type == VAL_LIST_NODE) {
        copy_list(val);
    }
    else if (type == VAL_DICT_NODE) {
        copy_dict(val); 
    }
    else {
        copy_wrapper(val);
    }
}

/*!
 * Takes in the name and reference of a global variable and copies 
 * the Value and any connected Values to the to pool.
 */
void copy_global(const char *name, Reference ref) {
    if (ref == NULL_REF) {
        return;
    }
    Value *curr = deref(ref);
    copy_val(curr);

    /* Unused argument. */
    (void) name;
}

/*!
 * Iterates over the from pool and checks the seen field of all the Values.
 * If the seen field of the Value has not been changed by copy_global to 1, 
 * the Value will be garbage collected. We set the reference of this value to
 * NULL.
 */
void iterate_from() {
    unsigned char *curr = fromptr;
    while (curr < freeptr) {
        Value *val = (Value *) curr;
        Reference ref = val->ref;
        int seen = val->seen;
        if (seen == 0) {
            /* 
             * Sets the address in the reference table to NULL 
             * since the reference is not being used.
             */
            ref_table[ref] = NULL;
        }
        curr += get_size(val);
    }
}


/*!
 * First iterates through the global variables and copies all global variables
 * and associated values to the to pool. Also marks values that have been copied
 * over as seen. Iterates through the from pool and sets the references of the 
 * objects that were not copied over to NULL. Sets the from pool to 0's,
 * switches the places of the fromptr and toptr, and updates freeptr and
 * allocptr accordingly.
 */
void stop_and_copy(void) {
    foreach_global(copy_global);
    iterate_from();
    size_t len = 0;
    if (toptr > fromptr) {
        len = toptr - fromptr;
    }
    else {
        len = (mem + MEMORY_SIZE) -fromptr;
    }
    freeptr = allocptr;
    memset(fromptr, 0x0, len);
    unsigned char *oldfrom = fromptr;
    
    /* Switches the places of the from and to pools. */
    fromptr = toptr;
    toptr = oldfrom;
    allocptr = toptr;
}

/*!
 * Collects garbage using the stop and copy method.
 */
int collect_garbage(void) {
    int reclaimed;
    int before = freeptr - fromptr;
    if (!quiet) {
        fprintf(stderr, "Collecting garbage.\n");
    }

    // TODO:  Implement garbage collection.
    stop_and_copy();
    // END TODO
    int after = freeptr - fromptr;
    reclaimed =  before - after;

    if (!quiet) {
        // Ths will report how many bytes we were able to free in this garbage
        // collection pass.
        fprintf(stderr, "Reclaimed %d bytes of garbage.\n", reclaimed);
    }

    return reclaimed;
}


//// END GARBAGE COLLECTOR ////


/*!
 * Clean up the allocator state.
 * All this really has to do is free the user memory pool. This function mostly
 * ensures that the test program doesn't leak memory, so it's easy to check
 * if the allocator does.
 */
void mm_cleanup(void) {
    free(mem);
    mem = NULL;
}

