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
    "KJ-R-8team",
    /* First member's full name */
    "gitae Kim",
    /* First member's email address */
    "rlarlxo2628@gmail.com",
    /* Second member's full name (leave blank if none) */
    "dasol Park",
    /* Second member's email address (leave blank if none) */
    "davidtwins6970@gmail.com"
    /* Third member's full name (leave blank if none) */
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* define add */
#define WSIZE   4   // 32bit -> word
#define DSIZE   8   // 32bit -> double word
#define CHUNKSIZE   (1<<12) // 32bit -> basic heap size

#define MAX(x,y)    ((x) > (y)? (x) : (y))

#define PACK(size, alloc)   ((size) | (alloc))  // header & footer data

#define GET(p)  (*(unsigned int *)(p))  // read word at address p
#define PUT(p, val)   (*(unsigned int *)(p) = (val))    // write word at address p

#define GET_SIZE(p) (GET(p) & ~0x7) // read block size
#define GET_ALLOC(p) (GET(p) & 0x1) // read block allocated

#define HDRP(bp)    ((char *)(bp) - WSIZE)  // header address
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // footer address

#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))   // next bp address
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))   // pre bp address

/* forward declaration function */
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static char *heap_listp;
static char *last_bp;

/* 
 * mm_init - initialize the malloc package.
 */

int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1){
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE);
    // last_bp = (char *)heap_listp;

    if ((last_bp = extend_heap(CHUNKSIZE/WSIZE)) == NULL){
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc){

        last_bp = (char *)bp;
        return bp;

    }else if (prev_alloc && !next_alloc){

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

    }else if (!prev_alloc && next_alloc){

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }else{

        size += GET_SIZE(HDRP(PREV_BLKP(bp)))+ GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }
    last_bp = (char *)bp;
    return bp;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0)
        return NULL;
    
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        last_bp = NEXT_BLKP(bp);  /*next fit을 위한 bp 저장*/
        return bp;
    }
    
    /*No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    last_bp = NEXT_BLKP(bp);  /*next fit을 위한 bp 저장*/
    return bp;

}

static void *find_fit(size_t asize){
    /* first fit of 기태*/
    // void *bp;

    // for (bp = (char *)heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    //     if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
    //         return bp;
    //     }
    // }
    // return NULL;

    // /* First fit */
    // void *bp;

    // for(bp = (char*)heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
    //     if (!GET_ALLOC(HDRP(bp))  && GET_SIZE(HDRP(bp)) >= asize){
    //         return bp;
    //     }
    // }

    // return NULL;

    /* Next fit */
    void *bp;

    for(bp = (char*)last_bp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp))  && GET_SIZE(HDRP(bp)) >= asize){
            return bp;
        }
    }
    
    for(bp = (char *)heap_listp ; bp != last_bp ; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
        {
            return bp;
        }
    }

    return NULL;

}

static void place(void *bp, size_t asize)
{   
    size_t csize = GET_SIZE(HDRP(bp));
    size_t resize = csize - asize;

    if ((resize) >= 2*DSIZE){
        PUT(FTRP(bp), PACK((resize), 0));
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK((resize), 0));
    }
    else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t new_size = size + (2 * WSIZE);

    if (new_size <= old_size)
        return ptr;
    else
    {
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        size_t current_size = old_size + GET_SIZE(HDRP(NEXT_BLKP(ptr)));

        if (!next_alloc && size <= current_size)
        {
            PUT(HDRP(ptr), PACK(current_size, 1));
            PUT(FTRP(ptr), PACK(current_size, 1));
            return ptr;
        }
        void *new_bp = mm_malloc(new_size);
        place(new_bp, new_size);
        memcpy(new_bp, ptr, new_size);
        mm_free(ptr);
        return new_bp;
    }
    
    // newptr = mm_malloc(size);
    // if (newptr == NULL)
    //   return NULL;
    // copySize = GET_SIZE(HDRP(oldptr));
    // if (size < copySize)
    //   copySize = size;
    // memcpy(newptr, oldptr, copySize);
    // mm_free(oldptr);
    // return newptr;

}