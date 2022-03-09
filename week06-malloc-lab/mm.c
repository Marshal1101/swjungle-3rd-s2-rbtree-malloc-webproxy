/*
 * mm-explicit policy.c - first fit with FILO
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
    "3",
    /* First member's full name */
    "PARK Hongjung - simple_segregated",
    /* First member's email address */
    "adequateplayer9@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};


// CONSTANTS
#define ALIGNMENT         8         // payload 할당 최소 크기
#define WSIZE             4         // 워드
#define DSIZE             8         // 더블워드
#define INITSIZE          16        // 초기 블록 크기, 헤더, 풋터, payload
#define MINBLOCKSIZE      16        // 최소 블록 크기, 헤더, 풋터, payload

// segregated list 최대 개수
#define LISTLIMIT 20

// MACROS
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

//sizeof(size_t) = 8, ALIGN(sizeof(size_t)) = 8
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4             /* word and header, footer size */
#define DSIZE 8             /* double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extended heap by this amount */
#define LISTLIMIT 20        /* free list max num */             

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at adress p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
//p가 할당 되있는지(1) 아닌지 (0)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp)-DSIZE)))

/* get free list's SUCC and PREC pointer */
#define PRED_FREE(bp) (*(char **)(bp))
#define SUCC_FREE(bp) (*(char **)(bp + WSIZE))

// 전역 함수
static void *extend_heap(size_t words);
static void *find_fit(size_t size);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void remove_freeblock(void *bp);
static void insert_block(void *bp, size_t size);
// static int mm_check();

// 전역 변수
static void *heap_listp;  /* Points to the start of the heap */

// segregated list 크기별 정적 배열
static void *segregated_list;


