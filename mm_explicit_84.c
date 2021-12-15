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
    "PARK Hongjung - explicit",
    /* First member's email address */
    "adequateplayer9@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};



// CONSTANTS
#define ALIGNMENT         8         // payload 할당 최소 크기
#define WSIZE             4         // Size in bytes of a single word 
#define DSIZE             8         // Size in bytes of a double word
#define INITSIZE          16        // Initial size of free list before first free block added
#define MINBLOCKSIZE      16        /* Minmum size for a free block, includes 4 bytes for header/footer
                                       and space within the payload for two pointers to the prev and next
                                       free blocks */

// MACROS
 /* NEXT_FREE and PREV_FREE macros to traverse the free list */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define MAX(x, y) ((x) > (y)? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p)        (*(size_t *)(p))
#define PUT(p, val)   (*(size_t *)(p) = (val))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp)     ((void *)(bp) - WSIZE)
#define FTRP(bp)     ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((void *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((void *)(bp) - GET_SIZE(HDRP(bp) - WSIZE))
#define NEXT_FREE(bp) (*(void **)(bp))
#define PREV_FREE(bp) (*(void **)(bp + WSIZE))


// PROTOTYPES
static void *extend_heap(size_t words);
static void *find_fit(size_t size);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void remove_freeblock(void *bp);
// static int mm_check();


// Private variables represeneting the heap and free list within the heap
static char *heap_listp = 0;  /* Points to the start of the heap */
static char *free_listp = 0;  /* Poitns to the frist free block */


/* 
 * mm_init - Initializes the heap like that shown below.
 *  ____________                                                    _____________
 * |  PROLOGUE  |                8+ bytes or 2 ptrs                |   EPILOGUE  |
 * |------------|------------|-----------|------------|------------|-------------|
 * |   HEADER   |   HEADER   |        PAYLOAD         |   FOOTER   |    HEADER   |
 * |------------|------------|-----------|------------|------------|-------------|
 * ^            ^            ^       
 * heap_listp   free_listp   bp 
 */
int mm_init(void)
{
  // Initialize the heap with freelist prologue/epilogoue and space for the
  // initial free block. (32 bytes total)
  if ((heap_listp = mem_sbrk(INITSIZE + MINBLOCKSIZE)) == (void *)-1)
      return -1; 
  PUT(heap_listp,             PACK(MINBLOCKSIZE, 1));           // Prologue header 
  PUT(heap_listp +    WSIZE,  PACK(MINBLOCKSIZE, 0));           // Free block header 

  PUT(heap_listp + (2*WSIZE), PACK(0,0));                       // Space for next pointer 
  PUT(heap_listp + (3*WSIZE), PACK(0,0));                       // Space for prev pointer 
  
  PUT(heap_listp + (4*WSIZE), PACK(MINBLOCKSIZE, 0));           // Free block footer 
  PUT(heap_listp + (5*WSIZE), PACK(0, 1));                      // Epilogue header 

  // Point free_list to the first header of the first free block
  free_listp = heap_listp + (WSIZE);

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
  
  if (size == 0)
      return NULL;

  size_t asize;       // 8배수 정렬로 조정된 블록 크기
  size_t extendsize;  // 맞는 블록이 없을 때 확장된 힙 크기 
  char *bp;

  /* 헤터, 풋터, 그리고 거기에 더해서
   * payload가 최소 블록 크기보다 작으면 최소 블록 크기로 정한다.
   */
  asize = MAX(ALIGN(size) + DSIZE, MINBLOCKSIZE);
  
  // 가용리스트에서 가용 블록 검색
  if ((bp = find_fit(asize))) {
    place(bp, asize);
    return bp;
  }

  // 맞는 크기 가용 블록이 없으면 힙을 확장한다. 
  extendsize = MAX(asize, MINBLOCKSIZE);
  if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
    return NULL;

  // Place the newly allocated block
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
  
  // Ignore spurious requests 
  if (!bp)
      return;

  size_t size = GET_SIZE(HDRP(bp));

  /* Set the header and footer allocated bits to 0, thus
   * freeing the block */
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));

  // Coalesce to merge any free blocks and add them to the list 
  coalesce(bp);
}

/*
 * extend_heap - 정수 짝수 배로 할당한다. 
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
  if ((bp = mem_sbrk(asize)) == (void *)-1)
    return NULL;

  /* Set the header and footer of the newly created free block, and
   * push the epilogue header to the back */
  PUT(HDRP(bp), PACK(asize, 0));
  PUT(FTRP(bp), PACK(asize, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* Move the epilogue to the end */

  // Coalesce any partitioned free memory 
  return coalesce(bp); 
}

/*
 * find_fit - Attempts to find a free block of at least the given size in the free list.
 *
 * This function implements a first-fit search strategy for an explicit free list, which 
 * is simply a doubly linked list of free blocks.
 */
static void *find_fit(size_t size)
{
  // First-fit search 
  void *bp;

  /* 가용리스트 끝에는 프리블록의 헤더(할당비트1)가 놓여있다. */
  for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREE(bp)) {
    if (size <= GET_SIZE(HDRP(bp)))
      return bp; 
  }
  // Otherwise no free block was large enough
  return NULL; 
}

