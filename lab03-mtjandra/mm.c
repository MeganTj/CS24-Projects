/*
 * An explicit memory allocator in which free blocks are stored in a 
 * doubly linked list. Each block contains a header and footer that 
 * are both 4 bytes long, which store the size of the block and an allocated
 * bit in the least significant bit. 0 represents a free block and 1
 * represents an allocated block. Within the payload, free blocks
 * store the offsets to the headers of the previous and next blocks. The free list
 * is in last-in first-out order, meaning that blocks that are freed most recently
 * are added to the front of the list.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif 


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4

#define SIZE_PTR(p)  ((unsigned int *)(((char*)(p)) - WSIZE))

/* Retrieves various information about a block, where p is a pointer
 * to the beginning, or the header of the block. 
 */
#define GET_HEADER(p)  *SIZE_PTR(p + WSIZE)

#define SET_HEADER(p, size, alloc)  *SIZE_PTR(p + WSIZE) = PACK(size, alloc);

#define GET_FOOTER(p) *SIZE_PTR(p + GET_SIZE(p))

#define GET_F_SIZE(p) (GET_FOOTER(p) & ~0x7) 

#define SET_FOOTER(p, size, alloc)  *SIZE_PTR(p + size) = PACK(size, alloc);

#define GET_SIZE(p) (GET_HEADER(p) & ~0x7)

#define GET_ALLOC(p) (GET_HEADER(p) & 0x7)

#define GET_PAYLOAD(p) (p + WSIZE)

/* Pack a size and allocated bit into a word. */
#define PACK(size, alloc)  (int) ((size) | (alloc))

/* The size to initialize the heap to. */
#define SIZE_INIT WSIZE

/* Defines the size of an offset. */

#define SIZE_OFFSET WSIZE

/* Computes the offset between a pointer and another pointer. Both
 * pointers point to the headers of free blocks.
 */
#define COMPUTE_OFFSET(this, other)  (signed int)((unsigned char *)(other) - \
                                                  (unsigned char *)(this)) 

/* Retrieves the offset from the current block to previous/next free block. */
#define PREV_OFFSET(p)  *((signed int *)((unsigned char *)(p) + WSIZE))

#define NEXT_OFFSET(p)  *((signed int *)((unsigned char *)(p) + WSIZE + SIZE_OFFSET))

/* The minimum size includes the sizes of the header, footer, and the prev and next pointers.
 */
#define MIN_SIZE (2 * WSIZE + 2 * SIZE_OFFSET)

/* The following are for coalescing. 
 *
 * Retrieves the size and allocated bit  of the block physically before the current block.
 * Retrieves pointers to the blocks physically before and after the current block in the heap. 
 */
#define GET_B_SIZE(p) (*SIZE_PTR(p) & ~0x7)

#define GET_B_ALLOC(p) (*SIZE_PTR(p) & 0x7)

#define GET_BEFORE(p) (unsigned char *)((unsigned char *)(p) - GET_B_SIZE(p))

#define GET_AFTER(p) (unsigned char *)((unsigned char *)(p) + GET_SIZE(p))


/* Start of the heap. */
unsigned char *heap_initp;

/* Start of the free list. */
unsigned char *free_initp;


/*
 * mm_init - Called when a new trace starts.
 * 
 * Created an initial empty heap. The header of the 
 * prologue is initialized such that the size is 0 and the 
 * allocated bit is 1. The prologue also servers as 
 * padding so that the payloads of the heap are 8-byte aligned.        
 */
int mm_init(void) {
    /* Size of prologue set to SIZE_INIT. */
    unsigned char *heap = mem_sbrk(SIZE_INIT);
    SET_HEADER(heap, 0, 1);
    heap_initp = heap + SIZE_INIT;
    free_initp = 0;
    if (*heap == -1) {
        return -1;
    }
    return 0;
}

