/* db_statement.c - statement parser
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "db_compiler.h"
#include "db_vmdebug.h"

/* statement handler prototypes */
static void ParseInclude(ParseContext *c);
static void ParseOption(ParseContext *c);
static void SetIntegerOption(ParseContext *c, int *pValue);
static void ParseDef(ParseContext *c);
static void ParseConstantDef(ParseContext *c, char *name);
static void ParseFunctionDef(ParseContext *c, char *name);
static void ParseEndDef(ParseContext *c);
static void ParseDim(ParseContext *c);
static Type *ParseVariableDecl(ParseContext *c, char *name, VMUVALUE *pSize);
static VMVALUE ParseScalarInitializer(ParseContext *c);
static VMUVALUE ParseArrayInitializers(ParseContext *c, Type *type, VMUVALUE size);
static void ClearArrayInitializers(ParseContext *c, VMVALUE size);
static void ParseImpliedLetOrFunctionCall(ParseContext *c);
static void ParseLet(ParseContext *c);
static void ParseIf(ParseContext *c);
static void ParseElseIf(ParseContext *c);
static void ParseElse(ParseContext *c);
static void ParseEndIf(ParseContext *c);
static void ParseSelect(ParseContext *c);
static void ParseCase(ParseContext *c);
static void ParseEndSelect(ParseContext *c);
static void ParseEnd(ParseContext *c);
static void ParseFor(ParseContext *c);
static void ParseNext(ParseContext *c);
static void ParseDo(ParseContext *c);
static void ParseDoWhile(ParseContext *c);
static void ParseDoUntil(ParseContext *c);
static void ParseLoop(ParseContext *c);
static void ParseLoopWhile(ParseContext *c);
static void ParseLoopUntil(ParseContext *c);
static void ParseAsm(ParseContext *c);
static void ParseStop(ParseContext *c);
static void ParseGoto(ParseContext *c);
static void ParseReturn(ParseContext *c);
static void ParseInput(ParseContext *c);
static void ParsePrint(ParseContext *c);

/* prototypes */
static void PrintFunctionCall(ParseContext *c, char *name, ParseTreeNode *devExpr, ParseTreeNode *expr);
static void DefineLabel(ParseContext *c, char *name, int offset);
static int ReferenceLabel(ParseContext *c, char *name, int offset);
static void PushBlock(ParseContext *c);
static void PopBlock(ParseContext *c);
static void Assemble(ParseContext *c, char *opname);
static VMVALUE ParseIntegerConstant(ParseContext *c);

/*
    pass 1: handle definitions
    pass 2: compile code
*/

/* ParseStatement - parse a statement */
void ParseStatement(ParseContext *c, Token tkn)
{
again:
    /* dispatch on the statement keyword */
    switch (tkn) {
    case T_REM:
        /* just a comment so ignore the rest of the line */
        break;
    case T_INCLUDE:
        ParseInclude(c);
        break;
    case T_OPTION:
        ParseOption(c);
        break;
    case T_DEF:
        ParseDef(c);
        break;
    case T_END_DEF:
        ParseEndDef(c);
        break;
    case T_DIM:
        ParseDim(c);
        break;
    default:
        if (c->pass == 2) {
            if (!c->codeType) {
                switch (c->mainState) {
                case MAIN_NOT_DEFINED:
                    StartCode(c, NULL, NULL);
                    c->mainState = MAIN_IN_PROGRESS;
                    break;
                case MAIN_DEFINED:
                    ParseError(c, "all main code must be in the same place");
                    break;
                }
            }
            switch (tkn) {
            case T_LET:
                ParseLet(c);
                break;
            case T_IF:
                ParseIf(c);
                break;
            case T_ELSE_IF:
                ParseElseIf(c);
                break;
            case T_ELSE:
                ParseElse(c);
                break;
            case T_END_IF:
                ParseEndIf(c);
                break;
            case T_SELECT:
                ParseSelect(c);
                break;
            case T_CASE:
                ParseCase(c);
                break;
            case T_END_SELECT:
                ParseEndSelect(c);
                break;
            case T_END:
                ParseEnd(c);
                break;
            case T_FOR:
                ParseFor(c);
                break;
            case T_NEXT:
                ParseNext(c);
                break;
            case T_DO:
                ParseDo(c);
                break;
            case T_DO_WHILE:
                ParseDoWhile(c);
                break;
            case T_DO_UNTIL:
                ParseDoUntil(c);
                break;
            case T_LOOP:
                ParseLoop(c);
                break;
            case T_LOOP_WHILE:
                ParseLoopWhile(c);
                break;
            case T_LOOP_UNTIL:
                ParseLoopUntil(c);
                break;
            case T_ASM:
                ParseAsm(c);
                break;
            case T_STOP:
                ParseStop(c);
                break;
            case T_GOTO:
                ParseGoto(c);
                break;
            case T_RETURN:
                ParseReturn(c);
                break;
            case T_INPUT:
                ParseInput(c);
                break;
            case T_PRINT:
                ParsePrint(c);
                break;
            case T_IDENTIFIER:
                if (SkipSpaces(c) == ':') {
                    DefineLabel(c, c->token, codeaddr(c));
                    if ((tkn = GetToken(c)) == T_EOL)
                        return;
                    goto again;
                }
                UngetC(c);
                /* fall through */
            default:
                SaveToken(c, tkn);
                ParseImpliedLetOrFunctionCall(c);
                break;
            }
        }
        break;
    }
}

/* ParseInclude - parse the 'INCLUDE' statement */
static void ParseInclude(ParseContext *c)
{
    char name[MAXTOKEN];
    FRequire(c, T_STRING);
    strcpy(name, c->token);
    FRequire(c, T_EOL);
    if (!PushFile(c, name) && !PushFile(c, name))
        ParseError(c, "include file not found: %s", name);
}

