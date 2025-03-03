/*
 * malloc.c: safe wrappers around malloc, realloc, free, strdup
 */



// Define this to use nearly 32Mb of upper memory available on the GP2X
// which isn't normally accessible (or to fake such memory being available
// on non-GP2X hardware).
// #define OPTION_USE_UPPER_MEMORY

// Define this to pretend that "lower memory" is full and see what happens.
// #define DEBUG_PRETEND_MEMORY_FULL

// Define this to enable debugging messages for the upper memory area.
// #define DEBUG_UPPER_MEMORY  // Upper memory access on the GP2X.


#include <stdlib.h>
#include <string.h>
#include "puzzles.h"
#include "malloc.h"

#include <stdio.h>
#include <fcntl.h>
//#include <sys/mman.h>
#include <unistd.h>

int Uppermemfd;
void *UpperMem;

#define BLOCKSIZE 1024

int TakenSize[0x2000000 / BLOCKSIZE];

#define SetTaken(Start, Size) TakenSize[(Start - 0x2000000) / BLOCKSIZE] = (Size - 1) / BLOCKSIZE + 1


/*
 * smalloc should guarantee to return a useful pointer - Halibut
 * can do nothing except die when it's out of memory anyway.
 */
void *smalloc(size_t size)
{
    void *p=NULL;
#ifndef DEBUG_PRETEND_MEMORY_FULL
    p = malloc(size);
#endif

    if (!p)
    {
#ifdef OPTION_USE_UPPER_MEMORY
 #ifdef DEBUG_UPPER_MEMORY
        printf("Out of normal memory.  Trying upper memory...\n");
 #endif
        p = UpperMalloc(size);
        if (!p)
#endif
            fatal("Out of memory");
    };
    return p;
}

/*
 * sfree should guaranteeably deal gracefully with freeing NULL
 */
void sfree(void *p) {
    if (p)
    {
#ifdef OPTION_USE_UPPER_MEMORY
        int i = (((int)p) - ((int)UpperMem));
        if (i < 0 || i >= 0x2000000)
        {
 #ifdef DEBUG_UPPER_MEMORY
            printf("Requested free of not UpperMalloced mem: %p\nTrying normal free...", p);
 #endif
#endif
            free(p);
#ifdef OPTION_USE_UPPER_MEMORY
        }
        else
        {
            UpperFree(p);
        };
#endif
    }
}

/*
 * srealloc should guaranteeably be able to realloc NULL
 */
void *srealloc(void *p, size_t size)
{
    void *q=NULL;

    if (p)
    {
#ifdef OPTION_USE_UPPER_MEMORY
        int i = (((int)p) - ((int)UpperMem));
        if (i < 0 || i >= 0x2000000)
#endif
            q = realloc(p, size);
        if (!q)
        {
#ifdef OPTION_USE_UPPER_MEMORY
            q = UpperMalloc(size);
            if(!q)
                fatal("Out of memory");

            memcpy(q, p, size);
            if(memcmp(q,p,size) != 0)
#endif
                fatal("Out of memory");    
        };
    }
    else
    {
	q = smalloc(size);
    };

    return q;
}

/*
 * dupstr is like strdup, but with the never-return-NULL property
 * of smalloc (and also reliably defined in all environments :-)
 */
char *dupstr(const char *s) {
    char *r = smalloc(1+strlen(s));
    strcpy(r,s);
    return r;
}

void * UpperMalloc(size_t size)
{
  int i = 0, j;
ReDo:
  for (; TakenSize[i]; i += TakenSize[i]);
  if (i >= 0x2000000 / BLOCKSIZE) {
    printf("UpperMalloc out of mem!");
    return NULL;
  }
  int BSize = (size - 1) / BLOCKSIZE + 1;
  for(j = 1; j < BSize; j++) {
    if (TakenSize[i + j]) {
      i += j;
      goto ReDo; //OMG Goto, kill me.
    }
  }
  
  TakenSize[i] = BSize;
  void* mem = ((char*)UpperMem) + i * BLOCKSIZE;
  memset(mem, 0, size);
  return mem;
}

//Releases UpperMalloced memory
void UpperFree(void* mem)
{
  int i = (((int)mem) - ((int)UpperMem));

  if (i < 0 || i >= 0x2000000)
  {
    printf("Requested UpperFree of non-UpperMalloc'd memory\n");
  }
  else
  {
    if (i % BLOCKSIZE)
      fprintf(stderr, "delete error: %p\n", mem);
    TakenSize[i / BLOCKSIZE] = 0;
  }
}

//Returns the size of a UpperMalloced block.
int GetUpperSize(void* mem)
{
  int i = (((int)mem) - ((int)UpperMem));
  if (i < 0 || i >= 0x2000000)
  {
    fprintf(stderr, "GetUpperSize of not UpperMalloced mem: %p\n", mem);
    return -1;
  }
  return TakenSize[i / BLOCKSIZE] * BLOCKSIZE;
}

void InitMemPool() {
#ifdef OPTION_USE_UPPER_MEMORY

 #ifndef TARGET_GP2X
  printf("Faking an upper memory area\n");
  UpperMem = malloc(0x2000000);
  if(!UpperMem)
     fatal("Upper memory area failure.\n");
 #else
  //Try to apply MMU hack.
  int mmufd = open("/dev/mmuhack", O_RDWR);
  if(mmufd < 0) {
    system("/sbin/insmod mmuhack.o");
    mmufd = open("/dev/mmuhack", O_RDWR);
  }
  if(mmufd < 0)
  {
    printf("MMU hack failed");
  }
  else
  {
    close(mmufd);
  }

  Uppermemfd = open("/dev/mem", O_RDWR);
  UpperMem = mmap(0, 0x2000000, PROT_READ | PROT_WRITE, MAP_SHARED, Uppermemfd, 0x2000000);
 #endif

  memset(TakenSize, 0, sizeof(TakenSize));

  SetTaken(0x3000000, 0x80000); // Video decoder (you could overwrite this, but if you
                                // don't need the memory then be nice and don't)
  SetTaken(0x3101000, 153600);  // Primary frame buffer
  SetTaken(0x3381000, 153600);  // Secondary frame buffer (if you don't use it, uncomment)
  SetTaken(0x3600000, 0x8000);  // Sound buffer
#endif
}

void DestroyMemPool()
{
#ifdef OPTION_USE_UPPER_MEMORY
 #ifndef TARGET_GP2X
    printf("Destroying fake upper memory\n");
    free(UpperMem);
 #else
    close (Uppermemfd);
 #endif
    UpperMem = NULL;
#endif
}
