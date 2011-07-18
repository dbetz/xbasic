/* db_vmint.c - bytecode interpreter for a simple virtual machine
 *
 * Copyright (c) 2009 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "db_vm.h"
#include "db_vmdebug.h"

/* prototypes for local functions */
static uint8_t *MapAddress(Interpreter *i, VMUVALUE addr);
static VMUVALUE UnmapAddress(Interpreter *i, uint8_t *p);
static VMVALUE LoadValue(Interpreter *i, VMUVALUE addr);
static VMVALUE LoadByteValue(Interpreter *i, VMUVALUE addr);
static void StoreValue(Interpreter *i, VMUVALUE addr, VMVALUE value);
static void StoreByteValue(Interpreter *i, VMUVALUE addr, VMVALUE value);
static void DoTrap(Interpreter *i, int op);
static void PrintInteger(Interpreter *i, VMVALUE value);
static void PrintString(Interpreter *i, VMVALUE value);
static void PrintTab(Interpreter *i);
static void PrintC(Interpreter *i, int ch);

/* InitInterpreter - initialize the interpreter */
uint8_t *InitInterpreter(Interpreter *i, size_t stackSize)
{
    i->stack = (VMVALUE *)((uint8_t *)i + sizeof(Interpreter));
    i->stackTop = i->stack + stackSize;
    return (uint8_t *)i->stackTop;
}

