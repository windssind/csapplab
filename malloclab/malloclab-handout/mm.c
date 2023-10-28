/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define WSIZE 4
#define DSIZE 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define PACK(size, alloc) ((size) | (alloc))

#define GETSIZE(p) (*(unsigned int *)(p) & ~0x7)
#define GETALLO(p) (*(unsigned int *)(p)&0x1)

#define HEAD(bp) ((char *)(bp)-WSIZE)
#define FOOT(bp) ((char *)(bp) + GETSIZE(HEAD(bp)) - DSIZE)

#define NEXTBLOCK(bp) ((char *)(bp) + GETSIZE(HEAD(bp)))
#define PREVBLOCK(bp) ((char *)(bp)-GETSIZE((char *)(bp)-DSIZE))

#define BEGIN() (char *)mem_heap_lo() + DSIZE

#define ISALLOCATED(p) (*(size_t *)HEAD(p) & 0x1)
#define ISEND(p) (GETSIZE(HEAD(p)) == 0)

// #define _DEBUG

#ifdef _DEBUG
#define debugf(format, ...) printf(format, ##__VA_ARGS__)
#define printAll() printAllBlocks()
#else
#define debugf(format, ...)
#define printAll()
#endif

#define MAXFREESIZE 1 <<
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    debugf("init\n");
    char *start;
    if ((start = mem_sbrk(4 * WSIZE)) == (void *)-1) // Allocate a wordsize space
        return -1;
    PUT(start, 0);                          // padding
    PUT(start + 1 * WSIZE, PACK(DSIZE, 1)); // Head
    PUT(start + 2 * WSIZE, PACK(DSIZE, 1)); // Foot
    PUT(start + 3 * WSIZE, PACK(0, 1));     // End flag

    return 0;
}

void merge(void *ptr)
{
    debugf("merge\n");
    void *prev = PREVBLOCK(ptr);
    void *next = NEXTBLOCK(ptr);
    if (!ISALLOCATED(prev) && !ISALLOCATED(next))
    {
        int totalsize = GETSIZE(HEAD(prev)) + GETSIZE(HEAD(next)) + GETSIZE(HEAD(ptr));
        PUT(HEAD(prev), PACK(totalsize, 0));
        PUT(FOOT(next), PACK(totalsize, 0));
        printAll();
    }
    else if (!ISALLOCATED(prev) && ISALLOCATED(next))
    {
        int totalsize = GETSIZE(HEAD(prev)) + GETSIZE(HEAD(ptr));
        PUT(HEAD(prev), PACK(totalsize, 0));
        PUT(FOOT(ptr), PACK(totalsize, 0));
        printAll();
    }
    else if (ISALLOCATED(prev) && !ISALLOCATED(next))
    {
        int totalsize = GETSIZE(HEAD(next)) + GETSIZE(HEAD(ptr));
        PUT(HEAD(ptr), PACK(totalsize, 0));
        PUT(FOOT(next), PACK(totalsize, 0));
        printAll();
    }
    debugf("No merge\n");
}

void printAllBlocks()
{
    void *current = BEGIN();
    while (!ISEND(current))
    {
        size_t freeSize = GETSIZE(HEAD(current));
        debugf("|%c %d|", (ISALLOCATED(current) ? 'A' : 'F'), freeSize);
        current = NEXTBLOCK(current);
    }
    debugf("\n");
}

void *firstFit(size_t realsize)
{
    debugf("FirstFit:");
    void *current = BEGIN();
    while (!ISEND(current))
    {
        size_t freeSize = GETSIZE(HEAD(current));
        debugf("size:%d,", freeSize);
        // The block is free and has the size equal to realsize
        if (!ISALLOCATED(current) && freeSize == realsize)
        {
            PUT(HEAD(current), PACK(realsize, 1));
            PUT(FOOT(current), PACK(realsize, 1));
            debugf("\n");
            return current;
        }
        // the bigger size -> separate the free block into one allocated block and one free block
        else if (!ISALLOCATED(current) && freeSize > realsize)
        {
            PUT(HEAD(current), PACK(realsize, 1));
            PUT(FOOT(current), PACK(realsize, 1));

            PUT(HEAD(NEXTBLOCK(current)), PACK(freeSize - realsize, 0));
            PUT(FOOT(NEXTBLOCK(current)), PACK(freeSize - realsize, 0));
            debugf("\n");
            return current;
        }
        current = NEXTBLOCK(current);
    }
    debugf("\n");
    return NULL;
}

void *best_fit(size_t asize)
{
    void *bp;
    void *best_bp = NULL;
    size_t min_size = 0;
    for (bp = BEGIN(); GETSIZE(HEAD(bp)) > 0; bp = NEXTBLOCK(bp))
    {
        if ((GETSIZE(HEAD(bp)) >= asize) && (!ISALLOCATED(bp)))
        {
            if (min_size == 0 || min_size > GETSIZE(HEAD(bp)))
            {
                min_size = GETSIZE(HEAD(bp));
                best_bp = bp;
            }
        }
    }

    if (best_bp)
    {
        if (!ISALLOCATED(best_bp) && min_size == asize)
        {
            PUT(HEAD(best_bp), PACK(asize, 1));
            PUT(FOOT(best_bp), PACK(asize, 1));
            debugf("\n");
        }
        // the bigger size -> separate the free block into one allocated block and one free block
        else if (!ISALLOCATED(best_bp) && min_size > asize)
        {
            PUT(HEAD(best_bp), PACK(asize, 1));
            PUT(FOOT(best_bp), PACK(asize, 1));

            PUT(HEAD(NEXTBLOCK(best_bp)), PACK(min_size - asize, 0));
            PUT(FOOT(NEXTBLOCK(best_bp)), PACK(min_size - asize, 0));
            debugf("\n");
        }
    }

    return best_bp;
}

// return a pointer to a free block with enough size
void *extendHeap(size_t neededSize)
{
    void *p = mem_sbrk(0); // end
    void *lastValid = PREVBLOCK(p);

    if (!ISALLOCATED(lastValid))
    {
        size_t lastFreeSize = GETSIZE(HEAD(lastValid));
        mem_sbrk(neededSize - lastFreeSize);
        PUT(HEAD(lastValid), PACK(neededSize, 0));
        PUT(FOOT(lastValid), PACK(neededSize, 0));
        return lastValid;
    }
    else
    {
        return mem_sbrk(neededSize);
    }
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    debugf("Malloc size:%d\n", size);
    if (size == 0)
    {
        return NULL;
    }

    int newsize = ALIGN(size + SIZE_T_SIZE);

    void *p;
    if ((p = best_fit(newsize)))
    {
        debugf("Malloced pointer:%p\n", p);
        printAll();
        return p;
    }

    p = extendHeap(newsize); // extend heap,p is the end

    if (p == (void *)-1)
    {
        return NULL;
    }

    PUT(HEAD(p), PACK(newsize, 1));
    PUT(FOOT(p), PACK(newsize, 1));
    PUT(HEAD(NEXTBLOCK(p)), PACK(0, 1)); // End flag

    // TODO: merge the previous free block
    debugf("Malloced pointer:%p\n", p);
    printAllBlocks();
    return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    debugf("Free %p\n", ptr);
    size_t size = GETSIZE(HEAD(ptr));
    PUT(HEAD(ptr), PACK(size, 0));
    PUT(FOOT(ptr), PACK(size, 0));
    printAllBlocks();
    merge(ptr);
    // TODO: merge the free block
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    debugf("Realloc start\n");
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = GETSIZE(HEAD(oldptr)) - DSIZE;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    debugf("Realloc end\n");
    return newptr;
}