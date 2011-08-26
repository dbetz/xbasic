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
static void GenerateDependencies(ParseContext *c);
static void ApplyLocalFixups(ParseContext *c, VMUVALUE base);
static void DumpLocalFixups(ParseContext *c);
static void UpdateReferences(ParseContext *c);

/* InitCompiler - initialize the compiler */
ParseContext *InitCompiler(System *sys, BoardConfig *config, size_t codeBufSize)
{
    ParseContext *c;
    
    /* allocate a parse context */
    if (!(c = (ParseContext *)xbGlobalAlloc(sys, sizeof(ParseContext) + codeBufSize)))
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
        xbError(c->sys, "Unknown section: %s\n", c->config->defaultTextSection);
        return NULL;
    }
    if (!(c->dataTarget = GetSection(c->config, c->config->defaultDataSection))) {
        xbError(c->sys, "Unknown section: %s\n", c->config->defaultDataSection);
        return NULL;
    }
    
    /* return the new parse context */
    return c;
}

/* Compile - compile a program */
int Compile(ParseContext *c, const char *name)
{
    /* setup an error target */
    if (setjmp(c->errorTarget) != 0) {
        CloseParseContext(c);
        return FALSE;
    }
        
    /* start the image and initialize the interpreter stack size */
    if (!StartImage(c, name))
        return FALSE;
    c->stackSize = DEFAULT_STACK_SIZE;

    /* initialize block nesting stack */
    c->btop = (Block *)((char *)c->blockBuf + sizeof(c->blockBuf));
    c->bptr = c->blockBuf - 1;

    /* initialize the code staging buffer */
    c->cptr = c->codeBuf;
    
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
    c->inComment = FALSE;
    
    /* do three passes over the source program */
    for (c->pass = 1; c->pass <= 3; ++c->pass) {
        
        /* no main function yet */
        c->mainState = MAIN_NOT_DEFINED;

        /* rewind to the start of the source program */
        RewindInput(c);
    
        /* get the next line */
        while (GetLine(c)) {
            Token tkn;
            if ((tkn = GetToken(c)) != T_EOL)
                ParseStatement(c, tkn);
        }
        
        /* end the main function if it's in progress */
        if (c->pass > 1) {
            switch (c->mainState) {
            case MAIN_IN_PROGRESS:
                EndFunction(c);
                break;
            case MAIN_NOT_DEFINED:
                ParseError(c, "no main code");
                break;
            case MAIN_DEFINED:
                // nothing to do
                break;
            }
    
            /* make a list of dependencies at the end of the second pass */
            if (c->pass == 2)
                GenerateDependencies(c);
        }
        
        /* clear the list of included files for the next pass */
        ClearIncludedFiles(c);
    }
    
    /* close the input file */
    CloseParseContext(c);

    /* update all global variable references */
    UpdateReferences(c);

    /* show the symbol and string tables */
    if (c->flags & COMPILER_DEBUG) {
        xbInfo(c->sys, "\n");
        DumpSymbols(c, &c->globals, "symbols");
        if (c->strings) {
            int first = TRUE;
            String *str;
            for (str = c->strings; str != NULL; str = str->next)
                if (str->placed) {
                    if (first) {
                        xbInfo(c->sys, "\nstrings:\n");
                        first = FALSE;
                    }
                    xbInfo(c->sys, "  %08x %s\n", c->textTarget->base + str->offset, str->value);
                }
            xbInfo(c->sys, "\n");
        }
    }

    /* build an image in memory */
    return BuildImage(c, name);
}

/* GenerateDependencies - generate a list of dependencies of the main function */
static void GenerateDependencies(ParseContext *c)
{
    Dependency *dependencies, **pNext, *d, *d2, *d3, *next;
    
    /* initialize the main dependency list */
    dependencies = NULL;
    pNext = &dependencies;
    
    /* add all of the main dependencies */
    for (d = c->mainDependencies; d != NULL; d = next) {
        next = d->next;
        *pNext = d;
        pNext = &d->next;
        d->next = NULL;
    }
    
    /* add all of the recursive dependencies */
    for (d = dependencies; d != NULL; d = d->next) {
        Symbol *sym = d->symbol;
        if (sym->type->id == TYPE_FUNCTION) {
            for (d2 = sym->type->u.functionInfo.dependencies; d2 != NULL; d2 = next) {
                sym = d2->symbol;
                next = d2->next;
                for (d3 = dependencies; d3 != NULL; d3 = d3->next)
                    if (sym == d3->symbol)
                        break;
                if (!d3) {
                    *pNext = d2;
                    pNext = &d2->next;
                    d2->next = NULL;
                }
            }
        }
    }
    
    /* save the dependencies of the main function */
    c->mainDependencies = dependencies;

    if (c->flags & COMPILER_DEBUG) {
        if ((d = c->mainDependencies) != NULL) {
            xbInfo(c->sys, "main dependencies:\n");
            for (; d != NULL; d = d->next)
                xbInfo(c->sys, "  %s\n", d->symbol->name);
        }
    }
}

/* StoreCode - store the function or method under construction */
void StoreCode(ParseContext *c)
{
    int codeSize;

    /* initialize */
    c->symbolFixups = NULL;

    /* generate code for the function */
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
        xbInfo(c->sys, "\n%s:\n", symbol ? symbol->name : "[main]");
        DecodeFunction(c->sys, c->textTarget->base + c->textTarget->offset, c->codeBuf, codeSize);
        if (c->functionType)
            DumpSymbols(c, &c->function->type->u.functionInfo.arguments, "arguments");
        DumpSymbols(c, &c->function->u.functionDefinition.locals, "locals");
        DumpLabels(c);
        DumpLocalFixups(c);
    }
    
    /* store the code */
    c->textTarget->offset += WriteSection(c, c->textTarget, c->codeBuf, codeSize);

    /* reset to compile the next code */
    c->cptr = c->codeBuf;
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
        str->placed = TRUE;
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
        fixup = xbLocalAlloc(c->sys, sizeof(LocalFixup));
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
    if (!(p = xbGlobalAlloc(c->sys, size)))
        Fatal(c, "insufficient memory");
    return p;
}

/* LocalAlloc - allocate memory from the local heap */
void *LocalAlloc(ParseContext *c, size_t size)
{
    void *p;
    if (!(p = xbLocalAlloc(c->sys, size)))
        Fatal(c, "insufficient memory");
    return p;
}

/* Fatal - report a fatal error and exit */
void Fatal(ParseContext *c, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    xbErrorV(c->sys, fmt, ap);
    va_end(ap);
    longjmp(c->errorTarget, 1);
}

