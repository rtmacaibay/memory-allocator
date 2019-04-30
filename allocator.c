/**
 * allocator.c
 *
 * Explores memory management at the C runtime level.
 *
 * Author: Robert Macaibay and Elijah Delos Reyes
 *
 * To use (one specific command):
 * LD_PRELOAD=$(pwd)/allocator.so command
 * ('command' will run with your allocator)
 *
 * To use (all following commands):
 * export LD_PRELOAD=$(pwd)/allocator.so
 * (Everything after this point will use your custom allocator -- be careful!)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#define LOG(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

#define LOGP(str) \
    do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): %s", __FILE__, \
            __LINE__, __func__, str); } while (0)

struct mem_block {
    /* Each allocation is given a unique ID number. If an allocation is split in
     * two, then the resulting new block will be given a new ID. */
    unsigned long alloc_id;

    /* Size of the memory region */
    size_t size;

    /* Space used; if usage == 0, then the block has been freed. */
    size_t usage;

    /* Pointer to the start of the mapped memory region. This simplifies the
     * process of finding where memory mappings begin. */
    struct mem_block *region_start;

    /* If this block is the beginning of a mapped memory region, the region_size
     * member indicates the size of the mapping. In subsequent (split) blocks,
     * this is undefined. */
    size_t region_size;

    /* Next block in the chain */
    struct mem_block *next;
};

/* Start (head) of our linked list: */
struct mem_block *g_head = NULL;
/* End (tail) of our linked list: */
struct mem_block *g_tail = NULL;

/* Allocation counter: */
unsigned long g_allocations = 0;


pthread_mutex_t g_alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * print_memory
 *
 * Prints out the current memory state, including both the regions and blocks.
 * Entries are printed in order, so there is an implied link from the topmost
 * entry to the next, and so on.
 */
void print_memory(void) {
    puts("-- Current Memory State --");
    struct mem_block *current_block = g_head;
    struct mem_block *current_region = NULL;
    while (current_block != NULL) {
        if (current_block->region_start != current_region) {
            current_region = current_block->region_start;
            printf("[REGION] %p-%p %zu\n",
                    current_region,
                    (void *) current_region + current_region->region_size,
                    current_region->region_size);
        }
        printf("[BLOCK]  %p-%p (%ld) %zu %zu %zu\n",
                current_block,
                (void *) current_block + current_block->size,
                current_block->alloc_id,
                current_block->size,
                current_block->usage,
                current_block->usage == 0
                    ? 0 : current_block->usage - sizeof(struct mem_block));
        current_block = current_block->next;
    }
}

/**
 * write_memory
 *
 * Writes out the current memory state to a file, including both the regions and blocks.
 * Entries are printed in order, so there is an implied link from the topmost
 * entry to the next, and so on.
 */
void write_memory(FILE * fp) {
    fputs("-- Current Memory State --", fp);
    struct mem_block *current_block = g_head;
    struct mem_block *current_region = NULL;
    while (current_block != NULL) {
        if (current_block->region_start != current_region) {
            current_region = current_block->region_start;
            fprintf(fp, "[REGION] %p-%p %zu\n",
                    current_region,
                    (void *) current_region + current_region->region_size,
                    current_region->region_size);
        }
        fprintf(fp, "[BLOCK]  %p-%p (%ld) %zu %zu %zu\n",
                current_block,
                (void *) current_block + current_block->size,
                current_block->alloc_id,
                current_block->size,
                current_block->usage,
                current_block->usage == 0
                    ? 0 : current_block->usage - sizeof(struct mem_block));
        current_block = current_block->next;
    }
}

/**
 * reuse_space
 * 
 * Helper function that allows us to reuse space mem_block ptr
 * with size real_sz
 *
 * Parameters:
 * - ptr: memory block to reuse
 * - real_sz: usage space to set to
 *
 * Returns: new memory block pointer
 */