/* Execute - execute the main code */
int Execute(Interpreter *i, ImageHdr *image)
{
    VMVALUE tmp;
    int8_t tmpb;
    int cnt;

	/* setup the new image */
	i->image = image;

    /* initialize */    
    i->pc = (uint8_t *)MapAddress(i, i->image->mainCode);
    i->sp = i->fp = i->stackTop;
    i->linePos = 0;

    if (setjmp(i->errorTarget))
        return VMFALSE;

    for (;;) {
#if 0
        ShowStack(i);
        DecodeInstruction(UnmapAddress(i, i->pc), i->pc);
#endif
        switch (VMCODEBYTE(i->pc++)) {
        case OP_HALT:
            return VMTRUE;
        case OP_BRT:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            if (i->tos)
                i->pc += tmp;
            i->tos = Pop(i);
            break;
        case OP_BRTSC:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            if (i->tos)
                i->pc += tmp;
            else
                i->tos = Pop(i);
            break;
        case OP_BRF:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            if (!i->tos)
                i->pc += tmp;
            i->tos = Pop(i);
            break;
        case OP_BRFSC:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            if (!i->tos)
                i->pc += tmp;
            else
                i->tos = Pop(i);
            break;
        case OP_BR:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            i->pc += tmp;
            break;
        case OP_NOT:
            i->tos = (i->tos ? VMFALSE : VMTRUE);
            break;
        case OP_NEG:
            i->tos = -i->tos;
            break;
        case OP_ADD:
            tmp = Pop(i);
            i->tos = tmp + i->tos;
            break;
        case OP_SUB:
            tmp = Pop(i);
            i->tos = tmp - i->tos;
            break;
        case OP_MUL:
            tmp = Pop(i);
            i->tos = tmp * i->tos;
            break;
        case OP_DIV:
            tmp = Pop(i);
            i->tos = (i->tos == 0 ? 0 : tmp / i->tos);
            break;
        case OP_REM:
            tmp = Pop(i);
            i->tos = (i->tos == 0 ? 0 : tmp % i->tos);
            break;
        case OP_BNOT:
            i->tos = ~i->tos;
            break;
        case OP_BAND:
            tmp = Pop(i);
            i->tos = tmp & i->tos;
            break;
        case OP_BOR:
            tmp = Pop(i);
            i->tos = tmp | i->tos;
            break;
        case OP_BXOR:
            tmp = Pop(i);
            i->tos = tmp ^ i->tos;
            break;
        case OP_SHL:
            tmp = Pop(i);
            i->tos = tmp << i->tos;
            break;
        case OP_SHR:
            tmp = Pop(i);
            i->tos = tmp >> i->tos;
            break;
        case OP_LT:
            tmp = Pop(i);
            i->tos = (tmp < i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_LE:
            tmp = Pop(i);
            i->tos = (tmp <= i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_EQ:
            tmp = Pop(i);
            i->tos = (tmp == i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_NE:
            tmp = Pop(i);
            i->tos = (tmp != i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_GE:
            tmp = Pop(i);
            i->tos = (tmp >= i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_GT:
            tmp = Pop(i);
            i->tos = (tmp > i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_LIT:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            CPush(i, i->tos);
            i->tos = tmp;
            break;
        case OP_SLIT:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            CPush(i, i->tos);
            i->tos = tmpb;
            break;
        case OP_LOAD:
            i->tos = LoadValue(i, (VMUVALUE)i->tos);
            break;
        case OP_LOADB:
            i->tos = LoadByteValue(i, (VMUVALUE)i->tos);
            break;
        case OP_STORE:
            tmp = Pop(i);
            StoreValue(i, (VMUVALUE)i->tos, tmp);
            i->tos = Pop(i);
            break;
        case OP_STOREB:
            tmp = Pop(i);
            StoreByteValue(i, (VMUVALUE)i->tos, tmp);
            i->tos = Pop(i);
            break;
        case OP_LREF:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            CPush(i, i->tos);
            i->tos = i->fp[(int)tmpb];
            break;
        case OP_LSET:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            i->fp[(int)tmpb] = i->tos;
            i->tos = Pop(i);
            break;
        case OP_INDEX:
            tmp = Pop(i);
            i->tos = tmp + i->tos * sizeof (VMVALUE);
            break;
        case OP_PUSHJ:
            tmp = (VMVALUE)(i->pc - (uint8_t *)i->image);
            i->pc = (uint8_t *)MapAddress(i, i->tos);
            i->tos = tmp;
            break;
        case OP_POPJ:
            i->pc = (uint8_t *)i->image + i->tos;
            i->tos = Pop(i);
            break;
        case OP_CLEAN:
            cnt = VMCODEBYTE(i->pc++);
            Drop(i, cnt);
            break;
        case OP_FRAME:
            cnt = VMCODEBYTE(i->pc++);
            tmp = (VMVALUE)(i->fp - i->stack);
            i->fp = i->sp;
            Reserve(i, cnt);
            i->fp[F_FP] = tmp;
            break;
        case OP_RETURNZ:
            CPush(i, i->tos);
            i->tos = 0;
            // fall through
        case OP_RETURN:
            i->pc = (uint8_t *)i->image + Top(i);
            i->sp = i->fp;
            i->fp = (VMVALUE *)(i->stack + i->fp[F_FP]);
            break;
        case OP_DROP:
            i->tos = Pop(i);
            break;
        case OP_DUP:
            CPush(i, i->tos);
            break;
        case OP_NATIVE:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            break;
        case OP_TRAP:
            DoTrap(i, VMCODEBYTE(i->pc++));
            break;
        default:
            Abort(i, "undefined opcode 0x%02x", VMCODEBYTE(i->pc - 1));
            break;
        }
    }
}

static uint8_t *MapAddress(Interpreter *i, VMUVALUE addr)
{
    int j;
    for (j = 0; j < i->image->sectionCount; ++j) {
        ImageSection *section = &i->image->sections[j];
        VMUVALUE base = section->fileSection->base;
        if (addr >= base && addr < base + 0x10000000) {
            if (addr > base + section->fileSection->size)
                Abort(i, "address error");
            return (uint8_t *)(section->data + (addr - base));
        }
    }
    Abort(i, "address error");
    return NULL; // not reached
}

static VMUVALUE UnmapAddress(Interpreter *i, uint8_t *p)
{
    ImageSection *section = i->image->sections;
    int cnt = i->image->sectionCount;
    while (--cnt >= 0) {
        uint8_t *start = section->data;
        if (p >= start && p < start + 0x10000000) {
            if (p > start + section->fileSection->size)
                Abort(i, "address error");
            return section->fileSection->base + (p - start);
        }
        ++section;
    }
    Abort(i, "address error");
    return 0; // not reached
}

static VMVALUE LoadValue(Interpreter *i, VMUVALUE addr)
{
    VMVALUE *p = (VMVALUE *)MapAddress(i, addr);
    return *p;
}

static VMVALUE LoadByteValue(Interpreter *i, VMUVALUE addr)
{
    uint8_t *p = MapAddress(i, addr);
    return *p;
}

static void StoreValue(Interpreter *i, VMUVALUE addr, VMVALUE value)
{
    VMVALUE *p = (VMVALUE *)MapAddress(i, addr);
    *p = value;
}

static void StoreByteValue(Interpreter *i, VMUVALUE addr, VMVALUE value)
{
    uint8_t *p = MapAddress(i, addr);
    *p = value;
}

static void DoTrap(Interpreter *i, int op)
{
    switch (op) {
    case TRAP_GETCHAR:
        Push(i, i->tos);
        i->tos = VM_getchar();
        break;
    case TRAP_PUTCHAR:
        PrintC(i, i->tos);
        i->tos = Pop(i);
        break;
    default:
        Abort(i, "undefined print opcode 0x%02x", op);
        break;
    }
}

static void PrintC(Interpreter *i, int ch)
{
    VM_putchar(ch);
    if (ch == '\n')
        i->linePos = 0;
    else
        ++i->linePos;
}

void ShowStack(Interpreter *i)
{
    VMVALUE *p;
    if (i->sp < i->stackTop) {
        VM_printf(" %d", i->tos);
        for (p = i->sp; p < i->stackTop - 1; ++p) {
            if (p == i->fp)
                VM_printf(" <fp>");
            VM_printf(" %d", *p);
        }
        VM_printf("\n");
    }
}

void StackOverflow(Interpreter *i)
{
    Abort(i, "stack overflow");
}

void Warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_printf("warning: ");
    VM_vprintf(fmt, ap);
    VM_putchar('\n');
    va_end(ap);
}

void Abort(Interpreter *i, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_printf("abort: ");
    VM_vprintf(fmt, ap);
    VM_putchar('\n');
    va_end(ap);
    if (i)
        longjmp(i->errorTarget, 1);
    else
        exit(1);
}