/* ParseOption - parse the 'OPTION' statement */
static void ParseOption(ParseContext *c)
{
    FRequire(c, T_IDENTIFIER);
    FRequire(c, '=');

    /* handle the 'target' option */
    if (strcasecmp(c->token, "stacksize") == 0)
        SetIntegerOption(c, &c->stackSize);

    /* unknown option */
    else
        ParseError(c, "unknown option: %s", c->token);
        
    /* check for the end of the statement */
    FRequire(c, T_EOL);
}

/* SetIntegerOption - set an integer option */
static void SetIntegerOption(ParseContext *c, int *pValue)
{
    FRequire(c, T_NUMBER);
    *pValue = c->value;
}

/* ParseDef - parse the 'DEF' statement */
static void ParseDef(ParseContext *c)
{
    char name[MAXTOKEN];
    Token tkn;

    /* get the name being defined */
    FRequire(c, T_IDENTIFIER);
    strcpy(name, c->token);

    /* check for a constant definition */
    if ((tkn = GetToken(c)) == '=')
        ParseConstantDef(c, name);
    
    /* otherwise, assume a function definition */
    else {
        SaveToken(c, tkn);
        ParseFunctionDef(c, name);
    }
}

/* ParseConstantDef - parse a 'DEF <name> =' statement */
static void ParseConstantDef(ParseContext *c, char *name)
{
    if (c->pass == 1) {
        ParseTreeNode *expr;

        /* get the constant value */
        expr = ParseExpr(c);

        /* make sure it's a constant */
        if (IsIntegerLit(expr))
            AddGlobalConstantInteger(c, name, expr->u.integerLit.value);
        else if (IsStringLit(expr))
            AddGlobalConstantString(c, name, expr->u.stringLit.string);
        else
            ParseError(c, "expecting a constant expression");

        FRequire(c, T_EOL);
    }
}

/* ParseFunctionDef - parse a 'DEF <name>' statement */
static void ParseFunctionDef(ParseContext *c, char *name)
{
    Symbol *sym;

    /* end the main function if it is in progress */
    if (c->mainState == MAIN_IN_PROGRESS) {
        StoreMain(c);
        c->mainState = MAIN_DEFINED;
    }

    /* don't allow nested functions or subroutines (for now anyway) */
    if (c->codeType)
        ParseError(c, "nested subroutines and functions are not supported");

    /* handle pass 1 */
    if (c->pass == 1) {
        Type *type;
        Token tkn;
        
        /* make the function type */
        type = NewGlobalType(c, TYPE_FUNCTION);
        type->u.functionInfo.returnType = &c->integerType;
        InitSymbolTable(&type->u.functionInfo.arguments);
        
        /* set codeType so we know we're compiling a function */
        c->codeType = type;
        
        /* enter the function name in the global symbol table */
        sym = AddGlobalSymbol(c, name, SC_CONSTANT, type, NULL);

        /* get the argument list */
        if ((tkn = GetToken(c)) == '(') {
            if ((tkn = GetToken(c)) != ')') {
                int offset = 0;
                SaveToken(c, tkn);
                do {
                    char name[MAXTOKEN];
                    VMUVALUE size;
                    Type *type = ParseVariableDecl(c, name, &size);
                    if (type->id == TYPE_ARRAY && size != 0)
                        ParseError(c, "Array arguments can not specify size");
                    AddFormalArgument(c, name, type, offset++);
                } while ((tkn = GetToken(c)) == ',');
            }
            Require(c, tkn, ')');
        }
        else
            SaveToken(c, tkn);
        
        FRequire(c, T_EOL);
    }
    
    /* handle pass 2 */
    else {
    
        /* find the symbol defined in pass 1 */
        sym = FindSymbol(&c->globals, name);
        
        /* setup to compile the function body */
        sym->section = c->textTarget;
        StartCode(c, sym, sym->type);
    }
}

/* ParseEndDef - parse the 'END DEF' statement */
static void ParseEndDef(ParseContext *c)
{
    if (c->codeType) {
    
        /* handle pass 1 */
        if (c->pass == 1)
            c->codeType = NULL;
        
        /* handle pass 2 */
        else {
            c->codeSymbol->v.variable.offset = c->textTarget->offset;
            StoreCode(c);
        }
    }
    
    else
        ParseError(c, "not in a function definition");
}