/*
 * remove_freeblock - Removes the given free block pointed to by bp from the free list.
 * 
 * The explicit free list is simply a doubly linked list. This function performs a removal
 * of the block from the doubly linked free list.
 */
static void remove_freeblock(void *bp)
{
  if(bp) {
    if (PREV_FREE(bp))
      NEXT_FREE(PREV_FREE(bp)) = NEXT_FREE(bp);
    else
      free_listp = NEXT_FREE(bp);
    if(NEXT_FREE(bp) != NULL)
      PREV_FREE(NEXT_FREE(bp)) = PREV_FREE(bp);
  }
}

/*
 * coalesce - Coalesces the memory surrounding block bp using the Boundary Tag strategy
 * proposed in the text (Page 851, Section 9.9.11).
 * 
 * Adjancent blocks which are free are merged together and the aggregate free block
 * is added to the free list. Any individual free blocks which were merged are removed
 * from the free list.
 */
static void *coalesce(void *bp)
{
  // Determine the current allocation state of the previous and next blocks 
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

  // Get the size of the current free block
  size_t size = GET_SIZE(HDRP(bp));

  /* 경우1 : 앞뒤 블록이 전부 할당 */

  /* 경우2 : 다음 블록이 가용
   * (bp) and the next block
   */
  if (prev_alloc && !next_alloc) {           // Case 2 (in text) 
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));  
    remove_freeblock(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  /* 경우3 : 이전 블록이 가용
   * size에 이전 블록 크기를 더하고, 이전 블록 가용리스트에서 제거
   * 이전 블록으로 bp 이동, 
   * 
   */
  else if (!prev_alloc && next_alloc) {      // Case 3 (in text) 
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
  else if (!prev_alloc && !next_alloc) {     // Case 4 (in text) 
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(HDRP(NEXT_BLKP(bp)));
    remove_freeblock(PREV_BLKP(bp));
    remove_freeblock(NEXT_BLKP(bp));
    bp = PREV_BLKP(bp);
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  // 병합한 가용 블록을 가용리스트 처음에 넣기
  // free_listp 초기 위치는 프리블록의 헤더였다.
  // 새로운 블록 bp가 들어오면, free_listp는 bp가 된다.
  NEXT_FREE(bp) = free_listp;
  PREV_FREE(free_listp) = bp;
  PREV_FREE(bp) = NULL;
  free_listp = bp;

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
  // Gets the total size of the free block 
  size_t fsize = GET_SIZE(HDRP(bp));

  // Case 1: Splitting is performed 
  if((fsize - asize) >= (MINBLOCKSIZE)) {

    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    remove_freeblock(bp);
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(fsize-asize, 0));
    PUT(FTRP(bp), PACK(fsize-asize, 0));
    coalesce(bp);
  }

  // 그 외에는 가용 블록 전부 할당시킨다.
  else {

    PUT(HDRP(bp), PACK(fsize, 1));
    PUT(FTRP(bp), PACK(fsize, 1));
    remove_freeblock(bp);
  }
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
  // If ptr is NULL, realloc is equivalent to mm_malloc(size)
  if (ptr == NULL)
    return mm_malloc(size);

  // If size is equal to zero, realloc is equivalent to mm_free(ptr)
  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }
    
  /* Otherwise, we assume ptr is not NULL and was returned by an earlier malloc or realloc call.
   * Get the size of the current payload */
  size_t asize = MAX(ALIGN(size) + DSIZE, MINBLOCKSIZE);
  size_t current_size = GET_SIZE(HDRP(ptr));

  void *bp;
  char *next = HDRP(NEXT_BLKP(ptr));
  size_t newsize = current_size + GET_SIZE(next);

  /* Case 1: Size is equal to the current payload size */
  if (asize == current_size)
    return ptr;

  // Case 2: Size is less than the current payload size 
  if ( asize <= current_size ) {

    if( asize > MINBLOCKSIZE && (current_size - asize) > MINBLOCKSIZE) {  

      PUT(HDRP(ptr), PACK(asize, 1));
      PUT(FTRP(ptr), PACK(asize, 1));
      bp = NEXT_BLKP(ptr);
      PUT(HDRP(bp), PACK(current_size - asize, 1));
      PUT(FTRP(bp), PACK(current_size - asize, 1));
      mm_free(bp);
      return ptr;
    }

    // allocate a new block of the requested size and release the current block
    bp = mm_malloc(asize);
    memcpy(bp, ptr, asize);
    mm_free(ptr);
    return bp;
  }

  // Case 3: Requested size is greater than the current payload size 
  else {

    // next block is unallocated and is large enough to complete the request
    // merge current block with next block up to the size needed and free the 
    // remaining block.
    if ( !GET_ALLOC(next) && newsize >= asize ) {

      // merge, split, and release
      remove_freeblock(NEXT_BLKP(ptr));
      PUT(HDRP(ptr), PACK(asize, 1));
      PUT(FTRP(ptr), PACK(asize, 1));
      bp = NEXT_BLKP(ptr);
      PUT(HDRP(bp), PACK(newsize-asize, 1));
      PUT(FTRP(bp), PACK(newsize-asize, 1));
      mm_free(bp);
      return ptr;
    }  
    
    // otherwise allocate a new block of the requested size and release the current block
    bp = mm_malloc(asize); 
    memcpy(bp, ptr, current_size);
    mm_free(ptr);
    return bp;
  }
}