void *reuse_space(struct mem_block * ptr, size_t real_sz) {
    //check if our current block can be reused without splitting
    if (ptr->usage == 0) {
        //change usage to real_sz
        ptr->usage = real_sz;
        //copy the old id
        unsigned long old_id = ptr->alloc_id;
        //change alloc_id to new allocation
        ptr->alloc_id = g_allocations++;
        //log msg telling us we're using a certain alloc
        LOG("Allocation request USING reuse; reusing alloc %ld\n", old_id);
        //return the "new" pointer
        return ptr + 1;
    }

    //create a new mem_block that encompasses the rest of the space
    struct mem_block * freeloader = (void*)ptr + ptr->usage;
    //explanatory struct stuff
    freeloader->alloc_id = g_allocations++;
    freeloader->size = ptr->size - ptr->usage;
    freeloader->usage = real_sz;
    freeloader->region_start = ptr->region_start;
    freeloader->region_size = ptr->region_size;

    //update the linked list
    if (g_tail == ptr) {
        freeloader->next = NULL;
        g_tail->next = freeloader;
        g_tail = freeloader;
    } else {
        freeloader->next = ptr->next;
        ptr->next = freeloader;
    }

    //update previous pointer's size
    ptr->size = ptr->usage;

    //log msg to tell us we're reusing space somewhere
    LOG("Allocation request USING reuse;\n size = %zu, alloc = %ld, where?: %ld\n", 
        real_sz, freeloader->alloc_id, ptr->alloc_id);

    //return new pointer
    return freeloader + 1;
}

/**
 * reuse
 * 
 * Allows us to reuse empty regions with size
 *
 * Parameters:
 * - size: size to set block to
 *
 * Returns: memory block pointer with size
 */
void *reuse(size_t size) {
    //if g_head is NULL, we got nothing to reuse
    if (g_head == NULL) {
        return NULL;
    }

    //check what kind of algorithm we're using
    char * algo = getenv("ALLOCATOR_ALGORITHM");
    if (algo == NULL) {
        algo = "first_fit";
    }

    //keeps track of size that fits our criteria
    size_t fits_best = SIZE_MAX;
    size_t fits_worst = 0;

    //keeps track of the pointer we want to reuse
    struct mem_block * the_spot = NULL;

    //curr to iterate thru linked list
    struct mem_block * curr = g_head;

    //gather real_sz
    size_t real_sz = size + sizeof(struct mem_block);

    //iterate thru linked list until we're done or find something
    while (curr != NULL) {
        //this is just the free space of a current block
        size_t free_space = curr->size - curr->usage;

        //check for what kind of algorithm we want
        if (strcmp(algo, "first_fit") == 0) {
            //if there's space, take it
            if (curr->usage < curr->size && real_sz <= free_space) {
                return reuse_space(curr, real_sz);
            }
        } else if (strcmp(algo, "best_fit") == 0) {
            //if there's space, check if it fits best
            if (curr->usage < curr->size && real_sz <= free_space) {
                if (free_space < fits_best) {
                    fits_best = free_space;
                    the_spot = curr;
                }
            }
        } else if (strcmp(algo, "worst_fit") == 0) {
            //if there's space, check if it has a lot of free space
            if (curr->usage < curr->size && real_sz <= free_space) {
                if (free_space > fits_worst) {
                    fits_worst = free_space;
                    the_spot = curr;
                }
            }
        }

        //thank u, next
        curr = curr->next;
    }

    //if this isn't NULL, that means we got something
    if (the_spot != NULL) {
        return reuse_space(the_spot, real_sz);
    }

    //we return NULL when we can't reuse
    return NULL;
}

/**
 * scribble_this
 * 
 * Literally scribbles the memory in a pointer
 *
 * Parameters:
 * - ptr: pointer we want to scribble
 * - size: bytes to scribble
 */
void scribble_this(void * ptr, size_t sz) {
    memset(ptr, 0xAA, sz);
}

/**
 * malloc
 * 
 * Our own implementation of malloc that allocates memory
 *
 * Parameters:
 * - size: bytes to allocate
 *
 * Returns memory block pointer with allocated memory.
 */