/* ParseDim - parse the 'DIM' statement */
static void ParseDim(ParseContext *c)
{
    char name[MAXTOKEN];
    VMVALUE value = 0;
    VMUVALUE size;
    int isArray;
    Token tkn;

    /* parse variable declarations */
    do {
        Type *type;

        /* get variable name */
        type = ParseVariableDecl(c, name, &size);
        isArray = (type->id == TYPE_ARRAY);

        /* check for being inside a function definition */
        if (c->codeType) {
            ParseTreeNode *expr;
        
            /* local arrays are not yet supported */
            if (isArray)
                ParseError(c, "local arrays are not supported");
            
            /* only integer locals are currently supported */
            if (type != &c->integerType)
                ParseError(c, "only integer locals are currently supported");
                
            /* check for an initializer */
            if ((tkn = GetToken(c)) == '=')
                expr = ParseExpr(c);
        
            /* no initializers */
            else {
                SaveToken(c, tkn);
                expr = NULL;
            }
                
            if (c->pass == 2) {
            
                /* add the local symbol */
                AddLocal(c, name, type, -F_SIZE - c->localOffset - 1);
                c->localOffset += ValueSize(type, 0);
                
                /* compile the initialization code */
                if (expr) {
                    ParseTreeNode *var = GetSymbolRef(c, name);
                    PVAL pv;
                    code_lvalue(c, var, &pv);
                    code_rvalue(c, expr);
                    (*pv.fcn)(c, PV_STORE, &pv);
                }
            }
        }

        /* add to the global symbol table if outside a function definition */
        else {
            Section *target;
        
            /* check for target section */
            if ((tkn = GetToken(c)) == T_IN) {
                FRequire(c, T_STRING);
                if (strcasecmp(c->token, "text") == 0)
                    target = c->textTarget;
                else if (strcasecmp(c->token, "data") == 0)
                    target = c->dataTarget;
                else if (!(target = GetSection(c->config, c->token)))
                    ParseError(c, "no section '%s'", c->token);
            }
            
            /* use default data target */
            else {
                SaveToken(c, tkn);
                target = c->dataTarget;
            }
            
            /* check for initializers */
            if ((tkn = GetToken(c)) == '=') {
                if (isArray)
                    size = ParseArrayInitializers(c, type->u.arrayInfo.elementType, size);
                else
                    value = ParseScalarInitializer(c);
            }
            
            /* no initializers */
            else {
                SaveToken(c, tkn);
                if (isArray) {
                    if (size == 0)
                        ParseError(c, "no array size specified and no initializers");
                    ClearArrayInitializers(c, ValueSize(type, size));
                }
            }

            /* add the symbol on pass 1 */
            if (c->pass == 1) {
            
                /* handle arrays */
                if (isArray) {
                    AddGlobalSymbol(c, name, SC_CONSTANT, type, target);
                    target->offset += WriteSection(c, target, c->cptr, ValueSize(type, size) * sizeof(VMVALUE));
                }
                
                /* handle scalars */
                else {
                    AddGlobalSymbol(c, name, SC_GLOBAL, type, target);
                    target->offset += WriteSection(c, target, (uint8_t *)&value, sizeof(VMVALUE));
                }
            }                
        }
    } while ((tkn = GetToken(c)) == ',');

    Require(c, tkn, T_EOL);
}

/* ParseVariableDecl - parse a variable declaration */
static Type *ParseVariableDecl(ParseContext *c, char *name, VMUVALUE *pSize)
{
    Type *type = &c->integerType;
    int isArray = VMFALSE;
    Token tkn;

    /* parse the variable name */
    FRequire(c, T_IDENTIFIER);
    strcpy(name, c->token);
    
    /* handle arrays */
    if ((tkn = GetToken(c)) == '(') {
        isArray = VMTRUE;

        /* check for an array with unspecified size */
        if ((tkn = GetToken(c)) == ')')
            *pSize = 0;

        /* otherwise, parse the array size */
        else {
            ParseTreeNode *expr;

            /* put back the token */
            SaveToken(c, tkn);

            /* get the array size */
            expr = ParseExpr(c);

            /* make sure it's a constant */
            if (!IsIntegerLit(expr) || expr->u.integerLit.value <= 0)
                ParseError(c, "expecting a positive constant expression");
            *pSize = (VMUVALUE)expr->u.integerLit.value;

            /* only one dimension allowed for now */
            FRequire(c, ')');
        }
        
        /* get the next token */
        tkn = GetToken(c);
    }

    /* check for a type specification */
    if (tkn == T_AS) {
        FRequire(c, T_IDENTIFIER);
        if (strcasecmp(c->token, "INTEGER") == 0)
            type = &c->integerType;
        else if (strcasecmp(c->token, "BYTE") == 0)
            type = &c->byteType;
        else
            ParseError(c, "unknown type: %s", c->token);
    }

    /* just use the default type */
    else
        SaveToken(c, tkn);
    
    /* handle array types */
    if (isArray) {
        switch (type->id) {
        case TYPE_INTEGER:
            type = &c->integerArrayType;
            break;
        case TYPE_BYTE:
            type = &c->byteArrayType;
            break;
        default:
            ParseError(c, "unknown type: %d", type->id);
            break;                
        }
    }
    
    /* return the variable type */
    return type;
}

/* ParseScalarInitializer - parse a scalar initializer */
static VMVALUE ParseScalarInitializer(ParseContext *c)
{
    ParseTreeNode *expr = ParseExpr(c);
    VMVALUE value;

    if (IsIntegerLit(expr))
        value = expr->u.integerLit.value;
    else {
        ParseError(c, "expecting a constant expression");
        value = 0; // never reached
    }
    
    /* reset the local heap and return the constant value */
    c->nextLocal = c->heapTop;
    return value;
}
    
