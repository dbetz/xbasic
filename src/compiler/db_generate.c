/* db_generate.c - code generation functions
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include "db_compiler.h"

/* local function prototypes */
static void code_expr(ParseContext *c, ParseTreeNode *expr, PVAL *pv);
static void code_shortcircuit(ParseContext *c, int op, ParseTreeNode *expr);
static void code_addressof(ParseContext *c, ParseTreeNode *expr);
static void code_globalref(ParseContext *c, Symbol *sym);
static void code_arrayref(ParseContext *c, ParseTreeNode *expr, PVAL *pv);
static void code_call(ParseContext *c, ParseTreeNode *expr, PVAL *pv);
static void code_index(ParseContext *c, PValOp fcn, PVAL *pv);

/* code_lvalue - generate code for an l-value expression */
void code_lvalue(ParseContext *c, ParseTreeNode *expr, PVAL *pv)
{
    code_expr(c, expr, pv);
    if (pv->fcn == GEN_NULL)
        ParseError(c,"Expecting an lvalue");
}

/* code_rvalue - generate code for an r-value expression */
Type *code_rvalue(ParseContext *c, ParseTreeNode *expr)
{
    PVAL pv;
    code_expr(c, expr, &pv);
    if (pv.fcn)
        (*pv.fcn)(c, PV_LOAD, &pv);
    return pv.type;
}

/* code_expr - generate code for an expression parse tree */
static void code_expr(ParseContext *c, ParseTreeNode *expr, PVAL *pv)
{
    pv->type = expr->type;
    switch (expr->nodeType) {
    case NodeTypeGlobalRef:
        pv->fcn = expr->u.globalRef.fcn;
        pv->u.sym = expr->u.globalRef.symbol;
        break;
    case NodeTypeLocalRef:
        pv->fcn = expr->u.localRef.fcn;
        pv->u.val = expr->u.localRef.offset;
        break;
    case NodeTypeFunctionLit:
        code_globalref(c, expr->u.functionLit.symbol);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeArrayLit:
        code_globalref(c, expr->u.arrayLit.symbol);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeStringLit:
        putcbyte(c, OP_LIT);
        putcword(c, AddStringRef(c, expr->u.stringLit.string));
        pv->type = &c->bytePointerType;
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeIntegerLit:
        putcbyte(c, OP_LIT);
        putcword(c, expr->u.integerLit.value);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeUnaryOp:
        code_rvalue(c, expr->u.unaryOp.expr);
        putcbyte(c, expr->u.unaryOp.op);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeBinaryOp:
        code_rvalue(c, expr->u.binaryOp.left);
        code_rvalue(c, expr->u.binaryOp.right);
        putcbyte(c, expr->u.binaryOp.op);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeArrayRef:
        code_arrayref(c, expr, pv);
        break;
    case NodeTypeFunctionCall:
        code_call(c, expr, pv);
        break;
    case NodeTypeDisjunction:
        code_shortcircuit(c, OP_BRTSC, expr);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeConjunction:
        code_shortcircuit(c, OP_BRFSC, expr);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeAddressOf:
        code_addressof(c, expr);
        pv->fcn = GEN_NULL;
        break;
    }
}

/* code_shortcircuit - generate code for a conjunction or disjunction of boolean expressions */
static void code_shortcircuit(ParseContext *c, int op, ParseTreeNode *expr)
{
    ExprListEntry *entry = expr->u.exprList.exprs;
    int end = 0;

    code_rvalue(c, entry->expr);
    entry = entry->next;

    do {
        putcbyte(c, op);
        end = putcword(c, end);
        code_rvalue(c, entry->expr);
    } while ((entry = entry->next) != NULL);

    fixupbranch(c, end, codeaddr(c));
}

/* code_addressof - get the address of a data object */
static void code_addressof(ParseContext *c, ParseTreeNode *expr)
{
    if (expr->u.addressOf.expr->type->id == TYPE_POINTER)
        code_rvalue(c, expr->u.addressOf.expr);
    else {
        PVAL pv;
        code_lvalue(c, expr->u.addressOf.expr, &pv);
        (*pv.fcn)(c, PV_REFERENCE, &pv);
    }
}

/* code_globalref - code a global reference */
static void code_globalref(ParseContext *c, Symbol *sym)
{
    VMUVALUE offset = sym->v.variable.offset;
    putcbyte(c, OP_LIT);
    if (offset == UNDEF_VALUE)
        putcword(c, AddLocalSymbolFixup(c, sym, codeaddr(c)));
    else {
        switch (sym->storageClass) {
        case SC_CONSTANT: // function text offset
        case SC_GLOBAL:
            putcword(c, sym->section ? sym->section->base + offset : offset);
            break;
        case SC_COG:
        case SC_HUB:
            putcword(c, offset);
            break;
        default:
            ParseError(c, "unexpected storage class");
            break;
        }
    }
}

/* code_arrayref - code an array reference */
static void code_arrayref(ParseContext *c, ParseTreeNode *expr, PVAL *pv)
{
    code_rvalue(c, expr->u.arrayRef.array);
    code_rvalue(c, expr->u.arrayRef.index);
    if (expr->u.arrayRef.array->type->u.arrayInfo.elementType->id == TYPE_BYTE)
        putcbyte(c, OP_ADD);
    else
        putcbyte(c, OP_INDEX);
    pv->fcn = code_index;
}