/*
 * Takes in a pointer to the header of a free block and returns the
 * previous block in the free list by retrieving the offset to the
 * previous block. If the offset is 0, the previous block is 0.
 */
void *get_prev(void *p) {
    if (PREV_OFFSET(p) == 0) {
        return (void *) 0;
    }
    else {
        return ((unsigned char *)((unsigned char *)(p) + PREV_OFFSET(p)));
    }
}

/*
 * Takes in a pointer to the header of a free block and returns the next
 * block in the free list by retrieving the offset to the next
 * block. If the offset is 0, the next block is 0.
 */
void *get_next(void *p) {
    if (NEXT_OFFSET(p) == 0) {
        return (void *) 0;
    }
    else {
        return ((unsigned char *)((unsigned char *)(p) + NEXT_OFFSET(p)));
    }
}

/* 
 * Takes in a pointer to the header of a free block and a pointer
 * to the header of the previous free block. Stores the offset 
 * from the current free block to the previous free block.
 */
void set_prev(void *p, void *newp) {
    if (newp == 0) {
        PREV_OFFSET(p) = 0;
    }
    else {
        PREV_OFFSET(p) = COMPUTE_OFFSET(p, newp);
    }
}

/* 
 * Takes in a pointer to the header of a free block and a pointer
 * to the header of the next free block. Stores the offset 
 * from the current free block to the next free block.
 */
void set_next(void *p, void *newp) {
    if (newp == 0) {
        NEXT_OFFSET(p) = 0;
    }
    else {
        NEXT_OFFSET(p) = COMPUTE_OFFSET(p, newp);
    }
}

/*
 * Takes in p, a pointer to the header of a free block not yet added
 * to the free list. Adds the free block to which p points to the front of
 * the free list.
 */
void add_front(void *p) {
    set_prev(p, 0);
    set_next(p, free_initp);
    if (free_initp != 0) {
        set_prev(free_initp, p);
    }
    free_initp = p;
}

/*
 * Takes in p, a pointer to the header of an allocated block to be removed
 * from the free list. Removes the free block to which p points from the free list.
 */
void remove_free(void *p) {
    if (get_prev(p) == 0 && get_next(p) == 0) {
        free_initp = 0;
        return;
    }
    if (get_prev(p) == 0) {
        unsigned char *next = get_next(p);
        set_prev(next, 0);
        free_initp = next; 
    }
    else if (get_next(p) == 0) {
        unsigned char *prev = get_prev(p);
        set_next(prev, 0);
    }
    else {
        unsigned char *prev = get_prev(p);
        unsigned char *next = get_next(p);
        set_next(prev, next);
        set_prev(next, prev);
    }
}

/*
 * Takes in a pointer to the header of a free block. Uses boundary-tag
 * coalescing to merge the current free block with
 * the adjacent block to the left if that block is free.
 * 
 * Returns a pointer to the beginning of the coalesced block if 
 * coalescing occured, or the original block.
 */
void *coalesce_left(void *curr) {
    if (GET_BEFORE(curr) < (unsigned char *) heap_initp) {
        return curr;
    }
    size_t size = GET_SIZE(curr);
    size_t prev_alloc = GET_B_ALLOC(curr);
    unsigned char *prev = GET_BEFORE(curr);
    if (prev != 0 && prev_alloc == 0) {
        size += GET_SIZE(prev);
        SET_HEADER(prev, size, 0);
        SET_FOOTER(prev, size, 0);
        return prev;
    }
    return curr;
}

/*
 * Takes in a pointer to the header of a free block. Uses boundary-tag
 * coalescing to merge the current free block with
 * the adjacent block to the right if that block is free.
 * 
 * Returns a pointer to the beginning of the coalesced block if 
 * coalescing occured, or the original block.
 */
