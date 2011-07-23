/* db_compiler.c - a simple basic compiler
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include "db_config.h"
#include "db_compiler.h"
#include "db_vmdebug.h"

/* local function prototypes */
static void ApplyLocalFixups(ParseContext *c, VMUVALUE base);
static void DumpLocalFixups(ParseContext *c);
static void UpdateReferences(ParseContext *c);

/* InitCompiler - initialize the compiler */
ParseContext *InitCompiler(System *sys, BoardConfig *config, size_t codeBufSize)
{
    ParseContext *c;
    
    /* allocate a parse context */
    if (!(c = (ParseContext *)AllocateFreeSpace(sys, sizeof(ParseContext) + codeBufSize)))
        return NULL;
        
    /* initialize the new parse context */
    memset(c, 0, sizeof(ParseContext));
    c->stringType.id = TYPE_STRING;
    c->integerType.id = TYPE_INTEGER;
    c->integerArrayType.id = TYPE_ARRAY;
    c->integerArrayType.u.arrayInfo.elementType = &c->integerType;
    c->integerPointerType.id = TYPE_POINTER;
    c->integerPointerType.u.pointerInfo.targetType = &c->integerType;
    c->byteType.id = TYPE_BYTE;
    c->byteArrayType.id = TYPE_ARRAY;
    c->byteArrayType.u.arrayInfo.elementType = &c->byteType;
    c->bytePointerType.id = TYPE_POINTER;
    c->bytePointerType.u.pointerInfo.targetType = &c->byteType;
    c->codeBuf = (uint8_t *)c + sizeof(ParseContext);
    c->ctop = c->codeBuf + codeBufSize;
    c->sys = sys;
    c->config = config;

    /* setup the target sections */
    if (!(c->textTarget = GetSection(c->config, c->config->defaultTextSection))) {
        VM_printf("Unknown section: %s\n", c->config->defaultTextSection);
        return NULL;
    }
    if (!(c->dataTarget = GetSection(c->config, c->config->defaultDataSection))) {
        VM_printf("Unknown section: %s\n", c->config->defaultDataSection);
        return NULL;
    }
    
    /* return the new parse context */
    return c;
}

/* Compile - compile a program */
int Compile(ParseContext *c, char *name)
{
	/* setup an error target */
    if (setjmp(c->sys->errorTarget) != 0) {
        CloseParseContext(c);
        return VMFALSE;
    }
        
    /* start the image and initialize the interpreter stack size */
    if (!StartImage(c, name))
        return VMFALSE;
    c->stackSize = DEFAULT_STACK_SIZE;

    /* use the rest of the free space for the compiler heap */
    c->nextGlobal = AllocateAllFreeSpace(c->sys, &c->heapSize);
    c->heapTop = c->nextLocal = c->nextGlobal + c->heapSize;
    c->maxHeapUsed = 0;

    /* initialize block nesting stack */
    c->btop = (Block *)((char *)c->blockBuf + sizeof(c->blockBuf));
    c->bptr = c->blockBuf - 1;

    /* initialize the code staging buffer */
    c->cptr = c->codeBuf;
    
    /* no main function yet */
    c->mainState = MAIN_NOT_DEFINED;

    /* initialize the string and label tables */
    c->strings = NULL;

    /* initialize the global symbol table */
    InitSymbolTable(&c->globals);
    
    /* add some constants */
    AddGlobalConstantInteger(c, "TRUE", 1);
    AddGlobalConstantInteger(c, "FALSE", 0);

    /* add the registers */
    AddRegister(c, "PAR",   COG_BASE + 0x1f0 * 4);
    AddRegister(c, "CNT",   COG_BASE + 0x1f1 * 4);
    AddRegister(c, "INA",   COG_BASE + 0x1f2 * 4);
    AddRegister(c, "INB",   COG_BASE + 0x1f3 * 4);
    AddRegister(c, "OUTA",  COG_BASE + 0x1f4 * 4);
    AddRegister(c, "OUTB",  COG_BASE + 0x1f5 * 4);
    AddRegister(c, "DIRA",  COG_BASE + 0x1f6 * 4);
    AddRegister(c, "DIRB",  COG_BASE + 0x1f7 * 4);
    AddRegister(c, "CTRA",  COG_BASE + 0x1f8 * 4);
    AddRegister(c, "CTRB",  COG_BASE + 0x1f9 * 4);
    AddRegister(c, "FRQA",  COG_BASE + 0x1fa * 4);
    AddRegister(c, "FRQB",  COG_BASE + 0x1fb * 4);
    AddRegister(c, "PHSA",  COG_BASE + 0x1fc * 4);
    AddRegister(c, "PHSB",  COG_BASE + 0x1fd * 4);
    AddRegister(c, "VCFG",  COG_BASE + 0x1fe * 4);
    AddRegister(c, "VSCL",  COG_BASE + 0x1ff * 4);

    /* initialize scanner */
    c->inComment = VMFALSE;
    
    /* do two passes over the source program */
    for (c->pass = 1; c->pass <= 2; ++c->pass) {
        
        /* rewind to the start of the source program */
        RewindInput(c);
    
        /* get the next line */
        while (GetLine(c)) {
            Token tkn;
            if ((tkn = GetToken(c)) != T_EOL)
                ParseStatement(c, tkn);
        }
        
        /* clear the list of included files for the next pass */
        ClearIncludedFiles(c);
    }
    
    /* close the input file */
    CloseParseContext(c);

    /* write the main code */
    switch (c->mainState) {
    case MAIN_IN_PROGRESS:
		EndFunction(c);
        break;
    case MAIN_NOT_DEFINED:
        ParseError(c, "no main code");
        break;
    }
    
    /* update all global variable references */
    UpdateReferences(c);

    /* show the symbol and string tables */
    if (c->flags & COMPILER_DEBUG) {
        VM_putchar('\n');
        DumpSymbols(c, &c->globals, "symbols");
        if (c->strings) {
            int first = VMTRUE;
            String *str;
            for (str = c->strings; str != NULL; str = str->next)
                if (str->placed) {
                    if (first) {
                        VM_printf("\nstrings:\n");
                        first = VMFALSE;
                    }
                    VM_printf("  %08x %s\n", c->textTarget->base + str->offset, str->value);
                }
            VM_putchar('\n');
        }
    }

    /* build an image in memory */
    return BuildImage(c, name);
}