/* code_call - code a function call */
static void code_call(ParseContext *c, ParseTreeNode *expr, PVAL *pv)
{
    ExprListEntry *arg;
    
    /* code each argument expression */
    for (arg = expr->u.functionCall.args; arg != NULL; arg = arg->next)
        code_rvalue(c, arg->expr);

    /* get the value of the function */
    code_rvalue(c, expr->u.functionCall.fcn);

    /* call the function */
    putcbyte(c, OP_PUSHJ);
    if (expr->u.functionCall.argc > 0) {
        putcbyte(c, OP_CLEAN);
        putcbyte(c, expr->u.functionCall.argc);
    }
    
    /* we've got an rvalue now */
    pv->fcn = GEN_NULL;
}

/* code_global - compile a global variable reference */
void code_global(ParseContext *c, PValOp fcn, PVAL *pv)
{
    code_globalref(c, pv->u.sym);
    switch (fcn) {
    case PV_LOAD:
        putcbyte(c, OP_LOAD);
        break;
    case PV_STORE:
        putcbyte(c, OP_STORE);
        break;
    case PV_REFERENCE:
        // just return the address
        break;
    }
}

/* code_local - compile an local reference */
void code_local(ParseContext *c, PValOp fcn, PVAL *pv)
{
    switch (fcn) {
    case PV_LOAD:
        putcbyte(c, OP_LREF);
        putcbyte(c, pv->u.val);
        break;
    case PV_STORE:
        putcbyte(c, OP_LSET);
        putcbyte(c, pv->u.val);
        break;
    case PV_REFERENCE:
        // just return the address
        break;
    }
}

/* code_index - compile a vector reference */
static void code_index(ParseContext *c, PValOp fcn, PVAL *pv)
{
    switch (fcn) {
    case PV_LOAD:
        if (pv->type->id == TYPE_BYTE)
            putcbyte(c, OP_LOADB);
        else
            putcbyte(c, OP_LOAD);
        break;
    case PV_STORE:
        if (pv->type->id == TYPE_BYTE)
            putcbyte(c, OP_STOREB);
        else
            putcbyte(c, OP_STORE);
        break;
    case PV_REFERENCE:
        // just return the address
        break;
    }
}

/* codeaddr - get the current code address (actually, offset) */
VMUVALUE codeaddr(ParseContext *c)
{
    return (VMUVALUE)(c->cptr - c->codeBuf);
}

/* putcbyte - put a code byte into the code buffer */
VMUVALUE putcbyte(ParseContext *c, int b)
{
    VMUVALUE addr = codeaddr(c);
    if (c->cptr >= c->ctop)
        Fatal(c->sys, "Bytecode buffer overflow");
    *c->cptr++ = b;
    return addr;
}

/* putcword - put a code word into the code buffer */
VMUVALUE putcword(ParseContext *c, VMVALUE w)
{
    VMUVALUE addr = codeaddr(c);
    uint8_t *p;
    int cnt = sizeof(VMVALUE);
    if (c->cptr + sizeof(VMVALUE) > c->ctop)
        Fatal(c->sys, "Bytecode buffer overflow");
     c->cptr += sizeof(VMVALUE);
     p = c->cptr;
     while (--cnt >= 0) {
        *--p = w;
        w >>= 8;
    }
    return addr;
}

/* rd_cword - get a code word from the code buffer */
VMVALUE rd_cword(ParseContext *c, VMUVALUE off)
{
    int cnt = sizeof(VMVALUE);
    VMVALUE w = 0;
    while (--cnt >= 0)
        w = (w << 8) | c->codeBuf[off++];
    return w;
}

/* wr_cword - put a code word into the code buffer */
void wr_cword(ParseContext *c, VMUVALUE off, VMVALUE w)
{
    uint8_t *p = &c->codeBuf[off] + sizeof(VMVALUE);
    int cnt = sizeof(VMVALUE);
    while (--cnt >= 0) {
        *--p = w;
        w >>= 8;
    }
}

/* merge - merge two reference chains */
int merge(ParseContext *c, VMUVALUE chn, VMUVALUE chn2)
{
    VMVALUE last, nxt;

    /* if the chain we're adding is empty, just return the original chain */
    if (!chn2)
        return chn;

    /* find the last entry in the new chain */
    last = chn2;
    while (last != 0) {
        if (!(nxt = rd_cword(c, last)))
            break;
        last = nxt;
    }

    /* link the last entry in the new chain to the first entry in the original chain */
    wr_cword(c, last, chn);

    /* return the new chain now linked to the original chain */
    return chn2;
}

/* fixup - fixup a reference chain */
void fixup(ParseContext *c, VMUVALUE chn, VMUVALUE val)
{
    while (chn != 0) {
        VMUVALUE nxt = rd_cword(c, chn);
        wr_cword(c, chn, val);
        chn = nxt;
    }
}

/* fixupbranch - fixup a reference chain */
void fixupbranch(ParseContext *c, VMUVALUE chn, VMUVALUE val)
{
    while (chn != 0) {
        VMUVALUE nxt = rd_cword(c, chn);
        VMUVALUE off = val - (chn + sizeof(VMUVALUE));
        wr_cword(c, chn, off);
        chn = nxt;
    }
}
