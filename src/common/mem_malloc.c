#include <stdlib.h>
#include "db_config.h"
#include "mem_malloc.h"

typedef struct BlockHdr BlockHdr;
struct BlockHdr {
    BlockHdr *next;
};

typedef struct {
    System sys;
    BlockHdr *globalBlocks;         /* next global heap space location */
    BlockHdr *localBlocks;          /* next local heap space location */
    size_t localHeapUsed;           /* amount of local heap space currently allocated */
    size_t totalHeapUsed;           /* total amount of heap space currently allocated */
    size_t maxHeapUsed;             /* maximum amount of heap space allocated so far */
} MySystem;

/* MemInit - initialize the memory allocator */
System *MemInit(void)
{
    MySystem *sys;
    
    /* allocate the system interface structure */
    if (!(sys = (MySystem *)malloc(sizeof(MySystem))))
        return NULL;
        
    /* initialize */
    sys->globalBlocks = NULL;
    sys->localBlocks = NULL;
    sys->localHeapUsed = 0;
    sys->totalHeapUsed = 0;
    sys->maxHeapUsed = 0;
    
    /* return the system interface structure */
    return (System *)sys;
}

/* MemFree - free all allocated memory */
void MemFree(System *sysbase)
{
    MySystem *sys = (MySystem *)sysbase;
    BlockHdr *hdr, *next;
    for (hdr = sys->globalBlocks; hdr != NULL; hdr = next) {
        next = hdr->next;
        free(hdr);
    }
    xbLocalFreeAll(sysbase);
    free(sys);
}

/* xbGlobalAlloc - allocate memory from the global heap */
void *xbGlobalAlloc(System *sysbase, size_t size)
{
    MySystem *sys = (MySystem *)sysbase;
    BlockHdr *hdr;
    size = ROUND_TO_WORDS(size + sizeof(BlockHdr));
    if (!(hdr = (BlockHdr *)malloc(size)))
        return NULL;
    hdr->next = sys->globalBlocks;
    sys->globalBlocks = hdr;
    if ((sys->totalHeapUsed += size) > sys->maxHeapUsed)
        sys->maxHeapUsed = sys->totalHeapUsed;
    return (void *)++hdr;
}

/* xbLocalAlloc - allocate memory from the local heap */
void *xbLocalAlloc(System *sysbase, size_t size)
{
    MySystem *sys = (MySystem *)sysbase;
    BlockHdr *hdr;
    size = ROUND_TO_WORDS(size + sizeof(BlockHdr));
    if (!(hdr = (BlockHdr *)malloc(size)))
        return NULL;
    hdr->next = sys->localBlocks;
    sys->localBlocks = hdr;
    sys->localHeapUsed += size;
    if ((sys->totalHeapUsed += size) > sys->maxHeapUsed)
        sys->maxHeapUsed = sys->totalHeapUsed;
    return (void *)++hdr;
}

/* xbLocalFreeAll - free all local memory */
void xbLocalFreeAll(System *sysbase)
{
    MySystem *sys = (MySystem *)sysbase;
    BlockHdr *hdr, *next;
    for (hdr = sys->localBlocks; hdr != NULL; hdr = next) {
        next = hdr->next;
        free(hdr);
    }
    sys->localBlocks = NULL;
    sys->totalHeapUsed -= sys->localHeapUsed;
    sys->localHeapUsed = 0;
}
