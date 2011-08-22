#include "db_config.h"
#include "mem_linear.h"

typedef struct {
    System sys;
    uint8_t *nextGlobal;            /* next global heap space location */
    uint8_t *nextLocal;             /* next local heap space location */
    uint8_t *heapTop;               /* top of the heap */
    size_t heapSize;                /* size of heap space in bytes */
    size_t maxHeapUsed;             /* maximum amount of heap space allocated so far */
} MySystem;

/* MemInit - initialize the memory allocator */
System *MemInit(uint8_t *space, size_t size)
{
    MySystem *sys;
    
    /* make sure there is enough space for the system interface */
    if (size < sizeof(MySystem))
        return NULL;
        
    /* allocate the system interface structure */
    sys = (MySystem *)space;
    space += sizeof(MySystem);
    size -= sizeof(MySystem);
        
    /* use the rest of the free space for the compiler heap */
    sys->nextGlobal = space;
    sys->heapSize = size;
    sys->heapTop = sys->nextLocal = sys->nextGlobal + size;
    sys->maxHeapUsed = 0;
    
    /* return the system interface structure */
    return (System *)sys;
}

/* xbGlobalAlloc - allocate memory from the global heap */
void *xbGlobalAlloc(System *sysbase, size_t size)
{
    MySystem *sys = (MySystem *)sysbase;
    void *p;
    size = ROUND_TO_WORDS(size);
    if (sys->nextGlobal + size > sys->nextLocal)
        return NULL;
    p = sys->nextGlobal;
    sys->nextGlobal += size;
    if (sys->heapSize - (sys->nextLocal - sys->nextGlobal) > sys->maxHeapUsed)
        sys->maxHeapUsed = sys->heapSize - (sys->nextLocal - sys->nextGlobal);
    return p;
}

/* xbLocalAlloc - allocate memory from the local heap */
void *xbLocalAlloc(System *sysbase, size_t size)
{
    MySystem *sys = (MySystem *)sysbase;
    size = ROUND_TO_WORDS(size);
    if (sys->nextLocal - size < sys->nextGlobal)
        return NULL;
    sys->nextLocal -= size;
    if (sys->heapSize - (sys->nextLocal - sys->nextGlobal) > sys->maxHeapUsed)
        sys->maxHeapUsed = sys->heapSize - (sys->nextLocal - sys->nextGlobal);
    return sys->nextLocal;
}

/* xbLocalFreeAll - free all local memory */
void xbLocalFreeAll(System *sysbase)
{
    MySystem *sys = (MySystem *)sysbase;
    sys->nextLocal = sys->heapTop;
}
