/**
 * allocator.c
 *
 * Explores memory management at the C runtime level.
 *
 * Author: <your team members go here>
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
 * TODO: make this not allocate memory //fprintf ??
 *
 * Prints out the current memory state, including both the regions and blocks.
 * Entries are printed in order, so there is an implied link from the topmost
 * entry to the next, and so on.
 */
void print_memory(void)
{
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

void *reuse(size_t size) {
    assert(g_head != NULL);

    // TODO: using free space management algorithms, find a block of memory that
    // we can reuse. Return NULL if no suitable block is found.

    struct mem_block * curr = g_head;

    while (curr != NULL) {
        // i will finish this later you fool
        if (curr->size == curr->region_size && curr->usage < curr->size) {

        }
    }

    return NULL;
}

void *malloc(size_t size)
{
    // TODO: allocate memory. You'll first check if you can reuse an existing
    // block. If not, map a new memory region.

    // void * ptr = reuse(size);
    // if (ptr != NULL) {
    //     // noice we could reuse space
    //     return ptr;
    // }

    // re-align the size
    if (size % 8 != 0) {
        size = (((int) size / 8) * 8) + 8;
    }

    LOG("Allocation request; size = %zu\n", size);

    // go through list and see if there's some free blocks that fits
    // we're trying to allocate

    /* How much space we are using in the region */
    size_t real_sz = size + sizeof(struct mem_block);

    LOG("Allocation request; real size = %zu\n", real_sz);

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

    if (block == MAP_FAILED) {
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

    return block + 1;
}

void free(void *ptr)
{
    if (ptr == NULL) {
        /* Freeing a NULL pointer does nothing */
        return;
    }

    struct mem_block * blk = (struct mem_block *) ptr - 1;
    LOG("Free request; allocation = %lu\n", blk->alloc_id);

    blk->usage = 0;

    // TODO: free memory. If the containing region is empty (i.e., there are no
    // more blocks in use), then it should be unmapped.

    // 1. go to region start
    // 2. traverse through LL
    // 3. stop when you:
    //  a. find something that is not free
    //  b. when you find the start of a different region
    // 4. if you (a) move on; if (b) then munmap

    int ret = munmap(blk, blk->region_size);
    if (ret == -1) {
        perror("munmap");
    }
}

void *calloc(size_t nmemb, size_t size)
{
    // TODO: hmm, what does calloc do?
    // malloc
    // memset(ptr, 0, nmemb * size);
    return NULL;
}

void *realloc(void *ptr, size_t size)
{
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

    return NULL;
}

