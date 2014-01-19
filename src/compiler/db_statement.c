/* db_statement.c - statement parser
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "db_compiler.h"
#include "db_vmdebug.h"

/* statement handler prototypes */
static void ParseInclude(ParseContext *c);
static void ParseOption(ParseContext *c);
static void SetIntegerOption(ParseContext *c, int *pValue);
static void ParseDef(ParseContext *c);
static void ParseConstantDef(ParseContext *c, char *name);
static void ParseFunctionDef(ParseContext *c, char *name);
static void ParseFunctionDef_pass1(ParseContext *c, char *name);
static void ParseFunctionDef_pass23(ParseContext *c, char *name);
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
static void ParseGoto(ParseContext *c);
static void ParseReturn(ParseContext *c);
static void ParseInput(ParseContext *c);
static void ParsePrint(ParseContext *c);

/* prototypes */
static void StartFunction(ParseContext *c, Symbol *sym);
static ParseTreeNode *BuildHandlerCall(ParseContext *c, char *name, ParseTreeNode *devExpr, ParseTreeNode *expr);
static ParseTreeNode *BuildHandlerFunctionCall(ParseContext *c, char *name, ParseTreeNode *devExpr, ParseTreeNode *expr);
static void DefineLabel(ParseContext *c, char *name);
static void PushBlock(ParseContext *c, BlockType type, ParseTreeNode *node);
static void PopBlock(ParseContext *c);
static void Assemble(ParseContext *c, char *opname);
static VMVALUE ParseIntegerConstant(ParseContext *c);

/*
    pass 1: handle definitions
    pass 2: collect dependencies
    pass 3: compile code
*/

