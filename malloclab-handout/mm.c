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
static void add_to_list(void *bp);
static void remove_from_list(void *bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) // no need to change
{
     /* Create the initial empty heap */
     if ((heap_listp = mem_sbrk(8*WSIZE)) == NULL)
        return -1;
    PUT(heap_listp, 0); /* Alignment padding */ 
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* Epilogue header */ 
    free_listp = heap_listp + DSIZE; 

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */ 
    if (extend_heap(4) == NULL) 
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
    if (size < 16){
    size = 16;
  }
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
    //printf("what" );
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
        return bp; 
    }
    //printf("hi");
    /* No fit found. Get more memory and place the block */ 
    extendsize = MAX(asize,CHUNKSIZE); 
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) 
        return NULL; 
    place(bp, asize); 
    return bp; 
    //printf("hi2");
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) // no need to change
{
    //printf("hi");
    if (bp==NULL)
        return;
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp); 
    //printf("hi");
}

static void *coalesce(void *bp)
  {
     size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
     size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
     size_t size = GET_SIZE(HDRP(bp));

     if (prev_alloc && next_alloc) { /* Case 1 */
         add_to_list(bp);
         return bp;
     }

     else if (prev_alloc && !next_alloc) { /* Case 2 */
     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
     remove_from_list(NEXT_BLKP(bp));
     PUT(HDRP(bp), PACK(size, 0));
     PUT(FTRP(bp), PACK(size,0));
     add_to_list(bp);
     return bp;
     }

     else if (!prev_alloc && next_alloc) { /* Case 3 */
     size += GET_SIZE(HDRP(PREV_BLKP(bp)));
     remove_from_list(PREV_BLKP(bp));
     PUT(FTRP(bp), PACK(size, 0));
     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
     bp = PREV_BLKP(bp);
     add_to_list(bp);
     return bp;
     }

     else { /* Case 4 */
     size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
     remove_from_list(PREV_BLKP(bp));
     remove_from_list(NEXT_BLKP(bp));
     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
     PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
     bp = PREV_BLKP(bp);
     add_to_list(bp);
     return bp;
     }
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
    //printf("hi");
    size_t csize = GET_SIZE(HDRP(bp)); 
    if((csize-asize) < (2*DSIZE)){
        PUT(HDRP(bp), PACK(csize, 1)); 
        PUT(FTRP(bp), PACK(csize, 1)); 
        remove_from_list(bp); 
    }
    else{
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        remove_from_list(bp);
        bp = NEXT_BLKP(bp);

        PUT(HDRP(bp), PACK(csize-asize, 0)); 
        PUT(FTRP(bp), PACK(csize-asize, 0)); 
        add_to_list(bp);
    }
    //printf("hi");   
}

 static void *find_fit(size_t asize) // modified to explicit free list
 {
    void *bp;
    for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FBLKP(bp) ){
        if ((int) asize <= GET_SIZE(HDRP(bp)) ) {
          //last_size = asize;
          return bp;
        }
    }
    return NULL;
}



void *mm_realloc(void *ptr, size_t size)