/* StartCode - start a function or method under construction */
void StartCode(ParseContext *c)
{
    /* initialize */
    c->symbolFixups = NULL;

    /* reset to compile the next code */
    c->cptr = c->codeBuf;
}

/* StoreCode - store the function or method under construction */
void StoreCode(ParseContext *c)
{
    int codeSize;

    /* check for unterminated blocks */
	switch (c->bptr->type) {
    case BLOCK_IF:
    case BLOCK_ELSE:
		ParseError(c, "expecting END IF");
    case BLOCK_FOR:
		ParseError(c, "expecting NEXT");
	case BLOCK_DO:
		ParseError(c, "expecting LOOP");
	default:
		break;
	}

    /* make sure all referenced labels were defined */
    CheckLabels(c);

    /* generate code for the function */
    if (c->flags & COMPILER_DEBUG) 
        PrintNode(c->function, 0);
    Generate(c, c->function);
    
    /* store the function or main offset */
    if (c->functionType)
        c->function->u.functionDefinition.symbol->v.variable.offset = c->textTarget->offset;
    else
        c->mainCode = c->textTarget->base + c->textTarget->offset;

    /* apply the local symbol and string fixups */
    ApplyLocalFixups(c, c->textTarget->offset);
    
    /* determine the code size */
    codeSize = c->cptr - c->codeBuf;

    /* show the function disassembly */
    if (c->flags & COMPILER_DEBUG) {
        Symbol *symbol = c->function->u.functionDefinition.symbol;
        VM_printf("\n%s:\n", symbol ? symbol->name : "[main]");
        DecodeFunction(c->textTarget->base + c->textTarget->offset, c->codeBuf, codeSize);
        if (c->functionType)
            DumpSymbols(c, &c->function->type->u.functionInfo.arguments, "arguments");
        DumpSymbols(c, &c->function->u.functionDefinition.locals, "locals");
        DumpLabels(c);
        DumpLocalFixups(c);
    }

    /* store the code */
    c->textTarget->offset += WriteSection(c, c->textTarget, c->codeBuf, codeSize);

    /* empty the local heap */
    c->nextLocal = c->heapTop;
}

/* AddString - add a string to the string table */
String *AddString(ParseContext *c, char *value)
{
    String *str;
    
    /* check to see if the string is already in the table */
    for (str = c->strings; str != NULL; str = str->next)
        if (strcmp(value, (char *)str->value) == 0)
            return str;

    /* allocate the string structure */
    str = (String *)GlobalAlloc(c, sizeof(String) + strlen(value));
    memset(str, 0, sizeof(String));
    strcpy((char *)str->value, value);
    str->next = c->strings;
    c->strings = str;

    /* return the string table entry */
    return str;
}