int mm_init(void)
{
    int list;
    
    for (list = 0; list < LISTLIMIT; list++) {
        segregated_list[list] = NULL;
    }

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    {
        return -1;
    }
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    segregated_list = heap_listp+2*WSIZE;
    *segregated_list = NULL;
    
    /* Extended the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }
    return 0;
}


/*
 * mm_malloc - Allocates a block of memory of memory of the given size aligned to 8-byte
 * boundaries.
 *
 * A block is allocated according to this strategy:
 * (1) 요청 크기의 가용한 블록을 찾으면 할당하고 블록 주소를 반환한다.
 * (2) 찾을 수 없으면 필요한 만큼 힙을 늘리고 새로운 가용 블록을 할당한다.
 */
void *mm_malloc(size_t size)
{
    int asize = ALIGN(size + SIZE_T_SIZE);
    
    // size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Frees the block being pointed to by bp.
 *
 * 할당 비트를 단순히 0으로 변환한다.
 * 가용리스트 인접 블록과 병합한다.
 * 
 */
void mm_free(void *bp)
{ 
    if (!bp)
        return;

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * extend_heap - 정수 짝수 배로 할당한다.
 * 요구 크기를 최소 블록 사이즈 이상으로 할당한다.
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t asize;

    /* Adjust the size so the alignment and minimum block size requirements
    * are met. */ 
    asize = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
        if (asize < MINBLOCKSIZE)
            asize = MINBLOCKSIZE;

    // Attempt to grow the heap by the adjusted size 
    if ((long)(bp = mem_sbrk(asize)) == -1)
        return NULL;

    /* 새로 추가한 가용 블록 크기, 헤더, 풋터, 에필로그 갱신 */
    PUT(HDRP(bp), PACK(asize, 0));
    PUT(FTRP(bp), PACK(asize, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* Move the epilogue to the end */

    // bp 앞 블록과 병합
    return coalesce(bp); 
}

/*
 * find_fit
 */
static void *find_fit(size_t asize)
{
    /* First-fit search */
    void *bp;

    int list = 0;
    size_t searchsize = asize;

    while (list < LISTLIMIT){
        if ((list == LISTLIMIT-1) || ((searchsize <= 1)&&(segregated_list[list] != NULL))){
            bp = segregated_list[list];

            while ((bp != NULL) && (asize > GET_SIZE(HDRP(bp)))){
                bp = SUCC_FREE(bp);
            }
            if (bp != NULL){
                return bp;
            }
        }
        searchsize >>= 1;
        list++;
    }

    return NULL; /* no fit */

// #endif
}

/*
 * remove_freeblock - 주어진 bp와 segregated list 어느 배열인지 이용한다.
 */
static void remove_freeblock(void *bp){
    int list = 0;
    size_t size = GET_SIZE(HDRP(bp));

    while ((list < LISTLIMIT - 1) && (size > 1)) {
        size >>= 1;
        list++;
    }

    if (SUCC_FREE(bp) != NULL){
        if (PRED_FREE(bp) != NULL){
            PRED_FREE(SUCC_FREE(bp)) = PRED_FREE(bp);
            SUCC_FREE(PRED_FREE(bp)) = SUCC_FREE(bp);
        }else{
            PRED_FREE(SUCC_FREE(bp)) = NULL;
            segregated_list[list] = SUCC_FREE(bp);
        }
    }else{
        if (PRED_FREE(bp) != NULL){
            SUCC_FREE(PRED_FREE(bp)) = NULL;
        }else{
            segregated_list[list] = NULL;
        }
    }

    return;
}

/*
 * coalesce - 병합
 */
static void *coalesce(void *bp)
{
  // Determine the current allocation state of the previous and next blocks 
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

  // Get the size of the current free block
  size_t size = GET_SIZE(HDRP(bp));

  /* 경우1 : 앞뒤 블록이 전부 할당 -> 그냥 삽입 */
    if (prev_alloc && next_alloc){
        insert_block(bp,size);
        return bp;
    }

  /* 경우2 : 다음 블록이 가용
   * 다음 블록과 크기 합치고 현재 블록 주소로 리스트에서 꺼낸다.
   */
    else if (prev_alloc && !next_alloc) {      
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));  
        remove_freeblock(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

  /* 경우3 : 이전 블록이 가용
   * size에 이전 블록 크기를 더하고, 이전 블록 주소로 리스트에서 꺼낸다.
   */
    else if (!prev_alloc && next_alloc) {    
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp); 
        remove_freeblock(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } 

  /* 경우4 : 이전과 다음 블록이 모두 가용
   * size에 이전과 다음 블록 크기를 더하고, 두 블록을 가용리스트에서 제거
   * 이전 블록으로 bp 이동
   * size만큼 헤더와 풋터 할당비트 0
   */
    else if (!prev_alloc && !next_alloc) {    
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
                GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_freeblock(PREV_BLKP(bp));
        remove_freeblock(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // 병합한 가용 블록을 가용리스트 다시 넣어 정렬 -> 특정 인덱스 배열 안에 오름차순
    insert_block(bp, size);

    // Return the coalesced block 
    return bp;
}

/*
 * place - Places a block of the given size in the free block pointed to by the given
 * pointer bp.
 *
 * 쪼개기전략. 가용 블록 크기 >= 요구 크기 + 최소 블록 사이즈이면,
 * 전자는 할당시키고(remove_freeblock),
 * 후자는 가용리스트에 추가(할당비트0)하며 병합(coalesce)시킨다.
 * 
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    remove_freeblock(bp);
    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        coalesce(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


static void insert_block(void *bp, size_t size)
{
    // list: segregated list 리스트 인덱스
    // search_ptr: 리스트 내 삽입 블록 크기보다 바로 다음으로 큰 블록의 주소
    // insert_ptr: 리스트 내 삽입 블록 크기보다 바로 직전에 작은 블록의 주소
    int list = 0;
    void *search_listp;
    void *search_ptr;
    void *insert_ptr = NULL;

    // 블록크기의 이진수 자릿수가 곧 segregated list에 배정될 인덱스
    while (size > 1){
        size >>= 1;
        list++;
    }

    search_listp = segregated_list + (list * WSIZE);
    while ((search_listp != NULL) && (size > GET_SIZE(HDRP(search_ptr)))) {
        insert_ptr = search_ptr;
        search_ptr = SUCC_FREE(search_ptr);   
    }
    // list에 담긴 인덱스를 통해 해당 인덱스 배열의 첫 주소를 search_ptr에 넣는다.
    search_ptr = 
    if GET_ALLOC(HDRP(search_ptr)) 

    // 만약 배열이 비어있지 않고, 삽입할 블록 크기가 배열에 있는 첫 블록보다 크다면,
    // 배열 끝에 도달하거나 삽입할 블록보다 더 큰 블록을 만날 때까지
    // NEXT 주소로 이동
    while ((search_ptr != NULL) && (size > GET_SIZE(HDRP(search_ptr)))) {
        insert_ptr = search_ptr;
        search_ptr = SUCC_FREE(search_ptr);   
    }

    // 만약 search_ptr이 배열 맨 마지막에 도달했다면,
    if (!search_ptr) {
        // 이전 블록이 존재
        if (insert_ptr) {
            SUCC_FREE(bp) = NULL;
            PRED_FREE(bp) = insert_ptr;
            SUCC_FREE(insert_ptr) = bp;
            // insert_ptr -> bp -> NULL
        }
        // 이전 블록이 없다면 첫 블록이다.
        else {
            SUCC_FREE(bp) = NULL;
            PRED_FREE(bp) = NULL;
            segregated_list[list] = bp;
        }
    }
    // search_ptr이 배열 어딘가에 도달했는데,
    else {
        // 이전 블록이 존재할 때,
        if (insert_ptr) {
            SUCC_FREE(bp) = search_ptr;
            PRED_FREE(search_ptr) = bp;
            PRED_FREE(bp) = insert_ptr;
            SUCC_FREE(insert_ptr) = bp;
        }
        // 이전 블록이 없다면 첫 블록이다.
        else {
            SUCC_FREE(bp) = search_ptr;
            PRED_FREE(search_ptr) = bp;
            PRED_FREE(bp) = NULL;
            segregated_list[list] = bp;
        }
    }
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