{

    void *oldptr = ptr;

    void *newptr;

    void *temp;

    size_t tempsize;

    size_t copySize;

    

    //get allocation status of next and previous free blocks.  

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(oldptr)));

    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(oldptr)));

    

    //get size of the current block

    size_t size_prev = GET_SIZE(HDRP(oldptr));

    int increase;

    

    //decide if the new size constitutes and increase or not

    if(size_prev  < size + DSIZE) 

            increase = 1;

    else

            increase = 0;

    

    //if size is equal to zero, the call is equivalent to mm free(ptr)

    if (size == 0){

    mm_free(ptr);

        newptr = 0;

        return NULL;

    }

    

    //if ptr is NULL, the call is equivalent to mm malloc(size)

    if (oldptr == NULL) 

    return mm_malloc(size);



    // if shrinking ptr and released space will be large enough to be a block then shrink allocated block and create new block.  

    if(increase == 0 && (size_prev - size - DSIZE) > (2*DSIZE)){ 

            

        // Adjust block size to include overhead and alignment reqs.

        if (size <= DSIZE)

                size = 2*DSIZE;

         else

        size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); // align size

    

    //if the adjusted size still leaves enough space for a free block continue with creating new free block and shrunk allocated block. 

    if((size_prev - size) > (2*DSIZE)){



     //reset header and footer of old block for the new (shrunk) size

         PUT(HDRP(oldptr), PACK(size, 1)); 

     PUT(FTRP(oldptr), PACK(size, 1)); 

     

     // set new ptr to old ptr

     newptr = oldptr;

     

     // reset old pointer to the new (empty) block

     oldptr =  (NEXT_BLKP(newptr)); 

     

     //set header and footer for new (empty) block with remaining size

     PUT(HDRP(oldptr), PACK(size_prev - size, 0));

     PUT(FTRP(oldptr), PACK(size_prev - size, 0));

    

    //coalesce new (free) block (adds to free list)

    coalesce(oldptr);

    

    //return pointer to shurnk block

    return newptr;

    }

    }

    

    //if shrinking ptr and released space too small to be a block or size is the same return same ptr because no change is necessary 

    if(increase == 0) {

            return ptr;

    }

    

    // else if expanding ptr 

    else {

            

            //if next and prev are unallocated and combining the three blocks would fufill new size requirement merge blocks.

            if (next_alloc == 0 && prev_alloc == 0 && ((GET_SIZE(HDRP(PREV_BLKP(oldptr)))) + (GET_SIZE(HDRP(NEXT_BLKP(oldptr)))) + size_prev) >= (size + DSIZE)){

                    

                    //set new ptr to the prev block since this will be the address of the expanded block.  

                newptr = PREV_BLKP(oldptr);

                

                //temp set to next block.

                temp = NEXT_BLKP(oldptr);

                

                //temp size is size of next and prev blocks -- ie the added size to the current block.

                    tempsize = GET_SIZE(FTRP(newptr)) + GET_SIZE(FTRP(temp));

                    

                    //remove next block and previous block from the free list since they will no longer exist.  

                    remove_from_list(PREV_BLKP(oldptr));

                    remove_from_list(NEXT_BLKP(oldptr));

                    

                    // Adjust block size to include overhead and alignment reqs.

                    if (size <= DSIZE)

                        size = 2*DSIZE;

            else

                size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); 

        

            // if not big enough for new free block make new block take all space

                    if((tempsize + size_prev) < (size + 2*DSIZE))

                         size = tempsize + size_prev;

                 

                //set header to reflect new (expanded) size.

                    PUT(HDRP(newptr), PACK(size, 1));

                    

                    // calculate copy size and copy memory from old block to new (expanded) block

                    copySize = GET_SIZE(HDRP(oldptr));

                    memcpy(newptr, oldptr, copySize);

                    

                    //set footer to reflect new (expanded) size. 

                    PUT(FTRP(newptr), PACK(size, 1));

                                

                    //if new free block initialize i

                    if((tempsize + size_prev) >= (size + 2*DSIZE)){ 

                

                            // set new pointer to the new (empty) block

                            temp = NEXT_BLKP(newptr); 

                            

                            // set header and foot for new (empty) block with remaining size

                PUT(HDRP(temp), PACK(tempsize + size_prev - size, 0));

                PUT(FTRP(temp), PACK(tempsize + size_prev - size, 0));



                //add new (free) block to the free list

                add_to_list(temp);

                    }

                    //return expanded block.

                    return newptr;                      

            }           

            

            //if next is unallocated and combining next with this block would fufill new size requirement merge the blocks

            else if(next_alloc == 0 && ((GET_SIZE(HDRP(NEXT_BLKP(oldptr)))) + size_prev) >= (size + DSIZE)){

                    

                    //temp set to next block

                    temp = NEXT_BLKP(oldptr);

                    

                    //temp size is size of next block

                    tempsize = GET_SIZE(FTRP(temp));

                    

                    //remove next block from the free list since it will no longer exist. 

                    remove_from_list(NEXT_BLKP(ptr));

                    

                    // Adjust block size to include overhead and alignment reqs.

                    if (size <= DSIZE)

                        size = 2*DSIZE;

            else

                size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

        

                // if not big enough for new free block make new block take all space

                    if((tempsize + size_prev) < (size + 2*DSIZE)) 

                         size = tempsize + size_prev;

                

                    //set header and footer for block to reflect new (expanded) sizes

                    PUT(HDRP(oldptr), PACK(size, 1));

                    PUT(FTRP(oldptr), PACK(size, 1)); 

                    

                    //if new free block initialize it

                    if((tempsize + size_prev) >= (size + 2*DSIZE)){ 

                            

                // set new pointer to the new (empty) block

                            newptr = NEXT_BLKP(oldptr);

                            

                            // set header and foot for new (empty) block with remaining size

                PUT(HDRP(newptr), PACK(tempsize + size_prev - size, 0));

                PUT(FTRP(newptr), PACK(tempsize + size_prev - size, 0));

                

                //add new (free) block to the free list

                add_to_list(newptr);

                    }

                    //return expanded block.

                    return oldptr;                  

            }

            

            //if prev is unallocated and combining prev with this block would fufill new size requirement merge the blocks

            else if(prev_alloc == 0 && ((GET_SIZE(HDRP(PREV_BLKP(oldptr)))) + size_prev) >= (size + DSIZE)){

                    

                    //set new ptr to the prev block since this will be the address of the expanded block.  

                    newptr = PREV_BLKP(oldptr);

                    

                    //tempsize is size of prev block. 

                    tempsize = GET_SIZE(FTRP(newptr));

                    

                    //remove prev block from the free list

                    remove_from_list(PREV_BLKP(oldptr));

                    

                    // Adjust block size to include overhead and alignment reqs.

                    if (size <= DSIZE)

                        size = 2*DSIZE;

            else

                size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

        

            // if not big enough for new free block make new block take all space

                    if((tempsize + size_prev) < (size + 2*DSIZE))

                         size = tempsize + size_prev;

                

                    //set header to reflect new (expanded) size.

                    PUT(HDRP(newptr), PACK(size, 1)); 

                    

                    // calculate copy size and copy memory from old block to new (expanded) block

                    copySize = GET_SIZE(HDRP(oldptr));

                    memcpy(newptr, oldptr, copySize);

                    

                    //set footer to reflect new (expanded) size.

                    PUT(FTRP(newptr), PACK(size, 1)); // resize old 

                    

                    //if new free block initialize it

                    if((tempsize + size_prev) >= (size + 2*DSIZE)){

                            

                            // set new pointer to the new (empty) block

                temp = NEXT_BLKP(newptr);

                

                // set header and foot for new (empty) block with remaining size

                PUT(HDRP(temp), PACK(tempsize + size_prev - size, 0));

                PUT(FTRP(temp), PACK(tempsize + size_prev - size, 0));

                

                //add new (free) block to the free list

                add_to_list(temp);

                    }

                    //return expanded block.

                    return newptr;      

            }



            //else if next and previous blocks are allocated -- default case

            

            //set new pointer to newly allocated block of size

            newptr = mm_malloc(size);

            

            //calculate copy size with a maximum of size.  (size should never be less than copySize but left in for safety)

            copySize = GET_SIZE(HDRP(oldptr));

        if (size < copySize)

            copySize = size;

        

        //copy memory to new block

        memcpy(newptr, oldptr, copySize);

        

        //free old memory block

        mm_free(oldptr);

        

        //return new memory block.

        return newptr;

    }

}



static void add_to_list(void *bp){
    SET_PREV_FBLKP(free_listp, bp);
    SET_NEXT_FBLKP(bp, free_listp);
    SET_PREV_FBLKP(bp,NULL);
    free_listp = bp;
}

static void remove_from_list(void *bp) {
 if (!PREV_FBLKP(bp)) {
     SET_PREV_FBLKP( NEXT_FBLKP(bp), NULL );
     free_listp = NEXT_FBLKP(bp);
 }
 else {
     SET_PREV_FBLKP( NEXT_FBLKP(bp), PREV_FBLKP(bp) );
     SET_NEXT_FBLKP( PREV_FBLKP(bp), NEXT_FBLKP(bp) );
 }



}