/* ParseArrayInitializers - parse array initializers */
static VMUVALUE ParseArrayInitializers(ParseContext *c, Type *type, VMUVALUE size)
{
    VMVALUE *wp = (VMVALUE *)c->cptr;
    uint8_t *bp = (uint8_t *)c->cptr;
    VMUVALUE remaining = size;
    VMUVALUE count = 0;
    Token tkn;

    /* handle a bracketed list of initializers */
    if ((tkn = GetToken(c)) == '{') {
        VMVALUE initializer;
        int done = VMFALSE;
        
        /* handle each line of initializers */
        while (!done) {
            int lineDone = VMFALSE;

            /* look for the first non-blank line */
            while ((tkn = GetToken(c)) == T_EOL) {
                if (!GetLine(c))
                    ParseError(c, "unexpected end of file in initializers");
            }

            /* check for the end of the initializers */
            if (tkn == '}')
                break;
            SaveToken(c, tkn);

            /* handle each initializer in the current line */
            while (!lineDone) {

                /* check for too many initializers */
                if (size > 0 && remaining == 0)
                    ParseError(c, "too many initializers");
                --remaining;
        
                /* get the initializer */
                initializer = ParseScalarInitializer(c);
        
                /* store the initial value */
                switch (type->id) {
                case TYPE_INTEGER:
                    *wp++ = initializer;
                    if (wp >= (VMVALUE *)c->ctop)
                        ParseError(c, "insufficient data space");
                    break;
                case TYPE_BYTE:
                    *bp++ = initializer;
                    if (bp >= (uint8_t *)c->ctop)
                        ParseError(c, "insufficient data space");
                    break;
                default:
                    break;
                }
                ++count;
                    
                switch (tkn = GetToken(c)) {
                case T_EOL:
                    lineDone = VMTRUE;
                    break;
                case '}':
                    lineDone = VMTRUE;
                    done = VMTRUE;
                    break;
                case ',':
                    break;
                default:
                    ParseError(c, "expecting a comma, right brace or end of line");
                    break;
                }
            }
        }
    }
    
    /* check for a string as the initializer */
    else {
        ParseTreeNode *expr;
        uint8_t *p;
        
        /* get the initializer */
        SaveToken(c, tkn);
        expr = ParseExpr(c);
        
        /* make sure it's a string literal */
        if (expr->nodeType != NodeTypeStringLit)
            ParseError(c, "expecting a string initializer");
        
        /* insert each character in the string */
        for (p = expr->u.stringLit.string->value; *p != '\0'; ) {
            
            /* check for too many initializers */
            if (size > 0 && remaining == 0)
                ParseError(c, "too many initializers");
            --remaining;
            
            /* store the initial value */
            switch (type->id) {
            case TYPE_INTEGER:
                *wp++ = *p++;
                if (wp >= (VMVALUE *)c->ctop)
                    ParseError(c, "insufficient data space");
                break;
            case TYPE_BYTE:
                *bp++ = *p++;
                if (bp >= (uint8_t *)c->ctop)
                    ParseError(c, "insufficient data space");
                break;
            default:
                break;
            }
            ++count;
        }
    }
    
    /* fill the remaining entries with zero */
    if (size > 0) {
        while (remaining > 0) {
            switch (type->id) {
            case TYPE_INTEGER:
                *wp++ = 0;
                if (wp >= (VMVALUE *)c->ctop)
                    ParseError(c, "insufficient data space");
                break;
            case TYPE_BYTE:
                *bp++ = 0;
                if (bp >= (uint8_t *)c->ctop)
                    ParseError(c, "insufficient data space");
                break;
            default:
                break;
            }
            --remaining;
        }
    }
    
    /* return the actual number of elements */
    return size > 0 ? size : count;
}

/* ClearArrayInitializers - clear the array initializers */
static void ClearArrayInitializers(ParseContext *c, VMVALUE size)
{
    VMVALUE *dataPtr = (VMVALUE *)c->cptr;
    VMVALUE *dataTop = (VMVALUE *)c->ctop;
    if (dataPtr + size > dataTop)
        ParseError(c, "insufficient object initializer space");
    memset(dataPtr, 0, size * sizeof(VMVALUE));
}

/* TypesCompatible - determine if two types are compatible */
static int TypesCompatible(Type *type1, Type *type2)
{
    switch (type1->id) {
    case TYPE_INTEGER:
    case TYPE_BYTE:
        return type2->id == TYPE_INTEGER || type2->id == TYPE_BYTE;
    default:
        return type1->id == type2->id;
    }
}

