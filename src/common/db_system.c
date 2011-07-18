#include <stdio.h>
#include <stdarg.h>
#include "db_system.h"

/* InitFreeSpace - initialize free space allocator */
void InitFreeSpace(System *sys, uint8_t *space, size_t size)
{
    sys->freeSpace = sys->freeMark = sys->freeNext = space;
    sys->freeTop = space + size;
}

/* MarkFreeSpace - initialize free space allocator */
void MarkFreeSpace(System *sys)
{
    sys->freeMark = sys->freeNext;
}

/* ResetFreeSpace - initialize free space allocator */
void ResetFreeSpace(System *sys)
{
    sys->freeNext = sys->freeMark;
}

/* AllocateFreeSpace - allocate free space */
uint8_t *AllocateFreeSpace(System *sys, size_t size)
{
    uint8_t *p = sys->freeNext;
    size = ROUND_TO_WORDS(size);
	if (p + size > sys->freeTop)
        return NULL;
    sys->freeNext += size;
    return p;
}

/* AllocateAllFreeSpace - allocate all of the remaining free space */
uint8_t *AllocateAllFreeSpace(System *sys, size_t *pSize)
{
    uint8_t *p = sys->freeNext;
    *pSize = sys->freeTop - p;
    sys->freeNext = sys->freeTop;
    return p;
}

/* VM_printf - formatted print */
void VM_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_vprintf(fmt, ap);
    va_end(ap);
}

/* VM_vprintf - formatted print with varargs list */
void VM_vprintf(const char *fmt, va_list ap)
{
    char buf[80], *p = buf;
    vsprintf(buf, fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
}

/* Fatal - report a fatal error and exit */
void Fatal(System *sys, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_printf("error: ");
    VM_vprintf(fmt, ap);
    VM_putchar('\n');
    va_end(ap);
    longjmp(sys->errorTarget, 1);
}