void *malloc(size_t size) {
    //lock immediately lul
    pthread_mutex_lock(&g_alloc_mutex);

    //check if we should scribble
    char * s = getenv("ALLOCATOR_SCRIBBLE");
    int scribble = 0;
    if (s != NULL) {
        scribble = atoi(s);
    }

    /* Re-align the size */
    if (size % 8 != 0) {
        size = (((int) size / 8) * 8) + 8;
    }

    //check if we can reuse something
    void * ptr = reuse(size);
    if (ptr != NULL) {
        //noice we could reuse space 
        //scribble if we want to
        if (scribble) {
            scribble_this(ptr, size);
        }

        //unlock mutex
        pthread_mutex_unlock(&g_alloc_mutex);
        return ptr;
    }
    //log msg to let us know there's a new request
    LOG("Allocation request; size = %zu\n", size);

    /* How much space we are using in the region */
    size_t real_sz = size + sizeof(struct mem_block);

    //LOG("Allocation request; real size = %zu, alloc = %ld\n", real_sz, g_allocations);

    /* Size per page */
    int page_sz = getpagesize();

    /* Number of pages we are using */
    size_t num_pages = real_sz / page_sz;
    /* If there is a remainder, we need an extra page*/
    if (real_sz % page_sz) {
        num_pages++;
    }

    /* Size of entire region */
    size_t region_sz = num_pages * page_sz;

    /* Set up memory block */
    struct mem_block * block = mmap(
            NULL, /* Address (we use NULL to let the kernel decide) */
            region_sz, /* Size of memory block to allocate */
            PROT_READ | PROT_WRITE, /* Memory protection flags */
            MAP_PRIVATE | MAP_ANONYMOUS, /* Type of mapping */
            -1, /* file descriptor */
            0 /* offset to start at within the file */);

    //if MAP_FAILED, rip
    if (block == MAP_FAILED) {
        pthread_mutex_unlock(&g_alloc_mutex);
        perror("mmap");
        return NULL;
    }

    //struct stuff, don't really care
    block->alloc_id = g_allocations++;
    block->size = region_sz;
    block->usage = real_sz;
    block->region_start = block;
    block->region_size = region_sz;
    block->next = NULL;

    /* Add to linked list */
    if (g_head == NULL) {
        g_head = block;
        g_tail = block;
    } else {
        g_tail->next = block;
        g_tail = g_tail->next;
        g_tail->next = NULL;
    }

    //scribble?
    if (scribble) {
        scribble_this(block + 1, size);
    }

    //unlock
    pthread_mutex_unlock(&g_alloc_mutex);

    //haha block + 1 bc we don't want user to write over struct
    return block + 1;
}

/**
 * destroy_this
 * 
 * Helper function to destroy memory block pointer
 *
 * Parameters: 
 * - ptr: memory block pointer to destroy
 *
 * Returns void
 */
void destroy_this(struct mem_block * ptr) {
    //munmap that pointer
    int ret = munmap(ptr->region_start, ptr->region_size);
    if (ret == -1) {
        perror("munmap");
    }

    //log msg bc i want to know if we destroyed something
    LOGP("Free request; Destroyed a pointer\n");
}

/**
 * free
 * 
 * Our own implementation of free that deallocated the memory allocation
 * pointed to by ptr
 *
 * Parameters: 
 * - ptr: memory to deallocate
 *
 * Returns: void
 */