/* ParseImpliedLetOrFunctionCall - parse an implied let statement or a function call */
static void ParseImpliedLetOrFunctionCall(ParseContext *c)
{
    ParseTreeNode *expr;
    Type *type;
    Token tkn;
    PVAL pv;
    expr = ParsePrimary(c);
    switch (tkn = GetToken(c)) {
    case '=':
        type = ParseRValue(c);
        if (!TypesCompatible(expr->type, type))
            ParseError(c, "type mismatch");
        code_lvalue(c, expr, &pv);
        (*pv.fcn)(c, PV_STORE, &pv);
        break;
    default:
        SaveToken(c, tkn);
        code_rvalue(c, expr);
        putcbyte(c, OP_DROP);
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseLet - parse the 'LET' statement */
static void ParseLet(ParseContext *c)
{
    ParseTreeNode *lvalue;
    PVAL pv;
    lvalue = ParsePrimary(c);
    FRequire(c, '=');
    ParseRValue(c);
    code_lvalue(c, lvalue, &pv);
    if (lvalue->type->id != pv.type->id)
        ParseError(c, "type mismatch");
    (*pv.fcn)(c, PV_STORE, &pv);
    FRequire(c, T_EOL);
}

/* ParseIf - parse the 'IF' statement */
static void ParseIf(ParseContext *c)
{
    Token tkn;
    ParseRValue(c);
    FRequire(c, T_THEN);
    PushBlock(c);
    c->bptr->type = BLOCK_IF;
    putcbyte(c, OP_BRF);
    c->bptr->u.ifBlock.nxt = putcword(c, 0);
    c->bptr->u.ifBlock.end = 0;
    if ((tkn = GetToken(c)) != T_EOL) {
        ParseStatement(c, tkn);
        fixupbranch(c, c->bptr->u.ifBlock.nxt, codeaddr(c));
        PopBlock(c);
    }
    else
        Require(c, tkn, T_EOL);
}

/* ParseElseIf - parse the 'ELSE IF' statement */
static void ParseElseIf(ParseContext *c)
{
    switch (CurrentBlockType(c)) {
    case BLOCK_IF:
        putcbyte(c, OP_BR);
        c->bptr->u.ifBlock.end = putcword(c, c->bptr->u.ifBlock.end);
        fixupbranch(c, c->bptr->u.ifBlock.nxt, codeaddr(c));
        c->bptr->u.ifBlock.nxt = 0;
        ParseRValue(c);
        FRequire(c, T_THEN);
        putcbyte(c, OP_BRF);
        c->bptr->u.ifBlock.nxt = putcword(c, 0);
        FRequire(c, T_EOL);
        break;
    default:
        ParseError(c, "ELSE IF without a matching IF");
        break;
    }
}

/* ParseElse - parse the 'ELSE' statement */
static void ParseElse(ParseContext *c)
{
    int end;
    switch (CurrentBlockType(c)) {
    case BLOCK_IF:
        putcbyte(c, OP_BR);
        end = putcword(c, c->bptr->u.ifBlock.end);
        fixupbranch(c, c->bptr->u.ifBlock.nxt, codeaddr(c));
        c->bptr->type = BLOCK_ELSE;
        c->bptr->u.elseBlock.end = end;
        FRequire(c, T_EOL);
        break;
    default:
        ParseError(c, "ELSE without a matching IF");
        break;
    }
}

/* ParseEndIf - parse the 'END IF' statement */
static void ParseEndIf(ParseContext *c)
{
    switch (CurrentBlockType(c)) {
    case BLOCK_IF:
        fixupbranch(c, c->bptr->u.ifBlock.nxt, codeaddr(c));
        fixupbranch(c, c->bptr->u.ifBlock.end, codeaddr(c));
        PopBlock(c);
        break;
    case BLOCK_ELSE:
        fixupbranch(c, c->bptr->u.elseBlock.end, codeaddr(c));
        PopBlock(c);
        break;
    default:
        ParseError(c, "END IF without a matching IF/ELSE IF/ELSE");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseSelect - parse the 'SELECT' statement */
static void ParseSelect(ParseContext *c)
{
    ParseRValue(c);
    PushBlock(c);
    c->bptr->type = BLOCK_SELECT;
    c->bptr->u.selectBlock.first = VMTRUE;
    c->bptr->u.selectBlock.nxt = 0;
    c->bptr->u.selectBlock.end = 0;
    FRequire(c, T_EOL);
}

/* ParseCase - parse the 'CASE' statement */
static void ParseCase(ParseContext *c)
{
    Token tkn;

    /* handle 'CASE ELSE' statement */
    if ((tkn = GetToken(c)) == T_ELSE) {
        int end;
        switch (CurrentBlockType(c)) {
        case BLOCK_SELECT:

            /* finish the previous case */
            if (c->bptr->u.selectBlock.first) {
                c->bptr->u.selectBlock.first = VMFALSE;
                end = 0;
            }
            else {
                putcbyte(c, OP_BR);
                end = putcword(c, c->bptr->u.selectBlock.end);
            }

            /* start the else case */
            fixupbranch(c, c->bptr->u.selectBlock.nxt, codeaddr(c));
            putcbyte(c, OP_DROP);
            c->bptr->type = BLOCK_CASE_ELSE;
            c->bptr->u.caseElseBlock.end = end;
            break;
        case BLOCK_CASE_ELSE:
            ParseError(c, "only one CASE ELSE allowed in a SELECT");
            break;
        default:
            ParseError(c, "CASE ELSE without a matching SELECT");
            break;
        }
        FRequire(c, T_EOL);
    }

    /* handle 'CASE expr...' statement */
    else {
        int alt = 0;
        int body = 0;
        SaveToken(c, tkn);
        switch (CurrentBlockType(c)) {
        case BLOCK_SELECT:

            /* finish the previous case */
            if (c->bptr->u.selectBlock.first)
                c->bptr->u.selectBlock.first = VMFALSE;
            else {
                putcbyte(c, OP_BR);
                c->bptr->u.selectBlock.end = putcword(c, c->bptr->u.selectBlock.end);
            }
            
            fixupbranch(c, c->bptr->u.selectBlock.nxt, codeaddr(c));
            c->bptr->u.selectBlock.nxt = 0;

            /* handle a list of comma separated expressions or ranges */
            do {

                if (alt) {
                    fixupbranch(c, alt, codeaddr(c));
                    alt = 0;
                }

                putcbyte(c, OP_DUP);
                ParseRValue(c);

                /* handle an 'expr TO expr' range */
                if ((tkn = GetToken(c)) == T_TO) {

                    /* check the lower bound */
                    putcbyte(c, OP_GE);
                    putcbyte(c, OP_BRF);
                    alt = putcword(c, alt);

                    /* check the upper bound */
                    putcbyte(c, OP_DUP);
                    ParseRValue(c);
                    putcbyte(c, OP_LE);
                }

                /* handle a single expression */
                else {
                    SaveToken(c, tkn);
                    putcbyte(c, OP_EQ);
                }

                /* more expressions or ranges follow */
                if ((tkn = GetToken(c)) == ',') {
                    putcbyte(c, OP_BRT);
                    body = putcword(c, body);
                }

                /* last expression or range */
                else {
                    putcbyte(c, OP_BRF);
                    c->bptr->u.selectBlock.nxt = putcword(c, c->bptr->u.selectBlock.nxt);
                }
            } while (tkn == ',');
            c->bptr->u.selectBlock.nxt = merge(c, c->bptr->u.selectBlock.nxt, alt);
            fixupbranch(c, body, codeaddr(c));
            putcbyte(c, OP_DROP);
            Require(c, tkn, T_EOL);
            break;
        default:
            ParseError(c, "CASE without a matching SELECT");
            break;
        }
    }
}

/* ParseEndSelect - parse the 'END SELECT' statement */
static void ParseEndSelect(ParseContext *c)
{
    switch (CurrentBlockType(c)) {
    case BLOCK_SELECT:
        fixupbranch(c, c->bptr->u.selectBlock.nxt, codeaddr(c));
        putcbyte(c, OP_DROP);
        fixupbranch(c, c->bptr->u.selectBlock.end, codeaddr(c));
        PopBlock(c);
        break;
    case BLOCK_CASE_ELSE:
        fixupbranch(c, c->bptr->u.caseElseBlock.end, codeaddr(c));
        PopBlock(c);
        break;
    default:
        ParseError(c, "END SELECT without a matching SELECT");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseEnd - parse the 'END' statement */
static void ParseEnd(ParseContext *c)
{
    putcbyte(c, OP_HALT);
    FRequire(c, T_EOL);
}

/* ParseFor - parse the 'FOR' statement */
static void ParseFor(ParseContext *c)
{
    ParseTreeNode *var, *step;
    VMUVALUE test, body, inst;
    Token tkn;
    PVAL pv;

    PushBlock(c);
    c->bptr->type = BLOCK_FOR;

    /* get the control variable */
    FRequire(c, T_IDENTIFIER);
    var = GetSymbolRef(c, c->token);
    code_lvalue(c, var, &pv);
    FRequire(c, '=');

    /* parse the starting value expression */
    ParseRValue(c);

    /* parse the TO expression and generate the loop termination test */
    test = codeaddr(c);
    (*pv.fcn)(c, PV_STORE, &pv);
    (*pv.fcn)(c, PV_LOAD, &pv);
    FRequire(c, T_TO);
    ParseRValue(c);
    putcbyte(c, OP_LE);
    putcbyte(c, OP_BRT);
    body = putcword(c, 0);

    /* branch to the end if the termination test fails */
    putcbyte(c, OP_BR);
    c->bptr->u.forBlock.end = putcword(c, 0);
    
    /* update the for variable after an iteration of the loop */
    c->bptr->u.forBlock.nxt = codeaddr(c);
    (*pv.fcn)(c, PV_LOAD, &pv);

    /* get the STEP expression */
    if ((tkn = GetToken(c)) == T_STEP) {
        step = ParseExpr(c);
        code_rvalue(c, step);
        tkn = GetToken(c);
    }

    /* no step so default to one */
    else {
        putcbyte(c, OP_SLIT);
        putcbyte(c, 1);
    }

    /* generate the increment code */
    putcbyte(c, OP_ADD);
    inst = putcbyte(c, OP_BR);
    putcword(c, test - inst - 1 - sizeof(VMVALUE));

    /* branch to the loop body */
    fixupbranch(c, body, codeaddr(c));
    Require(c, tkn, T_EOL);
}

/* ParseNext - parse the 'NEXT' statement */
static void ParseNext(ParseContext *c)
{
    ParseTreeNode *var;
    int inst;
    switch (CurrentBlockType(c)) {
    case BLOCK_FOR:
        FRequire(c, T_IDENTIFIER);
        var = GetSymbolRef(c, c->token);
        /* BUG: check to make sure it matches the symbol used in the FOR */
        inst = putcbyte(c, OP_BR);
        putcword(c, c->bptr->u.forBlock.nxt - inst - 1 - sizeof(VMVALUE));
        fixupbranch(c, c->bptr->u.forBlock.end, codeaddr(c));
        PopBlock(c);
        break;
    default:
        ParseError(c, "NEXT without a matching FOR");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseDo - parse the 'DO' statement */
static void ParseDo(ParseContext *c)
{
    PushBlock(c);
    c->bptr->type = BLOCK_DO;
    c->bptr->u.doBlock.nxt = codeaddr(c);
    c->bptr->u.doBlock.end = 0;
    FRequire(c, T_EOL);
}

/* ParseDoWhile - parse the 'DO WHILE' statement */
static void ParseDoWhile(ParseContext *c)
{
    PushBlock(c);
    c->bptr->type = BLOCK_DO;
    c->bptr->u.doBlock.nxt = codeaddr(c);
    ParseRValue(c);
    putcbyte(c, OP_BRF);
    c->bptr->u.doBlock.end = putcword(c, 0);
    FRequire(c, T_EOL);
}

/* ParseDoUntil - parse the 'DO UNTIL' statement */
static void ParseDoUntil(ParseContext *c)
{
    PushBlock(c);
    c->bptr->type = BLOCK_DO;
    c->bptr->u.doBlock.nxt = codeaddr(c);
    ParseRValue(c);
    putcbyte(c, OP_BRT);
    c->bptr->u.doBlock.end = putcword(c, 0);
    FRequire(c, T_EOL);
}

/* ParseLoop - parse the 'LOOP' statement */
static void ParseLoop(ParseContext *c)
{
    int inst;
    switch (CurrentBlockType(c)) {
    case BLOCK_DO:
        inst = putcbyte(c, OP_BR);
        putcword(c, c->bptr->u.doBlock.nxt - inst - 1 - sizeof(VMVALUE));
        fixupbranch(c, c->bptr->u.doBlock.end, codeaddr(c));
        PopBlock(c);
        break;
    default:
        ParseError(c, "LOOP without a matching DO");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseLoopWhile - parse the 'LOOP WHILE' statement */
static void ParseLoopWhile(ParseContext *c)
{
    int inst;
    switch (CurrentBlockType(c)) {
    case BLOCK_DO:
        ParseRValue(c);
        inst = putcbyte(c, OP_BRT);
        putcword(c, c->bptr->u.doBlock.nxt - inst - 1 - sizeof(VMVALUE));
        fixupbranch(c, c->bptr->u.doBlock.end, codeaddr(c));
        PopBlock(c);
        break;
    default:
        ParseError(c, "LOOP without a matching DO");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseLoopUntil - parse the 'LOOP UNTIL' statement */
static void ParseLoopUntil(ParseContext *c)
{
    int inst;
    switch (CurrentBlockType(c)) {
    case BLOCK_DO:
        ParseRValue(c);
        inst = putcbyte(c, OP_BRF);
        putcword(c, c->bptr->u.doBlock.nxt - inst - 1 - sizeof(VMVALUE));
        fixupbranch(c, c->bptr->u.doBlock.end, codeaddr(c));
        PopBlock(c);
        break;
    default:
        ParseError(c, "LOOP without a matching DO");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseAsm - parse the 'ASM ... END ASM' statement */
static void ParseAsm(ParseContext *c)
{
    Token tkn;
    
    /* check for the end of the 'ASM' statement */
    FRequire(c, T_EOL);
    
    /* parse each assembly instruction */
    for (;;) {
    
        /* get the next line */
        if (!GetLine(c))
            ParseError(c, "unexpected end of file in ASM statement");
        
        /* check for the end of the assembly instructions */
        if ((tkn = GetToken(c)) == T_END_ASM)
            break;
            
        /* check for an empty statement */
        else if (tkn == T_EOL)
            continue;
            
        /* check for an opcode name */
        else if (tkn != T_IDENTIFIER)
            ParseError(c, "expected an assembly instruction opcode");
            
        /* assemble a single instruction */
        Assemble(c, c->token);
    }
    
    /* check for the end of the 'END ASM' statement */
    FRequire(c, T_EOL);
}

/* Assemble - assemble a single line */
static void Assemble(ParseContext *c, char *name)
{
    FLASH_SPACE OTDEF *def;
    
    /* lookup the opcode */
    for (def = OpcodeTable; def->name != NULL; ++def)
        if (strcasecmp(name, def->name) == 0) {
            putcbyte(c, def->code);
            switch (def->fmt) {
            case FMT_NONE:
                break;
            case FMT_BYTE:
            case FMT_SBYTE:
                putcbyte(c, ParseIntegerConstant(c));
                break;
            case FMT_WORD:
                putcword(c, ParseIntegerConstant(c));
                break;
            default:
                ParseError(c, "instruction not currently supported");
                break;
            }
            FRequire(c, T_EOL);
            return;
        }
        
    ParseError(c, "undefined opcode");
}

/* ParseIntegerConstant - parse an integer constant expression */
static VMVALUE ParseIntegerConstant(ParseContext *c)
{
    ParseTreeNode *expr;
    expr = ParseExpr(c);
    if (!IsIntegerLit(expr))
        ParseError(c, "expecting an integer constant expression");
    return expr->u.integerLit.value;
}

/* ParseStop - parse the 'STOP' statement */
static void ParseStop(ParseContext *c)
{
    putcbyte(c, OP_HALT);
    FRequire(c, T_EOL);
}

/* ParseGoto - parse the 'GOTO' statement */
static void ParseGoto(ParseContext *c)
{
    FRequire(c, T_IDENTIFIER);
    putcbyte(c, OP_BR);
    putcword(c, ReferenceLabel(c, c->token, codeaddr(c)));
    FRequire(c, T_EOL);
}

/* ParseReturn - parse the 'RETURN' statement */
static void ParseReturn(ParseContext *c)
{
    Token tkn;
            
    /* return with no value returns zero */
    if ((tkn = GetToken(c)) == T_EOL)
        putcbyte(c, OP_RETURNZ);
        
    /* handle return with a value */
    else {
        SaveToken(c, tkn);
        ParseRValue(c);
        putcbyte(c, OP_RETURN);
        FRequire(c, T_EOL);
    }
}

/* ParseInput - parse the 'INPUT' statement */
static void ParseInput(ParseContext *c)
{
    ParseTreeNode *devExpr, *expr;
    Token tkn;

    /* check for file input */
    if ((tkn = GetToken(c)) == '#') {
        devExpr = ParseExpr(c);
        FRequire(c, ',');
    }
    
    /* handle terminal input */
    else {
        SaveToken(c, tkn);
        devExpr = NewParseTreeNode(c, NodeTypeIntegerLit);
        devExpr->type = &c->integerType;
        devExpr->u.integerLit.value = 0;
        
    }
    
    /* peek at the next token */
    tkn = GetToken(c);
    SaveToken(c, tkn);
    
    /* check for a prompt string */
    if (tkn == T_STRING) {
        expr = ParseExpr(c);
        PrintFunctionCall(c, "printStr", devExpr, expr);
        putcbyte(c, OP_DROP);
        FRequire(c, ';');
    }
    
    /* force reading a new line */
    PrintFunctionCall(c, "inputGetLine", devExpr, NULL);
    putcbyte(c, OP_DROP);

    /* parse each input variable */
    if ((tkn = GetToken(c)) == T_IDENTIFIER) {
        SaveToken(c, tkn);
        do {
            ParseTreeNode *expr;
            PVAL pv;
            expr = ParsePrimary(c);
            switch (expr->type->id) {
            case TYPE_INTEGER:
            case TYPE_BYTE:
                PrintFunctionCall(c, "inputInt", devExpr, NULL);
                code_lvalue(c, expr, &pv);
                (*pv.fcn)(c, PV_STORE, &pv);
                break;
            case TYPE_ARRAY:
            case TYPE_POINTER:
                PrintFunctionCall(c, "inputStr", devExpr, expr);
                putcbyte(c, OP_DROP);
                break;
            default:
                ParseError(c, "invalid argument to INPUT");
                break;
            }
        } while ((tkn = GetToken(c)) == ',');
    }
}

/* ParsePrint - handle the 'PRINT' statement */
static void ParsePrint(ParseContext *c)
{
    ParseTreeNode *devExpr, *expr;
    int needNewline = VMTRUE;
    Token tkn;

    /* check for file output */
    if ((tkn = GetToken(c)) == '#') {
        devExpr = ParseExpr(c);
        FRequire(c, ',');
    }
    
    /* handle terminal output */
    else {
        SaveToken(c, tkn);
        devExpr = NewParseTreeNode(c, NodeTypeIntegerLit);
        devExpr->type = &c->integerType;
        devExpr->u.integerLit.value = 0;
    }
    
    while ((tkn = GetToken(c)) != T_EOL) {
        switch (tkn) {
        case ',':
            needNewline = VMFALSE;
            PrintFunctionCall(c, "printTab", devExpr, NULL);
            putcbyte(c, OP_DROP);
            break;
        case ';':
            needNewline = VMFALSE;
            break;
        default:
            needNewline = VMTRUE;
            SaveToken(c, tkn);
            expr = ParseExpr(c);
            switch (expr->type->id) {
            case TYPE_ARRAY:
            case TYPE_POINTER:
                if (expr->type->u.pointerInfo.targetType->id != TYPE_BYTE)
                    ParseError(c, "invalid argument to PRINT");
                PrintFunctionCall(c, "printStr", devExpr, expr);
                putcbyte(c, OP_DROP);
                break;
            case TYPE_INTEGER:
            case TYPE_BYTE:
                PrintFunctionCall(c, "printInt", devExpr, expr);
                putcbyte(c, OP_DROP);
                break;
            default:
                ParseError(c, "invalid argument to PRINT");
                break;
            }
            break;
        }
    }

    if (needNewline) {
        PrintFunctionCall(c, "printNL", devExpr, NULL);
        putcbyte(c, OP_DROP);
    }
}

/* PrintFunctionCall - compile a call to a runtime print function */
static void PrintFunctionCall(ParseContext *c, char *name, ParseTreeNode *devExpr, ParseTreeNode *expr)
{
    ParseTreeNode *functionNode, *callNode;
    ExprListEntry *actual;
    Type *functionType;
    Symbol *symbol;

    if (!(symbol = FindSymbol(&c->globals, name)))
        ParseError(c, "print helper not defined: %s", name);
        
    functionType = symbol->type;
    
    if (functionType->id != TYPE_FUNCTION)
        ParseError(c, "print helper not a function: %s", name);
    else if (functionType->u.functionInfo.arguments.count != (expr ? 2 : 1))
        ParseError(c, "print helper has wrong prototype: %s", name);
        
    functionNode = NewParseTreeNode(c, NodeTypeFunctionLit);
    functionNode->type = functionType;
    functionNode->u.functionLit.symbol = symbol;
    
    callNode = NewParseTreeNode(c, NodeTypeFunctionCall);

    /* intialize the function call node */
    callNode->type = functionType->u.functionInfo.returnType;
    callNode->u.functionCall.fcn = functionNode;
    callNode->u.functionCall.args = NULL;
    
    actual = (ExprListEntry *)LocalAlloc(c, sizeof(ExprListEntry));
    actual->expr = devExpr;
    actual->next = callNode->u.functionCall.args;
    callNode->u.functionCall.args = actual;
    ++callNode->u.functionCall.argc;

    if (expr) {
        actual = (ExprListEntry *)LocalAlloc(c, sizeof(ExprListEntry));
        actual->expr = expr;
        actual->next = callNode->u.functionCall.args;
        callNode->u.functionCall.args = actual;
        ++callNode->u.functionCall.argc;
    }

    code_rvalue(c, callNode);
}

/* DefineLabel - define a local label */
static void DefineLabel(ParseContext *c, char *name, int offset)
{
    Label *label;
    
    /* check to see if the label is already in the table */
    for (label = c->labels; label != NULL; label = label->next)
        if (strcasecmp(name, label->name) == 0) {
            if (!label->fixups)
                ParseError(c, "duplicate label: %s", name);
            else {
                fixupbranch(c, label->fixups, offset);
                label->offset = offset;
                label->fixups = 0;
            }
            return;
        }

    /* allocate the label structure */
    label = (Label *)LocalAlloc(c, sizeof(Label) + strlen(name));
    memset(label, 0, sizeof(Label));
    strcpy(label->name, name);
    label->offset = offset;
    label->next = c->labels;
    c->labels = label;
}

/* ReferenceLabel - add a reference to a local label */
static int ReferenceLabel(ParseContext *c, char *name, int offset)
{
    Label *label;
    
    /* check to see if the label is already in the table */
    for (label = c->labels; label != NULL; label = label->next)
        if (strcasecmp(name, label->name) == 0) {
            int link;
            if (!(link = label->fixups))
                return label->offset - offset - sizeof(VMVALUE);
            else {
                label->fixups = offset;
                return link;
            }
        }

    /* allocate the label structure */
    label = (Label *)LocalAlloc(c, sizeof(Label) + strlen(name));
    memset(label, 0, sizeof(Label));
    strcpy(label->name, name);
    label->fixups = offset;
    label->next = c->labels;
    c->labels = label;

    /* return zero to terminate the fixup list */
    return 0;
}

/* CheckLabels - check for undefined labels */
void CheckLabels(ParseContext *c)
{
    Label *label;
    for (label = c->labels; label != NULL; label = label->next) {
        if (label->fixups)
            Fatal(c->sys, "undefined label: %s", label->name);
    }
}

/* DumpLabels - dump labels */
void DumpLabels(ParseContext *c)
{
    Label *label;
    if (c->labels) {
        VM_printf("labels:\n");
        for (label = c->labels; label != NULL; label = label->next)
            VM_printf("  %08x %s\n", label->offset, label->name);
    }
}

/* CurrentBlockType - make sure there is a block on the stack */
BlockType  CurrentBlockType(ParseContext *c)
{
    return c->bptr < c->blockBuf ? BLOCK_NONE : c->bptr->type;
}

/* PushBlock - push a block on the block stack */
static void PushBlock(ParseContext *c)
{
    if (++c->bptr >= c->btop)
        Fatal(c->sys, "statements too deeply nested");
}

/* PopBlock - pop a block off the block stack */
static void PopBlock(ParseContext *c)
{
    --c->bptr;
}
