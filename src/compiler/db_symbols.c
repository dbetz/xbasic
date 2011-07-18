/* db_symbols.c - symbol table routines
 *
 * Copyright (c) 2009 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db_compiler.h"

/* local functions */
static Symbol *AddGlobal(ParseContext *c, SymbolTable *table, const char *name, StorageClass storageClass, Type *type, VMUVALUE offset);

/* InitSymbolTable - initialize a symbol table */
void InitSymbolTable(SymbolTable *table)
{
    table->head = NULL;
    table->pTail = &table->head;
    table->count = 0;
}

/* AddGlobalSymbol - add a global symbol to the symbol table */
Symbol *AddGlobalSymbol(ParseContext *c, const char *name, StorageClass storageClass, Type *type, Section *section)
{
    Symbol *sym = AddGlobal(c, &c->globals, name, storageClass, type, section ? section->offset : UNDEF_VALUE);
    sym->section = section;
    return sym;
}

/* AddGlobalOffset - add a global symbol to the symbol table */
Symbol *AddGlobalOffset(ParseContext *c, const char *name, StorageClass storageClass, Type *type, VMUVALUE offset)
{
    return AddGlobal(c, &c->globals, name, storageClass, type, offset);
}

/* AddGlobalConstantInteger - add a global constant integer symbol to the symbol table */
Symbol *AddGlobalConstantInteger(ParseContext *c, const char *name, VMVALUE value)
{
    Symbol *sym = AddGlobal(c, &c->globals, name, SC_CONSTANT, &c->integerType, 0);
    sym->v.value = value;
    return sym;
}

/* AddGlobalConstantString - add a global constant string symbol to the symbol table */
Symbol *AddGlobalConstantString(ParseContext *c, const char *name, String *string)
{
    Symbol *sym = AddGlobal(c, &c->globals, name, SC_CONSTANT, &c->stringType, 0);
    sym->v.string = string;
    return sym;
}

/* AddFormalArgument - add a formal argument using the global heap */
Symbol *AddFormalArgument(ParseContext *c, const char *name, Type *type, VMUVALUE offset)
{
    if (type->id == TYPE_ARRAY)
        type = ArrayTypeToPointerType(c, type);
    return AddGlobal(c, &c->codeType->u.functionInfo.arguments, name, SC_LOCAL, type, offset);
}

/* AddGlobal - add a symbol to a global symbol table */
static Symbol *AddGlobal(ParseContext *c, SymbolTable *table, const char *name, StorageClass storageClass, Type *type, VMUVALUE offset)
{
    size_t size = sizeof(Symbol) + strlen(name);
    Symbol *sym;
    
    /* check for a duplicate symbol name */
    if ((sym = FindSymbol(table, name)) != NULL)
        ParseError(c, "duplicate symbol '%s'", name);
        
    /* allocate the symbol structure */
    sym = (Symbol *)GlobalAlloc(c, size);
    strcpy(sym->name, name);
    sym->storageClass = storageClass;
    sym->section = NULL;
    sym->type = type;
    sym->v.variable.offset = offset;
    sym->v.variable.fixups = 0;
    sym->next = NULL;

    /* add it to the symbol table */
    *table->pTail = sym;
    table->pTail = &sym->next;
    ++table->count;
    
    /* return the symbol */
    return sym;
}

/* AddLocal - add a symbol to the symbol table */
Symbol *AddLocal(ParseContext *c, const char *name, Type *type, VMUVALUE offset)
{
    SymbolTable *table = &c->locals;
    size_t size = sizeof(Symbol) + strlen(name);
    Symbol *sym;
    
    /* check for a duplicate symbol name */
    if ((sym = FindSymbol(table, name)) != NULL)
        ParseError(c, "duplicate symbol '%s'", name);
        
    /* allocate the symbol structure */
    sym = (Symbol *)LocalAlloc(c, size);
    strcpy(sym->name, name);
    sym->storageClass = SC_LOCAL;
    sym->section = NULL;
    sym->type = type;
    sym->v.variable.offset = offset;
    sym->next = NULL;

    /* add it to the symbol table */
    *table->pTail = sym;
    table->pTail = &sym->next;
    ++table->count;
    
    /* return the symbol */
    return sym;
}

/* FindSymbol - find a symbol in a symbol table */
Symbol *FindSymbol(SymbolTable *table, const char *name)
{
    Symbol *sym = table->head;
    while (sym) {
        if (strcasecmp(name, sym->name) == 0)
            return sym;
        sym = sym->next;
    }
    return NULL;
}

/* IsConstant - check to see if the value of a symbol is a constant */
int IsConstant(Symbol *symbol)
{
    return symbol->storageClass == SC_CONSTANT;
}

/* DumpSymbols - dump a symbol table */
void DumpSymbols(ParseContext *c, SymbolTable *table, char *tag)
{
    Symbol *sym;
    if ((sym = table->head) != NULL) {
        VM_printf("%s:\n", tag);
        for (; sym != NULL; sym = sym->next) {
            VMUVALUE value = sym->v.variable.offset;
            switch (sym->storageClass) {
            case SC_CONSTANT:
            case SC_GLOBAL:
                if (sym->section)
                    value += sym->section->base;
                break;
            default:
                // no offset
                break;
            }
            VM_printf("  %c %c %08x %08x %s\n", "CLTDHR"[sym->storageClass], "IBSAPF"[sym->type->id], value, sym->v.variable.fixups, sym->name);
        }
    }
}
