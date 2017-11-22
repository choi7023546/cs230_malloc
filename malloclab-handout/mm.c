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
    "Mina Huh",
    /* First member's email address */
    "minarainbow@kaist.ac.kr",
    /* Second member's full name (leave blank if none) */
    "Jeeyoung Choi",
    /* Second member's email address (leave blank if none) */
    "choi7023546@kaist.ac.kr"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */ 
#define WSIZE 4 /* Word and header/footer size (bytes) */ 
#define DSIZE 8 /* Double word size (bytes) */ 
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */ 

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */ 
#define PACK(size, alloc) ((size) | (alloc)) 

/* Read and write a word at address p */ 
#define GET(p) (*(unsigned int *)(p)) 
#define PUT(p, val) (*(unsigned int *)(p) = (val)) 

/* Read the size and allocated fields from address p */ 
#define GET_SIZE(p) (GET(p) & ~0x7) 
#define GET_ALLOC(p) (GET(p) & 0x1) 

/* Given block ptr bp, compute address of its header and footer */ 
#define HDRP(bp) ((char *)(bp) - WSIZE) 
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, compute address of next and previous blocks */ 
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given block ptr bp, compute address of next and previous free blocks */
#define NEXT_FBLKP(bp) (*(char **)(bp + WSIZE)) 
#define PREV_FBLKP(bp) (*(char **)(bp))

/* Given block ptr bp, set previous and next pointers*/
#define SET_PREV_FBLKP(bp,prev) (*((char **)(bp)) = prev)
#define SET_NEXT_FBLKP(bp,next) (*((char **)(bp+WSIZE)) = next)

/* marking start of heap_list and free_list  */
static void* heap_listp = NULL;
static void* free_listp = NULL;

/* function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void insert(void *bp);
static void remove(void *bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) // no need to change
{
     /* Create the initial empty heap */
     if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); /* Alignment padding */ 
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* Epilogue header */ 
    free_listp = heap_listp + DSIZE; 

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */ 
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
        return -1;
    return 0; 
}

 static void *extend_heap(size_t words) // no need to change
 {  
    char *bp; 
    size_t size; 
    /* Allocate an even number of words to maintain alignment */  
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 
    if ((long)(bp = mem_sbrk(size)) == -1) 
    return NULL; 

    /* Initialize free block header/footer and the epilogue header */ 
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */ 
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */ 
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 

    /* Coalesce if the previous block was free */ 
    return coalesce(bp);
} 

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) // no need to change
{
     size_t asize; /* Adjusted block size */ 
     size_t extendsize; /* Amount to extend heap if no fit */ 
     char *bp; 
    
    /* Ignore spurious requests */ 
    if (size == 0)
        return NULL; 
    /* Adjust block size to include overhead and alignment reqs. */ 
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */ 
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize); 
        return bp; }

    /* No fit found. Get more memory and place the block */ 
    extendsize = MAX(asize,CHUNKSIZE); 
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) 
        return NULL; 
    place(bp, asize); 
    return bp; 
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) // no need to change
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp); 
}

 static void *coalesce(void *bp) 
 { 
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); 
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); 
    size_t size = GET_SIZE(HDRP(bp)); 

    if (prev_alloc && next_alloc) { /* Case 1 */  
    return bp; 
    } 

    else if (prev_alloc && !next_alloc) { /* Case 2 */ 
    size += GET_SIZE(HDRP(NEXT_BLKP(bp))); 
    PUT(HDRP(bp), PACK(size, 0)); 
    PUT(FTRP(bp), PACK(size,0)); 
    } 

    else if (!prev_alloc && next_alloc) { /* Case 3 */ 
    size += GET_SIZE(HDRP(PREV_BLKP(bp))); 
    PUT(FTRP(bp), PACK(size, 0)); 
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
    bp = PREV_BLKP(bp); 
    } 

    else { /* Case 4 */ 
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); 
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); 
    bp = PREV_BLKP(bp); } 
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
/*void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}*/

/* we can devide 2 cases. case1 : split free block. remove bp from the free list and insert new free block that splitted 
case 2 : not split free block. just update size of free block.
 */
static void place(void *bp, size_t asize) 
{
    size_t csize = GET_SIZE(HDRP(bp)); 
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

 static void *find_fit(size_t asize) // modified to explicit free list
 { 
    /* First fit search */ 
     void *bp; 
     for (bp = free_listp; bp!=NULL ;  bp = NEXT_FBLKP(bp)) {
        if ( ( asize <= GET_SIZE(HDRP(bp)) ) ) {
            return bp;
        }
    }
    return NULL; /* No fit */
}



/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
     // bp is NULL, equivalent to malloc call
    if (bp == NULL) {
       return mm_malloc(size);
    }

    // size is 0, equivalent to free
    if (size == 0) {
        mm_free(bp);
        return NULL;
    }

    size_t old_size = GET_SIZE(HDRP(bp));
    size_t asize, nextblc_size, copy_size;
    void *oldbp = bp;
    void *newbp;

    // check whether realloc is shrinking or expanding
    // get the adjusted size of the request
    if (size <= DSIZE) {
        asize = 2*DSIZE;
    } else {
        asize = DSIZE * ((size + DSIZE + (DSIZE-1)) / DSIZE);
    }

    // if the same size
    if (asize == old_size) {
        return bp;
    }
    // if shrinking
    if (asize < old_size) {
        // we split the block if the remainder is big enough
        if (old_size - asize >= 2*DSIZE) {
            // shrink the old block
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            // construct the new block
            newbp = NEXT_BLKP(bp);
            PUT(HDRP(newbp), PACK(old_size - asize, 0));
            PUT(FTRP(newbp), PACK(old_size - asize, 0));
        }
        // otherwise we don't do anything
        return oldbp;
    }

    // if expanding
    else {
        // we check if we can make the block large enough by
        // coalescing with the block to its right
        nextblc_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        if (!GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
            if (nextblc_size + old_size >= asize) {
                // we coalesce
                // first, delete the next block from free list
               // delete(NEXT_BLKP(bp));
                // then we construct a large allocated block
                PUT(HDRP(bp), PACK(old_size + nextblc_size, 1));
                PUT(FTRP(bp), PACK(old_size + nextblc_size, 1));
                return oldbp;
            }
        }

        // coalescing with the left won't do any good because
        // everything still needs to be copied over so in all
        // other cases, we free the current block and allocate
        // a new block and copy everything over
        newbp = mm_malloc(size);
        copy_size = old_size - DSIZE;
        memcpy(newbp, oldbp, copy_size);
        mm_free(bp);
        return newbp;
    }

}

