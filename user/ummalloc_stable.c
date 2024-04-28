//ummalloc_test amptjp-bal.rep

#include "kernel/types.h"

//
#include "user/user.h"

//
#include "ummalloc.h"

#define DEBUG 1
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(uint)))


/* Base constants and macros */
#define WSIZE		4		 /* Word and header/footer size (bytes) */
#define DSIZE		8		 /* Double word size (bytes) */
#define MINISIZE  16 /* size of miniblock */
#define CHUNKSIZE	(1<<12)	/* Extend heap by this amount (bytes) */
#define ADDR_SIZE 8
#define LEAST_SIZE (WSIZE + ADDR_SIZE * 2 + WSIZE)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word*/
#define PACK(size, alloc)	((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)		(*(unsigned int *)(p))
#define PUT(p, val)	(*(unsigned int *)(p) = val)

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)		 (GET(p) & ~0x7)
#define GET_ALLOC(p)	 (GET(p) & 0x1)

/* Get and Set pointer value by ptr at address p */
#define GET_PTR_VAL(p)	(*(unsigned long *)(p))
#define SET_PTR(p, ptr)	(*(unsigned long *)(p) = (unsigned long)(ptr))

/* Read and write pred and succ pointer at address p */
#define GET_PRED(p)	((char *)(*(unsigned long *)(p)))
#define GET_SUCC(p)	((char *)(*(unsigned long *)(p + ADDR_SIZE)))
#define SET_PRED(p, ptr)	(SET_PTR((char *)(p), ptr))
#define SET_SUCC(p, ptr)	(SET_PTR(((char *)(p)+(ADDR_SIZE)), ptr))

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)	((char* )(bp) - WSIZE)
#define FTRP(bp)	((char* )(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Heap prologue block pointer*/
static void *free_list_headp, *free_list_tailp;


void *find(uint size) {
  void *rp = 0;
  uint ret = 0x7fffffff;
  for(void *p = free_list_headp; p != free_list_tailp; p = GET_SUCC(p)) {
    int tsize = GET_SIZE(HDRP(p));
    
    if(tsize >= size && tsize < ret) {
      rp = p;
      ret = tsize;
    }
  }
  
  return rp;
}
// void *find(uint size) {
//   for(void *p = free_list_headp; p != free_list_tailp; p = GET_SUCC(p)) {
//     int tsize = GET_SIZE(HDRP(p));
//     if(tsize >= size) {
//       return p;
//     }
//   }
//   return 0;
// }
void insert(void *ptr) {
  SET_PRED(ptr, free_list_headp);
  SET_SUCC(ptr, GET_SUCC(free_list_headp));
  SET_PRED(GET_SUCC(free_list_headp), ptr);
  SET_SUCC(free_list_headp, ptr);
}
void delete(void *ptr) {
  if(DEBUG) printf("delete %d-%d-%d\n",GET_PRED(ptr), ptr, GET_SUCC(ptr));
  SET_SUCC(GET_PRED(ptr), GET_SUCC(ptr));
  SET_PRED(GET_SUCC(ptr), GET_PRED(ptr));
}

/*
 * mm_init - initialize the malloc package.
 */


int mm_init(void) {
  free_list_headp = sbrk(WSIZE * 12);
  if(free_list_headp == (void*)-1) return -1;

  PUT(free_list_headp, 0); //alignment

  //Prologue: 24/1, pre(2), suc(2), 24/1
  PUT(free_list_headp + 1 * WSIZE, PACK(24, 1));
  free_list_headp += 2 * WSIZE;
  free_list_tailp = NEXT_BLKP(free_list_headp);
  SET_PRED(free_list_headp, 0);
  SET_SUCC(free_list_headp, free_list_tailp);
  PUT(free_list_headp + 2 * ADDR_SIZE, PACK(24, 1));

  //Epilogue: 0/1, pre(2)
  PUT(HDRP(free_list_tailp), PACK(0, 1));				
  SET_PRED(free_list_tailp, free_list_headp);
  PUT(free_list_tailp + ADDR_SIZE, PACK(0, 0));	//alignment

	//if (new_alloc(CHUNKSIZE/WSIZE) == 0)  return -1;
  return 0;
}

void *new_alloc(int wcnt) {
  if(DEBUG) printf("alloc in\n");
  int size = (wcnt & 1? wcnt + 1 : wcnt) * WSIZE;
  char *tp = sbrk(size);
  if((long)tp == -1) return 0;
  char *p = free_list_tailp;

  PUT(HDRP(p), PACK(size, 0));
  free_list_tailp = NEXT_BLKP(p);
  //pred doesn't need reset
  SET_SUCC(p, free_list_tailp);
  PUT(FTRP(p), PACK(size, 0));

  //Epilogue
  PUT(HDRP(free_list_tailp), PACK(0, 1));
  SET_PRED(free_list_tailp, p);
  PUT(free_list_tailp + DSIZE, PACK(0, 0));	//alignment
  return merge(p);
}

