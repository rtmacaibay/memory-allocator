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
#define DEBUG 1
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
    if (ptr->usage == 0) {
        ptr->usage = real_sz;
        unsigned long old_id = ptr->alloc_id;
        ptr->alloc_id = g_allocations++;
        LOG("Allocation request USING reuse; reusing alloc %ld\n", old_id);
        return ptr + 1;
    }
    struct mem_block * freeloader = (void*)ptr + ptr->usage;
    freeloader->alloc_id = g_allocations++;
    freeloader->size = ptr->size - ptr->usage;
    freeloader->usage = real_sz;
    freeloader->region_start = ptr->region_start;
    freeloader->region_size = ptr->region_size;
    if (g_tail == ptr) {
        freeloader->next = NULL;
        g_tail->next = freeloader;
        g_tail = freeloader;
    } else {
        freeloader->next = ptr->next;
        ptr->next = freeloader;
    }

    ptr->size = ptr->usage;

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
    if (g_head == NULL) {
        return NULL;
    }

    char * algo = getenv("ALLOCATOR_ALGORITHM");
    if (algo == NULL) {
        algo = "first_fit";
    }

    //LOG("Algo: %s\n", algo);

    size_t fits_best = SIZE_MAX;
    size_t fits_worst = 1;

    struct mem_block * the_spot = NULL;

    struct mem_block * curr = g_head;
    size_t real_sz = size + sizeof(struct mem_block);

    LOG("Allocation request USING reuse; size = %zu, alloc = %ld\n", real_sz, g_allocations);

    while (curr != NULL) {
        size_t free_space = curr->size - curr->usage;

        if (strcmp(algo, "first_fit") == 0) {
            if (curr->usage < curr->size && real_sz <= free_space) {
                return reuse_space(curr, real_sz);
            }
        } else if (strcmp(algo, "best_fit") == 0) {
            if (curr->usage < curr->size && real_sz <= free_space) {
                LOG("Possible space: %zu\n", free_space);
                if (free_space < fits_best) {
                    //LOG("New space: %zu\n", free_space);
                    fits_best = free_space;
                    the_spot = curr;
                }
            }
        } else if (strcmp(algo, "worst_fit") == 0) {
            if (curr->usage < curr->size && real_sz <= free_space) {
                if (free_space > fits_worst) {
                    fits_worst = free_space;
                    the_spot = curr;
                }
            }
        }
        curr = curr->next;
    }

    if (the_spot != NULL) {
        size_t free_space = the_spot->size - the_spot->usage;
        LOG("Using this spot: %zu\n", free_space);
        return reuse_space(the_spot, real_sz);
    }

    return NULL;
}

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
    pthread_mutex_lock(&g_alloc_mutex);
    //LOG("Allocation request; size = %zu\n", size);
    char * s = getenv("ALLOCATOR_SCRIBBLE");
    int scribble = 0;

    if (s != NULL) {
        scribble = atoi(s);
    }

    /* Re-align the size */
    if (size % 8 != 0) {
        size = (((int) size / 8) * 8) + 8;
    }

    void * ptr = reuse(size);
    if (ptr == NULL) {
        //LOGP("Reuse returned NULL\n");
    }
    if (ptr != NULL) {
        // noice we could reuse space 
        if (scribble) {
            scribble_this(ptr, size);
        }
        pthread_mutex_unlock(&g_alloc_mutex);
        return ptr;
    }

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
    //LOGP("Mapping new region\n");
    /* Set up memory block */
    struct mem_block * block = mmap(
            NULL, /* Address (we use NULL to let the kernel decide) */
            region_sz, /* Size of memory block to allocate */
            PROT_READ | PROT_WRITE, /* Memory protection flags */
            MAP_PRIVATE | MAP_ANONYMOUS, /* Type of mapping */
            -1, /* file descriptor */
            0 /* offset to start at within the file */);

    if (block == MAP_FAILED) {
        pthread_mutex_unlock(&g_alloc_mutex);
        perror("mmap");
        return NULL;
    }

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

    if (scribble) {
        scribble_this(block + 1, size);
    }

    pthread_mutex_unlock(&g_alloc_mutex);
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
    int ret = munmap(ptr->region_start, ptr->region_size);
    //LOGP("Free request; Destroyed a pointer\n");
    if (ret == -1) {
        perror("munmap");
    }
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
    pthread_mutex_lock(&g_alloc_mutex);

    if (ptr == NULL) {
        /* Freeing a NULL pointer does nothing */
        pthread_mutex_unlock(&g_alloc_mutex);
        return;
    }

    struct mem_block * blk = (struct mem_block *) ptr - 1;
    LOG("Free request; size = %zu, alloc = %lu\n", blk->usage, blk->alloc_id);
    
    blk->usage = 0;

    struct mem_block * orig_start = blk->region_start;
    struct mem_block * prev = g_head;
    struct mem_block * first_half = NULL;
    struct mem_block * sec_half = NULL;
    int first_node = 0;

    //if first node is our block
    if (prev->region_start == orig_start) {
        first_node = 1;
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
                //LOGP("Free request; Completed - block still in use\n");
                pthread_mutex_unlock(&g_alloc_mutex);
                return;
            }
        } else if (first_half != NULL) {
            sec_half = curr;
            break;
        }
        prev = prev->next;
    }

    /* I WILL FINSIH THIS ELIJAH */
    if (first_node) {
        if (sec_half == NULL) {
            destroy_this(blk);
            g_head = NULL;
            g_tail = NULL;
        } else {
            destroy_this(blk);
            g_head = sec_half;
        }
    } else if (first_half != NULL) {
        if (sec_half == NULL) {
            destroy_this(blk);
            g_tail = first_half;
            g_tail->next = NULL;
        } else {
            destroy_this(blk);
            first_half->next = sec_half;
        }
    } else {
        perror("free");
    }

    pthread_mutex_unlock(&g_alloc_mutex);

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
void *calloc(size_t nmemb, size_t size)
{
    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    void * ptr = malloc(nmemb * size);
    memset(ptr, 0, nmemb * size);

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

    // TODO: reallocation logic
    struct mem_block * blk = (struct mem_block *) ptr - 1;

    if (blk->size >= size) {
        blk->usage = size;
        return ptr;
    } else {
        void * new = malloc(size);
        memcpy(new, ptr, blk->usage);
        free(ptr);
        return new;
    }

    return NULL;
}