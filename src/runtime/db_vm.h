/* db_vm.h - definitions for a simple virtual machine
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __DB_VM_H__
#define __DB_VM_H__

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include "db_vmimage.h"

/* forward type declarations */
typedef struct Interpreter Interpreter;

/* intrinsic function handler type */
typedef void IntrinsicFcn(Interpreter *i);

/* interpreter state structure */
struct Interpreter {
    ImageHdr *image;
    jmp_buf errorTarget;
    VMVALUE *stack;
    VMVALUE *stackTop;
    uint8_t *pc;
    VMVALUE *fp;
    VMVALUE *sp;
    VMVALUE tos;
    int argc;
    int linePos;
};

/* stack manipulation macros */
#define Reserve(i, n)   do {                                    \
                            if ((i)->sp - (n) < (i)->stack)     \
                                StackOverflow(i);               \
                            else  {                             \
                                int _cnt = (n);                 \
                                while (--_cnt >= 0)             \
                                    Push(i, 0);                 \
                            }                                   \
                        } while (0)
#define CPush(i, v)     do {                                    \
                            if ((i)->sp - 1 < (i)->stack)       \
                                StackOverflow(i);               \
                            else                                \
                                Push(i, v);                     \
                        } while (0)
#define Push(i, v)      (*--(i)->sp = (v))
#define Pop(i)          (*(i)->sp++)
#define Top(i)          (*(i)->sp)
#define Drop(i, n)      ((i)->sp += (n))

/* prototypes and variables from db_vmfcn.c */
extern IntrinsicFcn * FLASH_SPACE Intrinsics[];
extern int IntrinsicCount;

/* prototypes from db_vmint.c */
uint8_t *InitInterpreter(Interpreter *i, size_t stackSize);
int Execute(Interpreter *i, ImageHdr *image);
void Warn(const char *fmt, ...);                    /* fmt in FLASH_SPACE */
void Abort(Interpreter *i, const char *fmt, ...);   /* fmt in FLASH_SPACE */
void StackOverflow(Interpreter *i);
void ShowStack(Interpreter *i);

/* prototypes from db_vmimage.c */
ImageHdr *LoadImage(System *sys);

void VM_getline(char *buf, int size);
int ImageFileRead(uint8_t *buf, int size);

#endif
