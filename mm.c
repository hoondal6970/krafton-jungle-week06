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
    "",
    /* Second member's email address (leave blank if none) */
    ""
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
    // 초기 세팅 4word size heap | 패딩블럭 | 프-헤더 | 프-푸터 | 에필로그 |
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1){
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));

    // 초기 bp 세팅
    heap_listp += (2*WSIZE);
    // next fit 용 최근 bp 초기화
    last_bp = (char *)heap_listp;

    // 초기 세팅 후 사용자 힙 영역 잡아주기 chunksize = 4kb
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL){
        return -1;
    }
    return 0;
}

// 호출 case 1. 힙이 초기화될 때
// 호출 case 2. mm_malloc이 적당한 fit을 찾지 못했을 때
static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    // 늘려줄 힙이 홀수면 -> 한 칸 추가해서 짝수 맞춰주기 = DW 사이즈 정렬
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

    // 앞 블럭, 뒤 블럭 모두 할당 된 상태면 -> 그대로
    if (prev_alloc && next_alloc){

        last_bp = (char *)bp;
        return bp;

    // 뒤 블럭이 free 상태면 -> 뒤 블럭 합병 
    }else if (prev_alloc && !next_alloc){

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

    // 앞 블럭이 free 상태면 -> 앞 블럭 합병
    }else if (!prev_alloc && next_alloc){

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);

    // 둘 다 free 상태면 -> 앞, 현재, 뒤 3블럭 합병
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

    if (size == 0){
        return NULL;
    }

    // DW 정렬에 맞추기 위해 사이즈 재 정의
    if (size <= DSIZE){
        asize = 2*DSIZE;
    }else{
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }

    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        last_bp = bp;
        return bp;
    }

    // 할당할 블럭이 들어갈 자리를 찾지 못하면 힙 영역 늘려줌
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    last_bp = bp;
    return bp;

}

static void *find_fit(size_t asize){
    /*
    //first fit
    void *bp;

    for (bp = (char *)heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL;
    */

    
    // next fit
    char *bp = last_bp;
    // 최근 bp에서 부터 에필로그 블럭까지
    for (bp = NEXT_BLKP(bp); GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            last_bp = bp;
            return bp;
        }
    }

    // 에필로그 블럭까지 탐색해서 없으면 처음 블럭 부터 다시 탐색
    bp = heap_listp;
    while (bp < last_bp) {
        bp = NEXT_BLKP(bp);
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            last_bp = bp;
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize){
    size_t bsize = GET_SIZE(HDRP(bp));
    size_t resize = bsize - asize;

    // (현재 free 블럭 - 삽입할 블럭)이 2*DSIZE 보다 크면 나눠줌
    if (resize >= (2*(DSIZE))){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        PUT(HDRP(NEXT_BLKP(bp)), PACK(resize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(resize, 0));
        bp = NEXT_BLKP(bp);
        
    // 아니면 다 씀
    }else{
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
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
    // 2*WISE는 헤더와 풋터
    size_t new_size = size + (2 * WSIZE);   

    // new_size가 old_size보다 작거나 같으면 기존 bp 그대로 사용
    if (new_size <= old_size) {
        return ptr;
    }
    // new_size가 old_size보다 크면 사이즈 변경
    else {
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        size_t current_size = old_size + GET_SIZE(HDRP(NEXT_BLKP(ptr)));

        // next block이 가용상태이고 old, next block의 사이즈 합이 new_size보다 크면 그냥 그거 바로 합쳐서 쓰기
        if (!next_alloc && current_size >= new_size) {
            PUT(HDRP(ptr), PACK(current_size, 1));
            PUT(FTRP(ptr), PACK(current_size, 1));
            return ptr;
        }
        // 아니면 새로 block 만들어서 거기로 옮기기
        else {
            void *new_bp = mm_malloc(new_size);
            place(new_bp, new_size);
            // ptr로부터 new_size만큼의 문자를 new_bp로 복사해라
            memcpy(new_bp, ptr, new_size);  
            mm_free(ptr);
            return new_bp;
        }
    }
}