void *merge(void* p) {
  //Check if prev and next are spare
  void *prev_ptr = PREV_BLKP(p);
  void *next_ptr = NEXT_BLKP(p);

  uint prev_spare = !GET_ALLOC(HDRP(prev_ptr)), next_spare = !GET_ALLOC(HDRP(next_ptr));
  if(DEBUG) printf("p=%d prev=%d spare=%d next=%d spare=%d\n",p,prev_ptr,prev_spare,next_ptr,next_spare);
  if(prev_spare && next_spare) {
    int siz1 = GET_SIZE(HDRP(prev_ptr)), siz2 = GET_SIZE(HDRP(p)), siz3 = GET_SIZE(HDRP(next_ptr));
    delete(prev_ptr), delete(p), delete(next_ptr);
    insert(prev_ptr);
    PUT(HDRP(prev_ptr), PACK(siz1 + siz2 + siz3, 0));
    PUT(FTRP(prev_ptr), PACK(siz1 + siz2 + siz3, 0));
    return prev_ptr;
  } else if (prev_spare && !next_spare) {
    int siz1 = GET_SIZE(HDRP(prev_ptr)), siz2 = GET_SIZE(HDRP(p));
    delete(prev_ptr), delete(p);
    insert(prev_ptr);
    PUT(HDRP(prev_ptr), PACK(siz1 + siz2, 0));
    PUT(FTRP(prev_ptr), PACK(siz1 + siz2, 0));
    return prev_ptr;
  } else if (!prev_spare && next_spare) {
    int siz1 = GET_SIZE(HDRP(p)), siz2 = GET_SIZE(HDRP(next_ptr));
    delete(p), delete(next_ptr);
    insert(p);
    PUT(HDRP(p), PACK(siz1 + siz2, 0));
    PUT(FTRP(p), PACK(siz1 + siz2, 0));
    return p;
  } else {
    return p;
  }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void place(void *p, uint size) {
  uint tsize = GET_SIZE(HDRP(p));
  if(DEBUG) printf("place tsize=%d size=%d\n",tsize,size);
  if(tsize - size >= LEAST_SIZE) {
    PUT(HDRP(p), PACK(size, 1));
    PUT(FTRP(p), PACK(size, 1));
    void *np = NEXT_BLKP(p);
    if(DEBUG) printf("place p=%d np=%d pred=%d succ=%d\n",p,np,GET_PRED(p),GET_SUCC(p));
    PUT(HDRP(np), PACK(tsize - size, 0));
    SET_PRED(np, GET_PRED(p));
    SET_SUCC(np, GET_SUCC(p));
    SET_PRED(GET_SUCC(p), np);
    SET_SUCC(GET_PRED(p), np);
    PUT(FTRP(np), PACK(tsize - size, 0));
  } else {
    PUT(HDRP(p), PACK(tsize, 1));
    delete((char*)p);
    PUT(FTRP(p), PACK(tsize, 1));
  }
}
void *mm_malloc(uint size) {
  if(DEBUG) printf("malloc in\n"); 
  int newsize = MAX(ALIGN(size + WSIZE * 2), LEAST_SIZE);
  void *p = find(newsize);
  if(p == 0) {
    p = new_alloc(newsize / WSIZE);
    if(DEBUG) printf("new alloc p=%d\n", p);
  } 
  if(p == 0) return 0;
  place(p, newsize);
  if(DEBUG) printf("malloc out\n");
  return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  if(DEBUG) printf("free in %d\n", ptr);
  PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));
  insert((char*)ptr);
  PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));
  merge(ptr);
  if(DEBUG) printf("free out\n");
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, uint size) {
  if(DEBUG) printf("realloc in %d to %d\n", ptr, size);
  uint newsize = MAX(ALIGN(size + WSIZE * 2), LEAST_SIZE);

  if (size == 0) {
    if(DEBUG) printf("realloc out\n");
    mm_free(ptr);
    return 0;
  }

  uint tsize = GET_SIZE(HDRP(ptr));
  if (newsize == tsize) {
    if(DEBUG) printf("realloc out\n");
    return ptr;
  } else if (newsize > tsize) {
    if(DEBUG) printf("expand\n");
    void *np = NEXT_BLKP(ptr);
    uint nsize = GET_SIZE(HDRP(np));
    if(!GET_ALLOC(HDRP(np)) && nsize + tsize >= newsize) {
      if(nsize + tsize - newsize >= LEAST_SIZE) {
        PUT(HDRP(ptr), PACK(newsize, 1));

        //wait for the update of free list
        void *nsp = NEXT_BLKP(ptr);
        void *tpre = GET_PRED(np), *tsuc = GET_SUCC(np);
        SET_PRED(nsp, tpre);
        SET_SUCC(nsp, tsuc);
        SET_PRED(tsuc, nsp);
        SET_SUCC(tpre, nsp);
        PUT(HDRP(nsp), PACK(nsize + tsize - newsize, 0));
        PUT(FTRP(nsp), PACK(nsize + tsize - newsize, 0));

        PUT(FTRP(ptr), PACK(newsize, 1));
      } else {
        PUT(HDRP(ptr), PACK(nsize + tsize, 1));
        delete(np);
        PUT(FTRP(ptr), PACK(nsize + tsize, 1));
      }
      if(DEBUG) printf("realloc out\n");
      return ptr;
    } else {
      void *ret = find(newsize);
      if (ret == 0) {
        ret = new_alloc(newsize / WSIZE);
        if (ret == 0) return 0;
      }
      place(ret, newsize);
      memcpy(ret, ptr, newsize - WSIZE * 2);
      mm_free(ptr);
      if(DEBUG) printf("realloc out\n");
      return ret;
    }
  } else { //newsize < tsize
    if(DEBUG) printf("SHRINK\n");
    if(tsize - newsize >= LEAST_SIZE) {
      PUT(HDRP(ptr), PACK(newsize, 1));

      //wait for the update of free list
      void *nsp = NEXT_BLKP(ptr);
      PUT(HDRP(nsp), PACK(tsize - newsize, 0));
      insert(nsp);
      PUT(FTRP(nsp), PACK(tsize - newsize, 0));

      PUT(FTRP(ptr), PACK(newsize, 1));
      merge(nsp);
    }
    //else: nothing to do
    if(DEBUG) printf("realloc out\n");
    return ptr;
  }
  return 0;
}
