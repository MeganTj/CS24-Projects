/*! \file
 * Declarations for a simple memory allocator.  The allocator manages a small
 * pool of memory, provides memory chunks on request, and reintegrates freed
 * memory back into the pool.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2009.
 * All rights reserved.
 */

#ifndef IMPALLOC_H
#define IMPALLOC_H

#include <stdbool.h>

#include "types.h"

/* Returns true if an address is within the pool; false otherwise. */
bool is_pool_address(void *addr);

/* Initializes allocator state, and memory pool state too. */
void mm_init(int memory_size);

/* Attempt to allocate a value from the implicit allocator. */
Value *mm_malloc(ValueType type, int data_size);

/* Dereference a Reference into its corresponding Value. */
Value *deref(Reference ref);


/* Return the amount of used memory. */
int memuse(void);

/* Print all allocated objects and free regions in the pool. */
void memdump(void);

/* Runs the garbage collector to reclaim unused space. */
int collect_garbage(void);

/* Clean up the allocator and memory pool state. */
void mm_cleanup(void);


#endif /* IMPALLOC_H */
