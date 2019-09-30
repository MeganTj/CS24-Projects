/*============================================================================
 * Implementation of the aging page replacement policy.
 *
 * We don't mind if paging policies use malloc() and free(), just because it
 * keeps things simpler.  In real life, the pager would use the kernel memory
 * allocator to manage data structures, and it would endeavor to allocate and
 * release as little memory as possible to avoid making the kernel slow and
 * prone to memory fragmentation issues.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "virtualmem.h"
#include "vmpolicy.h"

/* Stores the age value in a set number of bits. We can set this value to 8, 
 * 16, or 32 bits.
 */
#define SIZE_AGE 8

/*============================================================================
 * "Loaded Pages" Data Structure
 *
 * This data structure records all pages that are currently loaded in the
 * virtual memory.
 */

typedef struct loaded_pages_t {
    /* The maximum number of pages that can be resident in memory at once. */
    int max_resident;
    
    /* The number of pages that are currently loaded.  This can initially be
     * less than max_resident.
     */
    int num_loaded;
    
    /* This is the array of pages that are actually loaded.  Note that only the
     * first "num_loaded" entries are actually valid.
     */
    page_t pages[];
} loaded_pages_t;


/*============================================================================
 * Policy Implementation
 */


/* The list of pages that are currently resident. */
static loaded_pages_t *loaded;

/* Array that contains the age of each resident page in either 8, 16, or 32 
 *  bits. Only the first "num_loaded" entries are actually valid.
 */
static uint32_t *age_table;

/* Initialize the policy.  Return nonzero for success, 0 for failure. */
int policy_init(int max_resident) {
    fprintf(stderr, "Using RANDOM eviction policy.\n\n");
    
    loaded = malloc(sizeof(loaded_pages_t) + max_resident * sizeof(page_t));
    if (loaded) {
        loaded->max_resident = max_resident;
        loaded->num_loaded = 0;
    }
    age_table = malloc(max_resident * sizeof(uint32_t));
    /* Return nonzero if initialization succeeded. */
    return (loaded != NULL);
}

/* Shifts the page's "age" value right by 1 bit and sets the topmost bit to the 
 * page's current "accessed" value. If the page's "accessed" bit was set, it is
 * also cleared.
 */
void shift_and_set(int num) {
    assert(num < loaded->max_resident);
    int accessed = is_page_accessed(loaded->pages[num]);
    if (accessed != 0) {
        accessed = 1;
    }
    age_table[num] = ((age_table[num] >> 1) | (accessed << (SIZE_AGE - 1)));
    if (accessed == 1) {
        clear_page_accessed(loaded->pages[num]);
        set_page_permission(loaded->pages[num], PAGEPERM_NONE);
    }
}

/* Clean up the data used by the page replacement policy. */
void policy_cleanup(void) {
    free(loaded);
    free(age_table);
    loaded = NULL;
}


/* This function is called when the virtual memory system maps a page into the
 * virtual address space.  Record that the page is now resident.
 */
void policy_page_mapped(page_t page) {
    assert(loaded->num_loaded < loaded->max_resident);
    loaded->pages[loaded->num_loaded] = page;
    /* Set the topmost bit of the "age" value to 1. */
    age_table[loaded->num_loaded] = (1 << (SIZE_AGE - 1)); 
    loaded->num_loaded++;
}

/* This function is called when the virtual memory system has a timer tick. */
void policy_timer_tick(void) {
    /* On a timer interval, the policy traverses all pages. */
    for (int i = 0; i < loaded->num_loaded; i++) {
        shift_and_set(i);
    }
}


/* Choose a page to evict from the collection of mapped pages.  Then, record
 * that it is evicted.  This is very simple since we are implementing a random
 * page-replacement policy.
 */
page_t choose_and_evict_victim_page(void) {
    int i_victim = 0;
    page_t victim = loaded->pages[i_victim];

    /* The page with the lowest "age" value is chosen to be evicted. */
    for (int i = 0; i < loaded->num_loaded; i++) {
        if(age_table[i] < age_table[i_victim]) {
            i_victim = i;
            victim = loaded->pages[i];
        }
    }

    /* Shrink the collection of loaded pages now, by moving the last page in 
     * the collection into the spot that the victim just occupied. The age
     * table is updated as well.
     */
    loaded->num_loaded--;
    loaded->pages[i_victim] = loaded->pages[loaded->num_loaded];
    age_table[i_victim] = age_table[loaded->num_loaded];

#if VERBOSE
    fprintf(stderr, "Choosing victim page %u to evict.\n", victim);
#endif

    return victim;
}