/* ParseStatement - parse a statement */
void ParseStatement(ParseContext *c, int tkn)
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
        if (c->pass > 1) {
            if (!c->functionType) {
                switch (c->mainState) {
                case MAIN_NOT_DEFINED:
                    StartFunction(c, NULL);
                    c->mainState = MAIN_IN_PROGRESS;
                    break;
                case MAIN_DEFINED:
                    ParseError(c, "all main code must be in the same place");
                    break;
                case MAIN_IN_PROGRESS:
                    // nothing to do
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
            case T_STOP:
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
                    DefineLabel(c, c->token);
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
    if (!PushFile(c, name))
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
    int tkn;

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
    /* end the main function if it is in progress */
    if (c->mainState == MAIN_IN_PROGRESS) {
        EndFunction(c);
        c->mainState = MAIN_DEFINED;
    }

    /* don't allow nested functions or subroutines (for now anyway) */
    if (c->functionType)
        ParseError(c, "nested subroutines and functions are not supported");
    
    if (c->pass == 1)
        ParseFunctionDef_pass1(c, name);
    else
        ParseFunctionDef_pass23(c, name);
}

/* ParseFunctionDef_pass1 - parse a 'DEF <name>' statement during pass 1 */
static void ParseFunctionDef_pass1(ParseContext *c, char *name)
{
    Symbol *sym;
    Type *type;
    int tkn;
    
    /* make the function type */
    type = NewGlobalType(c, TYPE_FUNCTION);
    type->u.functionInfo.returnType = &c->integerType;
    InitSymbolTable(&type->u.functionInfo.arguments);
    c->functionType = type;

    /* enter the function name in the global symbol table */
    sym = AddGlobalSymbol(c, name, SC_CONSTANT, type, NULL);

    /* get the argument list */
    if ((tkn = GetToken(c)) == '(') {
        if ((tkn = GetToken(c)) != ')') {
            SymbolTable *table = &type->u.functionInfo.arguments;
            int offset = 0;
            SaveToken(c, tkn);
            do {
                char name[MAXTOKEN];
                VMUVALUE size;
                type = ParseVariableDecl(c, name, &size);
                if (type->id == TYPE_ARRAY && size != 0)
                    ParseError(c, "Array arguments can not specify size");
                AddFormalArgument(c, table, name, type, offset++);
            } while ((tkn = GetToken(c)) == ',');
        }
        Require(c, tkn, ')');
    }
    else
        SaveToken(c, tkn);
        
    FRequire(c, T_EOL);
}

/* ParseFunctionDef_pass23 - parse a 'DEF <name>' statement during passes 2 and 3 */
static void ParseFunctionDef_pass23(ParseContext *c, char *name)
{
    Symbol *sym;
    sym = FindSymbol(&c->globals, name);
    sym->section = c->textTarget;
    StartFunction(c, sym);
}

/* StartFunction - start compiling a function */
static void StartFunction(ParseContext *c, Symbol *sym)
{
    ParseTreeNode *node;

    /* make the function node */
    node = NewParseTreeNode(c, NodeTypeFunctionDefinition);
    node->type = sym ? sym->type : NULL;
    node->u.functionDefinition.symbol = sym;
    InitSymbolTable(&node->u.functionDefinition.locals);
    node->u.functionDefinition.labels = NULL;
    node->u.functionDefinition.localOffset = 0;
    c->dependencies = NULL;
    c->pNextDependency = &c->dependencies;
    
    /* setup to compile the function body */
    PushBlock(c, BLOCK_FUNCTION, node);
    c->bptr->pNextStatement = &node->u.functionDefinition.bodyStatements;
    c->functionType = node->type;
    c->function = node;
}

/* EndFunction - end the current or main function definition */
void EndFunction(ParseContext *c)
{
    Dependency *d;

    /* check for unterminated blocks */
	switch (c->bptr->type) {
    case BLOCK_IF:
    case BLOCK_ELSE:
		ParseError(c, "expecting END IF");
    case BLOCK_SELECT:
    case BLOCK_CASE:
		ParseError(c, "expecting END SELECT");
    case BLOCK_FOR:
		ParseError(c, "expecting NEXT");
	case BLOCK_DO:
		ParseError(c, "expecting LOOP");
	default:
		break;
	}

    /* make sure all referenced labels were defined */
    CheckLabels(c);

    /* store the dependencies on pass 2 */
    if (c->pass == 2) {
    
        /* store dependencies */
        if (c->functionType)
            c->function->u.functionDefinition.symbol->type->u.functionInfo.dependencies = c->dependencies;
        else
            c->mainDependencies = c->dependencies;
            
        /* show the parse tree if requested */
        if (c->flags & COMPILER_DEBUG) {
            xbInfo(c->sys, "\n");
            PrintNode(c->function, 0);
            if ((d = c->dependencies) != NULL) {
                xbInfo(c->sys, "dependencies:\n");
                for (; d != NULL; d = d->next)
                    xbInfo(c->sys, "  %s\n", d->symbol->name);
            }
        }
    }
    
    /* generate code on pass 3 */
    else {
    
        /* handle named functions */
        if (c->functionType) {
            Symbol *sym = c->function->u.functionDefinition.symbol;
            for (d = c->mainDependencies; d != NULL; d = d->next)
                if (sym == d->symbol)
                    break;
            if (d)
                StoreCode(c);
        }
        
        /* always store the main function */
        else
            StoreCode(c);
    }
    
    /* exit the function block */
    PopBlock(c);
    c->functionType = NULL;
    c->function = NULL;
    
    /* empty the local heap */
    xbLocalFreeAll(c->sys);
}

/* ParseEndDef - parse the 'END DEF' statement */
static void ParseEndDef(ParseContext *c)
{
    if (c->functionType) {
        if (c->pass > 1)
            EndFunction(c);
        c->functionType = NULL;
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
    int tkn;

    /* parse variable declarations */
    do {
        Type *type;

        /* get variable name */
        type = ParseVariableDecl(c, name, &size);
        isArray = (type->id == TYPE_ARRAY);

        /* check for being inside a function definition */
        if (c->functionType) {
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
                
            if (c->pass > 1) {
            
                /* add the local symbol */
                AddLocal(c, name, type, -F_SIZE - c->function->u.functionDefinition.localOffset - 1);
                c->function->u.functionDefinition.localOffset += ValueSize(type, 0);
                
                /* compile the initialization code */
                if (expr) {
                    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeLetStatement);
                    node->u.letStatement.lvalue = GetSymbolRef(c, name);
                    node->u.letStatement.rvalue = expr;
                    AddNodeToList(c, &c->bptr->pNextStatement, node);
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
    int isArray = FALSE;
    int tkn;

    /* parse the variable name */
    FRequire(c, T_IDENTIFIER);
    strcpy(name, c->token);
    
    /* handle arrays */
    if ((tkn = GetToken(c)) == '(') {
        isArray = TRUE;

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
    xbLocalFreeAll(c->sys);
    return value;
}
    
/* ParseArrayInitializers - parse array initializers */
static VMUVALUE ParseArrayInitializers(ParseContext *c, Type *type, VMUVALUE size)
{
    VMVALUE *wp = (VMVALUE *)c->cptr;
    uint8_t *bp = (uint8_t *)c->cptr;
    VMUVALUE remaining = size;
    VMUVALUE count = 0;
    int tkn;

    /* handle a bracketed list of initializers */
    if ((tkn = GetToken(c)) == '{') {
        VMVALUE initializer;
        int done = FALSE;
        
        /* handle each line of initializers */
        while (!done) {
            int lineDone = FALSE;

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
                    lineDone = TRUE;
                    break;
                case '}':
                    lineDone = TRUE;
                    done = TRUE;
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

/* ParseImpliedLetOrFunctionCall - parse an implied let statement or a function call */
static void ParseImpliedLetOrFunctionCall(ParseContext *c)
{
    ParseTreeNode *node, *expr;
    int tkn;
    expr = ParsePrimary(c);
    switch (tkn = GetToken(c)) {
    case '=':
        node = NewParseTreeNode(c, NodeTypeLetStatement);
        node->u.letStatement.lvalue = expr;
        node->u.letStatement.rvalue = ParseExpr(c);
        break;
    default:
        SaveToken(c, tkn);
        node = NewParseTreeNode(c, NodeTypeCallStatement);
        node->u.callStatement.expr = expr;
        break;
    }
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_EOL);
}

/* ParseLet - parse the 'LET' statement */
static void ParseLet(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeLetStatement);
    node->u.letStatement.lvalue = ParsePrimary(c);
    FRequire(c, '=');
    node->u.letStatement.rvalue = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_EOL);
}

/* ParseIf - parse the 'IF' statement */
static void ParseIf(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeIfStatement);
    int tkn;
    node->u.ifStatement.test = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_THEN);
    PushBlock(c, BLOCK_IF, node);
    c->bptr->pNextStatement = &node->u.ifStatement.thenStatements;
    if ((tkn = GetToken(c)) != T_EOL) {
        ParseStatement(c, tkn);
        PopBlock(c);
    }
    Require(c, tkn, T_EOL);
}

/* ParseElseIf - parse the 'ELSE IF' statement */
static void ParseElseIf(ParseContext *c)
{
    NodeListEntry **pNext;
    ParseTreeNode *node;
    switch (c->bptr->type) {
    case BLOCK_IF:
        node = NewParseTreeNode(c, NodeTypeIfStatement);
        pNext = &c->bptr->node->u.ifStatement.elseStatements;
        AddNodeToList(c, &pNext, node);
        c->bptr->node = node;
        node->u.ifStatement.test = ParseExpr(c);
        c->bptr->pNextStatement = &node->u.ifStatement.thenStatements;
        FRequire(c, T_THEN);
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
    switch (c->bptr->type) {
    case BLOCK_IF:
        c->bptr->type = BLOCK_ELSE;
        c->bptr->pNextStatement = &c->bptr->node->u.ifStatement.elseStatements;
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
    switch (c->bptr->type) {
    case BLOCK_IF:
    case BLOCK_ELSE:
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
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeSelectStatement);
    node->u.selectStatement.expr = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_SELECT, node);
    c->bptr->pNextStatement = &node->u.selectStatement.caseStatements;
    FRequire(c, T_EOL);
}

/* ParseCase - parse the 'CASE' statement */
static void ParseCase(ParseContext *c)
{
    ParseTreeNode *node;
    int tkn;
    
    if (c->bptr->type == BLOCK_CASE)
        PopBlock(c);
    if (c->bptr->type != BLOCK_SELECT)
        ParseError(c, "CASE outside of SELECT");
        
    /* make a new case statement node */
    node = NewParseTreeNode(c, NodeTypeCaseStatement);
    
    /* handle 'CASE ELSE' statement */
    if ((tkn = GetToken(c)) == T_ELSE) {
        FRequire(c, T_EOL);
        if (c->bptr->node->u.selectStatement.elseStatements)
            ParseError(c, "only one CASE ELSE clause allowed");
        c->bptr->node->u.selectStatement.elseStatements = node;
    }

    /* handle 'CASE expr...' statement */
    else {
        CaseListEntry **pNext, *entry;

        SaveToken(c, tkn);

        AddNodeToList(c, &c->bptr->pNextStatement, node);
        pNext = &node->u.caseStatement.cases;
        
        /* handle a list of comma separated expressions or ranges */
        do {

            /* parse the single value or begining of a range */
            entry = (CaseListEntry *)xbLocalAlloc(c->sys, sizeof(CaseListEntry));
            entry->fromExpr = ParseExpr(c);
            entry->next = NULL;
            *pNext = entry;
            pNext = &entry->next;

            /* handle an 'expr TO expr' range */
            if ((tkn = GetToken(c)) == T_TO) {
                entry->toExpr = ParseExpr(c);
            }

            /* handle a single expression */
            else {
                SaveToken(c, tkn);
                entry->toExpr = NULL;
            }

        } while ((tkn = GetToken(c)) == ',');
        
        Require(c, tkn, T_EOL);
    }

    PushBlock(c, BLOCK_CASE, node);
    c->bptr->pNextStatement = &node->u.caseStatement.bodyStatements;
}

/* ParseEndSelect - parse the 'END SELECT' statement */
static void ParseEndSelect(ParseContext *c)
{
    if (c->bptr->type == BLOCK_CASE) {
        PopBlock(c);
    }
    if (c->bptr->type == BLOCK_SELECT) {
        PopBlock(c);
    }
    else
        ParseError(c, "END SELECT without a matching SELECT");
    FRequire(c, T_EOL);
}

/* ParseEnd - parse the 'END' statement */
static void ParseEnd(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeEndStatement);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_EOL);
}

/* ParseFor - parse the 'FOR' statement */
static void ParseFor(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeForStatement);
    int tkn;

    AddNodeToList(c, &c->bptr->pNextStatement, node);

    PushBlock(c, BLOCK_FOR, node);
    c->bptr->pNextStatement = &node->u.forStatement.bodyStatements;

    /* get the control variable */
    FRequire(c, T_IDENTIFIER);
    node->u.forStatement.var = GetSymbolRef(c, c->token);

    /* parse the starting value expression */
    FRequire(c, '=');
    node->u.forStatement.startExpr = ParseExpr(c);

    /* parse the TO expression and generate the loop termination test */
    FRequire(c, T_TO);
    node->u.forStatement.endExpr = ParseExpr(c);

    /* get the STEP expression */
    if ((tkn = GetToken(c)) == T_STEP) {
        node->u.forStatement.stepExpr = ParseExpr(c);
        tkn = GetToken(c);
    }
    Require(c, tkn, T_EOL);
}

/* ParseNext - parse the 'NEXT' statement */
static void ParseNext(ParseContext *c)
{
    switch (c->bptr->type) {
    case BLOCK_FOR:
        FRequire(c, T_IDENTIFIER);
        //if (GetSymbolRef(c, c->token) != c->bptr->node->u.forStatement.var)
        //    ParseError(c, "wrong variable in FOR");
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
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeLoopStatement);
    node->u.loopStatement.test = NULL;
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_DO, node);
    c->bptr->pNextStatement = &node->u.loopStatement.bodyStatements;
    FRequire(c, T_EOL);
}

/* ParseDoWhile - parse the 'DO WHILE' statement */
static void ParseDoWhile(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeDoWhileStatement);
    node->u.loopStatement.test = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_DO, node);
    c->bptr->pNextStatement = &node->u.loopStatement.bodyStatements;
    FRequire(c, T_EOL);
}