void free(void *ptr) {
    //lock immediately lul
    pthread_mutex_lock(&g_alloc_mutex);

    //we can't free something that isn't there LMAO
    if (ptr == NULL) {
        pthread_mutex_unlock(&g_alloc_mutex);
        return;
    }

    //get the struct
    struct mem_block * blk = (struct mem_block *) ptr - 1;
    //log msg telling us we are removing something
    LOG("Free request; size = %zu, alloc = %lu\n", blk->usage, blk->alloc_id);
    
    //update usage to 0
    blk->usage = 0;

    //keep track of region start
    struct mem_block * orig_start = blk->region_start;
    //prev to iterate thru linked list
    struct mem_block * prev = g_head;
    //keep track of linked list parts to connect them
    struct mem_block * first_half = NULL;
    struct mem_block * sec_half = NULL;
    //first node or nah
    int first_node = 0;

    //if first node is our block
    if (prev->region_start == orig_start) {
        first_node = 1;
        //wouldn't it be crazy if the first node had something
        if (prev->usage != 0) {
            pthread_mutex_unlock(&g_alloc_mutex);    
            return;
        }
    }

    //iterate thru linked list
    while (prev->next != NULL) {
        //get next block
        struct mem_block * curr = prev->next;
        //check if same region as block
        if (curr->region_start == orig_start) {
            //yo if we dont have a first half
            if (first_half == NULL) {
                //save previous link
                first_half = prev;
                continue;
            } 

            if (curr->usage != 0) {
                //log msg with explanatory msg
                LOGP("Free request; Completed - block still in use\n");
                pthread_mutex_unlock(&g_alloc_mutex);
                return;
            }
        } else if (first_half != NULL) {
            //if we get a new region, save that as our other half
            sec_half = curr;
            break;
        }

        //thank u, next
        prev = prev->next;
    }

    //this is to update our linked list yuuuhh
    if (first_node) {
        if (sec_half == NULL) {
            /* if we in here, remove entire linked list */
            destroy_this(blk);
            g_head = NULL;
            g_tail = NULL;
        } else {
            /* if we in here, remove top half of linked list */
            destroy_this(blk);
            g_head = sec_half;
        }
    } else if (first_half != NULL) {
        if (sec_half == NULL) {
            /* if we in here, remove bottom half of linked list */
            destroy_this(blk);
            g_tail = first_half;
            g_tail->next = NULL;
        } else {
            /* if we in here, connect halves of linked list */
            destroy_this(blk);
            first_half->next = sec_half;
        }
    } else {
        perror("free");
    }

    //unlock
    pthread_mutex_unlock(&g_alloc_mutex);

    //pls let me know if we survived free
    LOGP("Free request; Completed\n");
}

/**
 * calloc
 *
 * Our own implementation of calloc that allocated enough space
 * for nmeb objects that are size bytes of memory each
 *
 * Parameters
 * - nmeb: object to allocate memory
 * - size: bytes to allocate
 *
 * Returns memory block pointer with allocated memory.
 */
void *calloc(size_t nmemb, size_t size) {
    //5 times 0 is still 0
    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    //malloc that
    void * ptr = malloc(nmemb * size);
    //init that to 0
    memset(ptr, 0, nmemb * size);

    //ret
    return ptr;
}

/** 
 * realloc
 *
 * Our own implementation of realloc that tries to change the size of the 
 * allocation point to by ptr to size
 *
 * Parameters:
 * - ptr: allocation to change 
 * - size: bytes to allocate
 * 
 * Returns memory block pointer with allocated memory
 */
void *realloc(void *ptr, size_t size) {
    /* Re-align the size */
    if (size % 8 != 0) {
        size = (((int) size / 8) * 8) + 8;
    }

    if (ptr == NULL) {
        /* If the pointer is NULL, then we simply malloc a new block */
        return malloc(size);
    }

    if (size == 0) {
        /* Realloc to 0 is often the same as freeing the memory block... But the
         * C standard doesn't require this. We will free the block and return
         * NULL here. */
        free(ptr);
        return NULL;
    }

    //get mem_block struct
    struct mem_block * blk = (struct mem_block *) ptr - 1;

    //check if there's space to just use
    if (blk->size >= size) {
        //change usage to new size
        blk->usage = size;
        return ptr;
    } else {
        //malloc a new pointer
        void * new = malloc(size);
        //copy those contents
        memcpy(new, ptr, blk->usage);
        //free old pointer
        free(ptr);
        //return new pointer
        return new;
    }

    //return NULL if we end up dying
    return NULL;
}