/* AddStringRef - add a reference to a string in the string table */
VMUVALUE AddStringRef(ParseContext *c, String *str)
{
    if (!str->placed) {
        str->offset = c->textTarget->offset;
        c->textTarget->offset += WriteSection(c, c->textTarget, str->value, strlen((char *)str->value) + 1);
        str->placed = VMTRUE;
    }
    return c->textTarget->base + str->offset;
}

/* AddLocalSymbolFixup - add a symbol entry to the local fixup list */
VMUVALUE AddLocalSymbolFixup(ParseContext *c, Symbol *symbol, VMUVALUE offset)
{
    LocalFixup **pFixups = &c->symbolFixups, *fixup;
    VMUVALUE next;
    
    /* look for an existing fixup */
    for (; (fixup = *pFixups) != NULL; pFixups = &fixup->next)
        if (symbol == fixup->symbol)
            break;
    
    /* add a new fixup if no existing one was found */
    if (!fixup) {
        fixup = LocalAlloc(c, sizeof(LocalFixup));
        fixup->symbol = symbol;
        fixup->chain = 0;
        fixup->next = 0;
        *pFixups = fixup;
    }
    
    /* link this new fixup into the chain */
    next = fixup->chain;
    fixup->chain = offset;
    
    /* return the offset to the next entry in the chain */
    return next;
}

/* ApplyLocalFixups - apply the local fixups for the current function */
static void ApplyLocalFixups(ParseContext *c, VMUVALUE base)
{
    LocalFixup *fixup;
    for (fixup = c->symbolFixups; fixup != NULL; fixup = fixup->next) {
        VMUVALUE offset, next;
        for (offset = fixup->chain; offset != 0; offset = next) {
            next = rd_cword(c, offset);
            wr_cword(c, offset, fixup->symbol->v.variable.fixups);
            fixup->symbol->v.variable.fixups = base + offset;
        }
    }
}

/* DumpLocalFixups - dump the local symbol and string fixups */
static void DumpLocalFixups(ParseContext *c)
{
    LocalFixup *fixup;
    if (c->symbolFixups) {
        printf("symbol fixups:\n");
        for (fixup = c->symbolFixups; fixup != NULL; fixup = fixup->next)
            printf("  %08x %s\n", fixup->chain, fixup->symbol->name);
    }
}

/* UpdateReferences - update all global symbol references */
static void UpdateReferences(ParseContext *c)
{
    Symbol *sym;

    for (sym = c->globals.head; sym != NULL; sym = sym->next) {
        VMUVALUE offset, next;
        if (sym->type->id != TYPE_STRING && (offset = sym->v.variable.fixups) != 0) {
            VMUVALUE addr;
            switch (sym->storageClass) {
            case SC_CONSTANT: // function text offset
            case SC_GLOBAL:
                addr = sym->section->base + sym->v.variable.offset;
                break;
            default:
                ParseError(c, "unexpected storage class");
                break;
            }
            for (; offset != 0; offset = next) {
                next = ReadSectionOffset(c, c->textTarget, offset);
                WriteSectionOffset(c, c->textTarget, offset, addr);
            }
        }
    }
}

/* AddRegister - add a register to the global symbol table */
void AddRegister(ParseContext *c, char *name, VMUVALUE addr)
{
    AddGlobalOffset(c, name, SC_COG, &c->integerType, addr);
}

/* GlobalAlloc - allocate memory from the global heap */
void *GlobalAlloc(ParseContext *c, size_t size)
{
    void *p;
    size = ROUND_TO_WORDS(size);
    if (c->nextGlobal + size > c->nextLocal)
        Fatal(c->sys, "insufficient memory");
    p = c->nextGlobal;
    c->nextGlobal += size;
    if (c->heapSize - (c->nextLocal - c->nextGlobal) > c->maxHeapUsed)
        c->maxHeapUsed = c->heapSize - (c->nextLocal - c->nextGlobal);
    return p;
}

/* LocalAlloc - allocate memory from the local heap */
void *LocalAlloc(ParseContext *c, size_t size)
{
    size = ROUND_TO_WORDS(size);
    if (c->nextLocal - size < c->nextGlobal)
        Fatal(c->sys, "insufficient memory");
    c->nextLocal -= size;
    if (c->heapSize - (c->nextLocal - c->nextGlobal) > c->maxHeapUsed)
        c->maxHeapUsed = c->heapSize - (c->nextLocal - c->nextGlobal);
    return c->nextLocal;
}