void *coalesce_right(void *curr) {
    if (GET_AFTER(curr) > (unsigned char *) mem_heap_hi()) {
        return curr;
    }
    size_t size = GET_SIZE(curr);
    unsigned char *next = GET_AFTER(curr);
    size_t next_alloc = GET_ALLOC(next);
    if (next_alloc == 0) {
        size += GET_SIZE(next);
        SET_HEADER(curr, size, 0);
        SET_FOOTER(curr, size, 0);
        remove_free(next);
    }
    return curr;
}

/*
 * Takes in a pointer to the header of a free block. Frees a block
 * Uses boundary-tag coalescing to merge it with any adjacent free blocks
 * in constant time.
 *
 * Returns a pointer to the beginning of the coalesced block if 
 * coalescing occured, or the original block.
 */
void *coalesce(void *curr) {     
    coalesce_right(curr); 
    curr = coalesce_left(curr);
    return curr;
}

/* 
 * Takes in the pointer to the header of the block that will be split
 * into an allocated block and a free block. Takes in the size of the
 * allocated block and the free block, which is at least of minimum size. 
 *
 * Returns the pointer to the payload of the allocated block.
 */

void *split_explicit(void *curr, size_t allocsize, size_t freesize) {
    remove_free(curr);
    SET_HEADER(curr, allocsize, 0x1);
    SET_FOOTER(curr, allocsize, 0x1);
    unsigned char *newblock = curr + allocsize;
    SET_HEADER(newblock, freesize, 0x0);
    SET_FOOTER(newblock, freesize, 0x0);
    add_front(newblock);
    coalesce_right(newblock);
    return GET_PAYLOAD(curr);
}

/*
 * Takes in the 8-bytes aligned size of the requested space on the heap.
 * If a block in the free list is found, removes the block from the free list.
 * Otherwise, expands the heap by the aligned size. 
 * 
 * Returns the payload of the allocated block. 
 */
void *find_fit(size_t newsize) {
    unsigned char *curr = free_initp;
    while (curr != 0 && GET_SIZE(curr) < newsize) {
        curr = get_next(curr);       
    }
    if (curr == 0) {
        unsigned char *p = mem_sbrk(newsize);
        if ((long)p < 0) {
            return NULL;
        }
        else {
            SET_HEADER(p, newsize, 0x1);
            SET_FOOTER(p, newsize, 0x1);
            return GET_PAYLOAD(p);
        }
    }
    int freesize = GET_SIZE(curr) - newsize;
    /* Ensures that the free block from splitting will be of minimum
     * size.
     */
    if (freesize > MIN_SIZE) {
        return (void *) split_explicit((void *) curr, newsize, freesize);
    }
    
    SET_HEADER(curr, GET_SIZE(curr), 0x1);
    SET_FOOTER(curr, GET_SIZE(curr), 0x1);
    remove_free(curr);
    return GET_PAYLOAD(curr);
    
}


/* 
 * Takes in the size of the requested space on the heap and makes it
 * 8-byte aligned. Traverses the explicit free list in search of space
 * to allocate a block.
 *
 * Returns the payload of the allocated block.  
 */
void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    size_t newsize = ALIGN(size + MIN_SIZE);
    unsigned char *block = (unsigned char *)find_fit(newsize);
    return block;
}



/*
 * Takes in a pointer returned by malloc() to the payload of an allocated block. 
 * Frees the allocated block and uses boundary-tag coalescing to merge it with
 * any adjacent free blocks if possible.
 */
void free(void *ptr) {
    unsigned char *pos = ptr;
    if (heap_initp == mem_heap_hi()) {
        return;
    }

    /* Checks if the pointer is NULL, or if it does not point to a payload
     * on the heap.
     */
    if (pos == 0x0 || pos - WSIZE < heap_initp) {
        return;
    }
    
    /* pos now points to the beginning of the block, not its payload. */
    pos -= WSIZE;
    SET_HEADER(pos, GET_SIZE(pos), 0);
    SET_FOOTER(pos, GET_SIZE(pos), 0);
    unsigned char *oldpos = pos;
    pos = coalesce(pos); 
    /*  If the free block has not been added to the free list, add it. */
    if (oldpos == pos) {
        add_front(pos);
    }
}