/* ParseDoUntil - parse the 'DO UNTIL' statement */
static void ParseDoUntil(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeDoUntilStatement);
    node->u.loopStatement.test = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_DO, node);
    c->bptr->pNextStatement = &node->u.loopStatement.bodyStatements;
    FRequire(c, T_EOL);
}

/* ParseLoop - parse the 'LOOP' statement */
static void ParseLoop(ParseContext *c)
{
    switch (c->bptr->type) {
    case BLOCK_DO:
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
    switch (c->bptr->type) {
    case BLOCK_DO:
        if (c->bptr->node->nodeType != NodeTypeLoopStatement)
            ParseError(c, "can't have a test at both the top and bottom of a loop");
        c->bptr->node->nodeType = NodeTypeLoopWhileStatement;
        c->bptr->node->u.loopStatement.test = ParseExpr(c);
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
    switch (c->bptr->type) {
    case BLOCK_DO:
        if (c->bptr->node->nodeType != NodeTypeLoopStatement)
            ParseError(c, "can't have a test at both the top and bottom of a loop");
        c->bptr->node->nodeType = NodeTypeLoopUntilStatement;
        c->bptr->node->u.loopStatement.test = ParseExpr(c);
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
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeAsmStatement);
    uint8_t *start = c->cptr;
    int length;
    int tkn;
    
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
    
    /* store the code */
    length = c->cptr - start;
    node->u.asmStatement.code = xbLocalAlloc(c->sys, length);
    node->u.asmStatement.length = length;
    memcpy(node->u.asmStatement.code, start, length);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    c->cptr = start;
    
    /* check for the end of the 'END ASM' statement */
    FRequire(c, T_EOL);
}

/* Assemble - assemble a single line */
static void Assemble(ParseContext *c, char *name)
{
    int PasmAssemble1(char *line, uint32_t *pValue);
    FLASH_SPACE OTDEF *def;
    uint32_t value;
    char *p;
    
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
            case FMT_NATIVE:
                for (p = c->linePtr; *p != '\0' && isspace(*p); ++p)
                    ;
                if (isdigit(*p))
                    putcword(c, ParseIntegerConstant(c));
                else {
                    if (!PasmAssemble1(c->linePtr, &value))
                        ParseError(c, "native assembly failed");
                    putcword(c, (VMVALUE)value);
                    for (p = c->linePtr; *p != '\0' && *p != '\n'; ++p)
                        ;
                    c->linePtr = p;
                }
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

/* DefineLabel - define a label */
static void DefineLabel(ParseContext *c, char *name)
{
    ParseTreeNode *node;
    Label *label;
    
    /* check to see if the label is already in the table */
    for (label = c->function->u.functionDefinition.labels; label != NULL; label = label->next)
        if (strcasecmp(name, label->name) == 0) {
            if (label->state != LS_UNDEFINED)
                ParseError(c, "duplicate label: %s", label->name);
            break;
        }

    /* allocate the label structure */
    if (!label) {
        label = (Label *)xbLocalAlloc(c->sys, sizeof(Label) + strlen(name));
        memset(label, 0, sizeof(Label));
        strcpy(label->name, name);
        label->state = LS_DEFINED;
        label->next = c->function->u.functionDefinition.labels;
        c->function->u.functionDefinition.labels = label;
    }
    
    /* add a label definition node */
    node = NewParseTreeNode(c, NodeTypeLabelDefinition);
    node->u.labelDefinition.label = label;
    AddNodeToList(c, &c->bptr->pNextStatement, node);
}

/* ParseGoto - parse the 'GOTO' statement */
static void ParseGoto(ParseContext *c)
{
    ParseTreeNode *node;
    Label *label;
    
    FRequire(c, T_IDENTIFIER);

    /* check to see if the label is already in the table */
    for (label = c->function->u.functionDefinition.labels; label != NULL; label = label->next)
        if (strcasecmp(c->token, label->name) == 0)
            break;

    /* allocate the label structure */
    if (!label) {
        label = (Label *)xbLocalAlloc(c->sys, sizeof(Label) + strlen(c->token));
        memset(label, 0, sizeof(Label));
        strcpy(label->name, c->token);
        label->state = LS_UNDEFINED;
        label->next = c->function->u.functionDefinition.labels;
        c->function->u.functionDefinition.labels = label;
    }

    node = NewParseTreeNode(c, NodeTypeGotoStatement);
    node->u.gotoStatement.label = label;
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    
    FRequire(c, T_EOL);
}

/* ParseReturn - parse the 'RETURN' statement */
static void ParseReturn(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeReturnStatement);
    int tkn;
            
    /* return with no value returns zero */
    if ((tkn = GetToken(c)) == T_EOL)
        node->u.returnStatement.expr = NULL;
        
    /* handle return with a value */
    else {
        SaveToken(c, tkn);
        node->u.returnStatement.expr = ParseExpr(c);
        FRequire(c, T_EOL);
    }
    
    /* add the statement to the current function */
    AddNodeToList(c, &c->bptr->pNextStatement, node);
}

/* ParseInput - parse the 'INPUT' statement */
static void ParseInput(ParseContext *c)
{
    ParseTreeNode *devExpr, *expr;
    int tkn;

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
        AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printStr", devExpr, expr));
        FRequire(c, ';');
    }
    
    /* force reading a new line */
    AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "inputGetLine", devExpr, NULL));

    /* parse each input variable */
    if ((tkn = GetToken(c)) == T_IDENTIFIER) {
        SaveToken(c, tkn);
        do {
            ParseTreeNode *node;
            expr = ParsePrimary(c);
            switch (expr->type->id) {
            case TYPE_INTEGER:
            case TYPE_BYTE:
                node = NewParseTreeNode(c, NodeTypeLetStatement);
                node->u.letStatement.lvalue = expr;
                node->u.letStatement.rvalue = BuildHandlerFunctionCall(c, "inputInt", devExpr, NULL);
                AddNodeToList(c, &c->bptr->pNextStatement, node);
                break;
            case TYPE_ARRAY:
            case TYPE_POINTER:
                AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "inputStr", devExpr, expr));
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
    int needNewline = TRUE;
    int tkn;

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
            needNewline = FALSE;
            AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printTab", devExpr, NULL));
            break;
        case ';':
            needNewline = FALSE;
            break;
        default:
            needNewline = TRUE;
            SaveToken(c, tkn);
            expr = ParseExpr(c);
            switch (expr->type->id) {
            case TYPE_ARRAY:
            case TYPE_POINTER:
                if (expr->type->u.pointerInfo.targetType->id != TYPE_BYTE)
                    ParseError(c, "invalid argument to PRINT");
                AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printStr", devExpr, expr));
                break;
            case TYPE_INTEGER:
            case TYPE_BYTE:
                AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printInt", devExpr, expr));
                break;
            default:
                ParseError(c, "invalid argument to PRINT");
                break;
            }
            break;
        }
    }

    if (needNewline)
        AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printNL", devExpr, NULL));
}

