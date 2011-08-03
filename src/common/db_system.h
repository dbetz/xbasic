#ifndef __DB_SYSTEM_H__
#define __DB_SYSTEM_H__

#include <stdarg.h>
#include <setjmp.h>
#include "db_config.h"

#define MAXLINE 128

typedef struct {
    jmp_buf errorTarget;    /* error target */
    uint8_t *freeSpace;     /* base of free space */
    uint8_t *freeMark;      /* mark the end of the permanently allocated space */
    uint8_t *freeNext;      /* next free space available */
    uint8_t *freeTop;       /* top of free space */
    char lineBuf[MAXLINE];  /* line buffer */
    char *linePtr;          /* pointer to the current character */
} System;

void InitFreeSpace(System *sys, uint8_t *space, size_t size);
void MarkFreeSpace(System *sys);
void ResetFreeSpace(System *sys);
uint8_t *AllocateFreeSpace(System *sys, size_t size);
uint8_t *AllocateAllFreeSpace(System *sys, size_t *pSize);
void VM_printf(const char *fmt, ...);
void VM_vprintf(const char *fmt, va_list ap);
void Fatal(System *sys, char *fmt, ...);

int VM_AddToPath(const char *p);
int VM_AddEnvironmentPath(void);
FILE *VM_fopen(const char *name, const char *mode);
FILE *VM_CreateTmpFile(const char *name, const char *mode);
void VM_RemoveTmpFile(const char *name);
int VM_getchar(void);
void VM_putchar(int ch);

#endif