/* 
 * Takes in the pointer of an allocated block and checks the block
 * physically after to see if it is free. If the combined total
 * of the sizes of these two blocks is greater than or equal
 * to the size necessary, combines these blocks.
 */
void *expand_right(void *curr, size_t asize) {
    if (GET_AFTER(curr) > (unsigned char *) mem_heap_hi()) {
        return curr;
    }
    unsigned char *next = GET_AFTER(curr);
    size_t next_alloc = GET_ALLOC(next);
    size_t size = GET_SIZE(curr) + GET_SIZE(next);
    if (next_alloc == 0 && size >= asize) {
        SET_HEADER(curr, size, 1);
        SET_FOOTER(curr, size, 1);
        remove_free(next);
        return curr;
    }
    return curr;
}


/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.  
 **/
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        free(oldptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(oldptr == NULL) {
        return malloc(size);
    }
    
    size_t asize = ALIGN(size + MIN_SIZE);
    newptr = oldptr - WSIZE;
    newptr = (unsigned char *) expand_right(newptr, asize);
    if (GET_SIZE(newptr) >= asize) {
        return newptr + WSIZE;
    }
    
    newptr = malloc(size);
    
    /* If realloc() fails the original block is left untouched  */
    if(newptr == 0 || !newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = *SIZE_PTR(oldptr);
    if(size < oldsize) oldsize = size;
    memcpy(newptr, oldptr, oldsize);

    /* Free the old block. */
    free(oldptr);
    return newptr;
}

/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc (size_t nmemb, size_t size) {
    size_t bytes = nmemb * size;
    void *newptr;

    newptr = malloc(bytes);
    memset(newptr, 0, bytes);

    return newptr;
}

/*
 * Scans the heap for possible errors. Checks the prologue.
 * Makes sure the headers and footers of each block are identical. 
 * Checks that the free list is doubly linked. Checks the heap 
 * and makes sure the addresses of the payload are aligned.
 */
void mm_checkheap(int verbose) {
    unsigned char *curr = mem_heap_lo();
    
    /* Check prologue block. */
    if ((GET_SIZE(curr) != 0) && (GET_ALLOC(curr) != 1)) {
        fprintf(stderr, "Header of prologue is not size 0 and/or not allocated.  \n");
    }
    curr += WSIZE;
    
    /* Check explicit list. */
    unsigned char *listp = free_initp;
    while (listp != 0) {  
        unsigned char *prev = get_prev(listp);
        if (prev != 0) {
            if (listp != get_next(prev)) {
                fprintf(stderr, "Line: %d. The previous block of the current is not the same as"\
                        " the next block of the previous. \n", verbose);
                return;
            }
        } 
        if (GET_ALLOC(listp) != 0) {
            fprintf(stderr, "Line: %d. An allocated block is in the free list. \n", verbose);
        }
        
        listp = get_next(listp);
    }
    
    /* Check heap. */
    int blocksize = GET_SIZE(curr);
    while (curr < (unsigned char *)mem_heap_hi()) {
        if (((((size_t)(GET_PAYLOAD(curr)))) & 0x7) != 0) {
            fprintf(stderr, "Line: %d. Address %p is not aligned at payload. \n",\
                    verbose, (void *) curr); 
            return;
        }
        int footer = GET_F_SIZE(curr);
        
        if (footer != blocksize) {
            fprintf(stderr, "Line: %d. Size at header and size at footer are not the same.\
 Size at header: %d. Size at footer: %d \n", (int) verbose, blocksize, footer);
            return;
        } 
        curr += blocksize;
        blocksize = GET_SIZE(curr);
        
    }
    /* Check that the end of the heap is reached.*/
    if (curr != (unsigned char *) mem_heap_hi() + 1) {
        fprintf(stderr, "Line: %d. Does not end at proper end address. \n", verbose);
        return;
    }
}