/* BuildHandlerCall - compile a call to a runtime print function */
static ParseTreeNode *BuildHandlerCall(ParseContext *c, char *name, ParseTreeNode *devExpr, ParseTreeNode *expr)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeCallStatement);
    node->u.callStatement.expr = BuildHandlerFunctionCall(c, name, devExpr, expr);
    return node;
}

/* BuildHandlerFunctionCall - compile a call to a runtime print function */
static ParseTreeNode *BuildHandlerFunctionCall(ParseContext *c, char *name, ParseTreeNode *devExpr, ParseTreeNode *expr)
{
    ParseTreeNode *functionNode, *callNode;
    NodeListEntry *actual;
    Type *functionType;
    Symbol *symbol;

    if (!(symbol = FindSymbol(&c->globals, name)))
        ParseError(c, "print helper not defined: %s", name);
        
    functionType = symbol->type;
    
    if (functionType->id != TYPE_FUNCTION)
        ParseError(c, "handler is not a function: %s", name);
    else if (functionType->u.functionInfo.arguments.count != (expr ? 2 : 1))
        ParseError(c, "handler has wrong prototype: %s", name);
        
    functionNode = NewParseTreeNode(c, NodeTypeFunctionLit);
    functionNode->type = functionType;
    functionNode->u.functionLit.symbol = symbol;
    AddDependency(c, symbol);
  
    callNode = NewParseTreeNode(c, NodeTypeFunctionCall);

    /* intialize the function call node */
    callNode->type = functionType->u.functionInfo.returnType;
    callNode->u.functionCall.fcn = functionNode;
    callNode->u.functionCall.args = NULL;
    
    actual = (NodeListEntry *)xbLocalAlloc(c->sys, sizeof(NodeListEntry));
    actual->node = devExpr;
    actual->next = callNode->u.functionCall.args;
    callNode->u.functionCall.args = actual;
    ++callNode->u.functionCall.argc;

    if (expr) {
        actual = (NodeListEntry *)xbLocalAlloc(c->sys, sizeof(NodeListEntry));
        actual->node = expr;
        actual->next = callNode->u.functionCall.args;
        callNode->u.functionCall.args = actual;
        ++callNode->u.functionCall.argc;
    }

    /* return the function call node */
    return callNode;
}

/* CheckLabels - check for undefined labels */
void CheckLabels(ParseContext *c)
{
    Label *label;
    for (label = c->function->u.functionDefinition.labels; label != NULL; label = label->next) {
        if (label->fixups)
            Fatal(c, "undefined label: %s", label->name);
    }
}

/* DumpLabels - dump labels */
void DumpLabels(ParseContext *c)
{
    Label *label;
    if (c->function->u.functionDefinition.labels) {
        xbInfo(c->sys, "labels:\n");
        for (label = c->function->u.functionDefinition.labels; label != NULL; label = label->next)
            xbInfo(c->sys, "  %08x %s\n", label->offset, label->name);
    }
}

/* PushBlock - push a block on the stack */
static void PushBlock(ParseContext *c, BlockType type, ParseTreeNode *node)
{
    if (++c->bptr >= c->btop)
        Fatal(c, "statements too deeply nested");
    c->bptr->type = type;
    c->bptr->node = node;
}

/* PopBlock - pop a block off the stack */
static void PopBlock(ParseContext *c)
{
    --c->bptr;
}
