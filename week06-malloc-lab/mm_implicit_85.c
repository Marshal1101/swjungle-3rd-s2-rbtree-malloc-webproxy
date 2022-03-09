/*
 * mm-implicit policy.c - first fit or next fit
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
    "PARK Hongjung - implicit - next fit",
    /* First member's email address */
    "adequateplayer9@gmail.com",
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


/* Basic constants and macros */
#define WSIZE       4       
#define DSIZE       8
#define CHUNKSIZE   (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)   ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(unsigned int *)(p))
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp,  compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


static char *heap_listp;
static char *last_bp;
static void *extend_heap(size_t words);
static void *coalesce(void* bp);
static void *find_fit(size_t asize);
static void *next_fit(size_t asize);
static void place(void *bp, size_t asize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) - 1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    last_bp = heap_listp;
    return 0;
}

static void* extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    // 가용블록 헤더와 풋터, 새로운 에필로그 헤더
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // 이전 블록이 가용이면 COALESCE
    return coalesce(bp);
}

static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // case 1: 이전과 다음 블록이 모두 할당(둘 다 1)
    // 블럭 병합 없이 bp return
    if (prev_alloc && next_alloc) {
        last_bp = bp;
        return bp;
    }

    // case 2: 이전 블록 할당(1), 다음 블록은 가용(0)
    // 다음 블록 크기를 현재 블록 size에 더한다.
    // 현재 블록 헤더와 푸터에 가용으로 갱신
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // case 3: 이전 블록이 가용, 다음 블록은 할당
    // 이전 블록 크키를 현재 블록 size에 더한다.
    // 현재 블록 푸터와 이전 블록 헤더를 갱신
    // bp를 이전 블록의 주소로 갱신
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    // case 4 : 이전 블록과 다음 블록 모두 가용
    // 이전 블록 크기, 다음 블록 크기를 현재 블록에 더한다.
    // 이전 블록 헤더, 다음 블록 푸터를 갱신
    // bp를 이전 블록의 주소로 갱신
    else {
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    last_bp = bp;
    return bp;
}

static void *find_fit(size_t asize)
{
    /* First-fit search */
    void *bp;
    // 힙의 마지막인 에필로그 블록의 크기 0값을 만날 때까지
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL;    /* No fit */
// #endif
}

static void *next_fit(size_t asize)
{
    /* Next-fit search */
    void *bp;

    // last_bp ~ mem_max_addr
    for (bp = last_bp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            last_bp = bp;
            return bp;
        }
    }

    // mem_heap(heap_listp) ~ last_bp
    for (bp = heap_listp; bp < last_bp; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            last_bp = bp;
            return bp;
        }
    }
    return NULL;    /* No fit */
}


static void place(void *bp, size_t asize)
// 이 할당기는 최소 블록 크기가 16바이트(HRD + Payload + FTR)이다.
// bp는 find_fit으로 찾은 가용 블록의 주소
// asize는 할당하고자 하는 크기
{
    size_t csize = GET_SIZE(HDRP(bp));
    // 만약 분할 후에 이 블록의 나머지가 최소 블록 크기와 같거나 크다면,
    // 블록을 분할해야 한다.    
    if ((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
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

    // 할당 사이즈가 0이면 할당하지 않는다.
    if (size == 0)
        return NULL;

    // 할당 사이즈가 8보다 작은면 asize = 16 bytes로 한다.
    // 정렬요건8 + 헤더4 풋터4 오버헤드
    if (size <= DSIZE)
        asize = 2*DSIZE;

    // 8보다 크면 오버헤드 바이트를 추가하고, 인접 8의 배수로 반올림
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
        // 9~16 -> 24, 17~24 -> 32, 25~32 -> 40 etc
    
    // 힙에서 할당할만한 블록을 찾은 다음 찾았다면 place 함수로 할당
    if ((bp = next_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    // 할당할만한 블록을 찾지 못했으면 asize 와 CHUNKSIZE 중 큰 값을 찾는다.
    // extend_heap함수를 사용해 공간을 확장한다. 입력은 words의 개수
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    // 확장하여 커진 힙 위치에 할당한다.
    place(bp, asize);
    last_bp = bp;
    return bp;
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free (참조:ksh19933)
 */
void *mm_realloc(void *bp, size_t size) {
    size_t old_size = GET_SIZE(HDRP(bp));
    size_t new_size = size + (2 * WSIZE);   // 2*WISE는 헤더와 풋터

    // new_size가 old_size보다 작거나 같으면 기존 bp 그대로 사용
    if (new_size <= old_size) {
        return bp;
    }
    // new_size가 old_size보다 크면 사이즈 변경
    else {
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t current_size = old_size + GET_SIZE(HDRP(NEXT_BLKP(bp)));

        // next block이 가용상태이고 old, next block의 사이즈 합이 new_size보다 크면 그냥 그거 바로 합쳐서 쓰기
        if (!next_alloc && current_size >= new_size) {
            PUT(HDRP(bp), PACK(current_size, 1));
            PUT(FTRP(bp), PACK(current_size, 1));
            return bp;
        }
        // 아니면 새로 block 만들어서 거기로 옮기기
        else {
            void *new_bp = mm_malloc(new_size);
            place(new_bp, new_size);
            memcpy(new_bp, bp, new_size);  
            // 메모리의 특정한 부분으로부터 얼마까지의 부분을 다른 메모리 영역으로 복사해주는 함수
            // (old_bp로부터 new_size만큼의 문자를 new_bp로 복사해라!)
            mm_free(bp);
            return new_bp;
        }
    }
}

void *mm_calloc (size_t nmemb, size_t size) {
    size_t bytes = nmemb * size;
    void *newptr;

    newptr = mm_malloc(bytes);
    memset(newptr, 0, bytes);

    return newptr;
}



// /*
//  * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
//  */
// void *mm_realloc(void *ptr, size_t size)
// {
//     void *oldptr = ptr;
//     void *newptr;
//     size_t copySize;
    
//     newptr = mm_malloc(size);
//     if (newptr == NULL)
//       return NULL;
//     // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
//     copySize = GET_SIZE(HDRP(oldptr));
//     if (size < copySize)
//       copySize = size;
//     memcpy(newptr, oldptr, copySize);
//     mm_free(oldptr);
//     return newptr;
// }

// void *mm_calloc (size_t nmemb, size_t size) {
//     size_t bytes = nmemb * size;
//     void *newptr;

//     newptr = mm_malloc(bytes);
//     memset(newptr, 0, bytes);

//     return newptr;
// }