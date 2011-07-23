/* db_generate.c - code generation functions
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <string.h>
#include "db_compiler.h"

/* local function prototypes */
static void code_lvalue(ParseContext *c, ParseTreeNode *expr, PVAL *pv);
static Type *code_rvalue(ParseContext *c, ParseTreeNode *expr);
static void code_function_definition(ParseContext *c, ParseTreeNode *node);
static void code_if_statement(ParseContext *c, ParseTreeNode *node);
static void code_select_statement(ParseContext *c, ParseTreeNode *node);
static void code_case_statement(ParseContext *c, ParseTreeNode *node);
static void code_for_statement(ParseContext *c, ParseTreeNode *node);
static void code_do_while_statement(ParseContext *c, ParseTreeNode *node);
static void code_do_until_statement(ParseContext *c, ParseTreeNode *node);
static void code_loop_statement(ParseContext *c, ParseTreeNode *node);
static void code_loop_while_statement(ParseContext *c, ParseTreeNode *node);
static void code_loop_until_statement(ParseContext *c, ParseTreeNode *node);
static void code_return_statement(ParseContext *c, ParseTreeNode *node);
static void code_label_definition(ParseContext *c, ParseTreeNode *node);
static void code_goto_statement(ParseContext *c, ParseTreeNode *node);
static void code_asm_statement(ParseContext *c, ParseTreeNode *node);
static void code_statement_list(ParseContext *c, NodeListEntry *entry);
static void code_shortcircuit(ParseContext *c, int op, ParseTreeNode *expr);
static void code_addressof(ParseContext *c, ParseTreeNode *expr);
static void code_call(ParseContext *c, ParseTreeNode *expr);
static void code_globalref(ParseContext *c, Symbol *sym);
static void code_arrayref(ParseContext *c, ParseTreeNode *expr, PVAL *pv);
static void code_index(ParseContext *c, PValOp fcn, PVAL *pv);
static void PushGenBlock(ParseContext *c, BlockType type);
static void PopGenBlock(ParseContext *c);

/* Generate - generate code for a function */
void Generate(ParseContext *c, ParseTreeNode *node)
{
    PVAL pv;
    
    /* initialize generate block nesting stack */
    c->gtop = (GenBlock *)((char *)c->genBlockBuf + sizeof(c->genBlockBuf));
    c->gptr = c->genBlockBuf - 1;

    /* generate code for the function */
    code_expr(c, node, &pv);
    
    /* make sure the stack is empty */
    if (c->gptr != c->genBlockBuf - 1)
        ParseError(c, "generate block nesting error");
}

/* code_lvalue - generate code for an l-value expression */
static void code_lvalue(ParseContext *c, ParseTreeNode *expr, PVAL *pv)
{
    code_expr(c, expr, pv);
    if (pv->fcn == GEN_NULL)
        ParseError(c,"Expecting an lvalue");
}

/* code_rvalue - generate code for an r-value expression */
static Type *code_rvalue(ParseContext *c, ParseTreeNode *expr)
{
    PVAL pv;
    code_expr(c, expr, &pv);
    if (pv.fcn)
        (*pv.fcn)(c, PV_LOAD, &pv);
    return pv.type;
}

/* code_expr - generate code for an expression parse tree */
void code_expr(ParseContext *c, ParseTreeNode *expr, PVAL *pv)
{
    VMVALUE ival;
    pv->type = expr->type;
    switch (expr->nodeType) {
    case NodeTypeFunctionDefinition:
        code_function_definition(c, expr);
        break;
    case NodeTypeLetStatement:
        code_rvalue(c, expr->u.letStatement.rvalue);
        code_lvalue(c, expr->u.letStatement.lvalue, pv);
        (pv->fcn)(c, PV_STORE, pv);
        break;
    case NodeTypeIfStatement:
        code_if_statement(c, expr);
        break;
    case NodeTypeSelectStatement:
        code_select_statement(c, expr);
        break;
    case NodeTypeCaseStatement:
        code_case_statement(c, expr);
        break;
    case NodeTypeForStatement:
        code_for_statement(c, expr);
        break;
    case NodeTypeDoWhileStatement:
        code_do_while_statement(c, expr);
        break;
    case NodeTypeDoUntilStatement:
        code_do_until_statement(c, expr);
        break;
    case NodeTypeLoopStatement:
        code_loop_statement(c, expr);
        break;
    case NodeTypeLoopWhileStatement:
        code_loop_while_statement(c, expr);
        break;
    case NodeTypeLoopUntilStatement:
        code_loop_until_statement(c, expr);
        break;
    case NodeTypeReturnStatement:
        code_return_statement(c, expr);
        break;
    case NodeTypeCallStatement:
        code_rvalue(c, expr->u.callStatement.expr);
        putcbyte(c, OP_DROP);
        break;
    case NodeTypeLabelDefinition:
        code_label_definition(c, expr);
        break;
    case NodeTypeGotoStatement:
        code_goto_statement(c, expr);
        break;
    case NodeTypeEndStatement:
        putcbyte(c, OP_HALT);
        break;
    case NodeTypeAsmStatement:
        code_asm_statement(c, expr);
        break;
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
        ival = expr->u.integerLit.value;
        if (ival >= -128 && ival <= 127) {
            putcbyte(c, OP_SLIT);
            putcbyte(c, ival);
        }
        else {
            putcbyte(c, OP_LIT);
            putcword(c, ival);
        }
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
    case NodeTypeFunctionCall:
        code_call(c, expr);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeArrayRef:
        code_arrayref(c, expr, pv);
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

/* code_function_definition - generate code for a function definition */
static void code_function_definition(ParseContext *c, ParseTreeNode *node)
{
    if (node->type) {
        putcbyte(c, OP_FRAME);
        putcbyte(c, F_SIZE + node->u.functionDefinition.localOffset);
    }
    code_statement_list(c, node->u.functionDefinition.bodyStatements);
    if (node->type)
        putcbyte(c, OP_RETURNZ);
    else
        putcbyte(c, OP_HALT);
}

/* code_if_statement - generate code for an IF statement */
static void code_if_statement(ParseContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, end;
    code_rvalue(c, node->u.ifStatement.test);
    putcbyte(c, OP_BRF);
    nxt = putcword(c, 0);
    code_statement_list(c, node->u.ifStatement.thenStatements);
    putcbyte(c, OP_BR);
    end = putcword(c, 0);
    fixupbranch(c, nxt, codeaddr(c));
    code_statement_list(c, node->u.ifStatement.elseStatements);
    fixupbranch(c, end, codeaddr(c));
}

/* code_select_statement - generate code for a SELECT statement */
static void code_select_statement(ParseContext *c, ParseTreeNode *node)
{
    /* generate code for the select expression */
    code_rvalue(c, node->u.selectStatement.expr);
    
    /* push a block to handle the select */
    PushGenBlock(c, GEN_BLOCK_SELECT);
    c->gptr->u.selectBlock.first = VMTRUE;
    c->gptr->u.selectBlock.nxt = 0;
    c->gptr->u.selectBlock.end = 0;
    
    /* generate code for each of the cases */
    code_statement_list(c, node->u.selectStatement.caseStatements);
    
    /* handle the case where none of the cases match */
    if (node->u.selectStatement.elseStatements) {
        PVAL pv;
        code_expr(c, node->u.selectStatement.elseStatements, &pv);
    }
    else {
        fixupbranch(c, c->gptr->u.selectBlock.nxt, codeaddr(c));
        putcbyte(c, OP_DROP);
    }
    
    /* handle the branches after each case to the end of the statement */
    fixupbranch(c, c->gptr->u.selectBlock.end, codeaddr(c));
    PopGenBlock(c);
}

/* code_case_statement - generate code for a CASE statement */
static void code_case_statement(ParseContext *c, ParseTreeNode *node)
{
    CaseListEntry *entry = node->u.caseStatement.cases;
    
    /* fixup the branch from the previous case */
    fixupbranch(c, c->gptr->u.selectBlock.nxt, codeaddr(c));
    c->gptr->u.selectBlock.nxt = 0;
    
    /* handle CASE */
    if (entry) {
        VMUVALUE alt = 0;
        VMUVALUE body = 0;
                
        while (entry != NULL) {
        
            /* fixup the branch from the previous entry */
            if (alt) {
                fixupbranch(c, alt, codeaddr(c));
                alt = 0;
            }

            /* duplicate the select expr and get the 'from' expr */
            putcbyte(c, OP_DUP);
            code_rvalue(c, entry->fromExpr);

            /* handle 'from TO to' */
            if (entry->toExpr) {

                /* check the lower bound */
                putcbyte(c, OP_GE);
                putcbyte(c, OP_BRF);
                alt = putcword(c, alt);

                /* check the upper bound */
                putcbyte(c, OP_DUP);
                code_rvalue(c, entry->toExpr);
                putcbyte(c, OP_LE);
            }
            
            /* handle 'expr' */
            else
                putcbyte(c, OP_EQ);
            
            /* move on to the next entry */
            entry = entry->next;

            /* more expressions or ranges follow */
            if (entry) {
                putcbyte(c, OP_BRT);
                body = putcword(c, body);
            }

            /* last expression or range */
            else {
                putcbyte(c, OP_BRF);
                c->gptr->u.selectBlock.nxt = putcword(c, c->gptr->u.selectBlock.nxt);
            }
        }
        
        /* start the body of the CASE clause */
        c->gptr->u.selectBlock.nxt = merge(c, c->gptr->u.selectBlock.nxt, alt);
        fixupbranch(c, body, codeaddr(c));
        putcbyte(c, OP_DROP);
        
        /* generate code for the statements in this case */
        code_statement_list(c, node->u.caseStatement.bodyStatements);
        
        /* finish the case */
        putcbyte(c, OP_BR);
        c->gptr->u.selectBlock.end = putcword(c, c->gptr->u.selectBlock.end);
    }
    
    /* handle CASE ELSE */
    else {
        putcbyte(c, OP_DROP);
    
        /* generate code for the statements in this case */
        code_statement_list(c, node->u.caseStatement.bodyStatements);
    }
}

/* code_for_statement - generate code for a FOR statement */
static void code_for_statement(ParseContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, upd, inst;
    PVAL pv;
    code_rvalue(c, node->u.forStatement.startExpr);
    code_lvalue(c, node->u.forStatement.var, &pv);
    putcbyte(c, OP_BR);
    upd = putcword(c, 0);
    nxt = codeaddr(c);
    code_statement_list(c, node->u.forStatement.bodyStatements);
    (*pv.fcn)(c, PV_LOAD, &pv);
    if (node->u.forStatement.stepExpr)
        code_rvalue(c, node->u.forStatement.stepExpr);
    else {
        putcbyte(c, OP_SLIT);
        putcbyte(c, 1);
    }
    putcbyte(c, OP_ADD);
    fixupbranch(c, upd, codeaddr(c));
    putcbyte(c, OP_DUP);
    (*pv.fcn)(c, PV_STORE, &pv);
    code_rvalue(c, node->u.forStatement.endExpr);
    putcbyte(c, OP_LE);
    inst = putcbyte(c, OP_BRT);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_do_while_statement - generate code for a DO WHILE statement */
static void code_do_while_statement(ParseContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, test, inst;
    putcbyte(c, OP_BR);
    test = putcword(c, 0);
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    fixupbranch(c, test, codeaddr(c));
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRT);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_do_until_statement - generate code for a DO UNTIL statement */
static void code_do_until_statement(ParseContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, test, inst;
    putcbyte(c, OP_BR);
    test = putcword(c, 0);
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    fixupbranch(c, test, codeaddr(c));
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRF);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_loop_statement - generate code for a LOOP statement */
static void code_loop_statement(ParseContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, inst;
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    inst = putcbyte(c, OP_BR);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_loop_while_statement - generate code for a LOOP WHILE statement */
static void code_loop_while_statement(ParseContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, inst;
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRT);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_loop_until_statement - generate code for a LOOP UNTIL statement */
static void code_loop_until_statement(ParseContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, inst;
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRF);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_return_statement - generate code for a RETURN statement */
static void code_return_statement(ParseContext *c, ParseTreeNode *node)
{
    if (node->u.returnStatement.expr) {
        code_rvalue(c, node->u.returnStatement.expr);
        putcbyte(c, OP_RETURN);
    }
    else
        putcbyte(c, OP_RETURNZ);
}

/* code_label_definition - generate code for a label definition */
static void code_label_definition(ParseContext *c, ParseTreeNode *node)
{
    Label *label = node->u.labelDefinition.label;
    VMUVALUE offset = codeaddr(c);
    fixupbranch(c, label->fixups, offset);
    label->state = LS_PLACED;
    label->offset = offset;
    label->fixups = 0;
}

/* code_goto_statement - generate code for a GOTO statement */
static void code_goto_statement(ParseContext *c, ParseTreeNode *node)
{
    Label *label = node->u.gotoStatement.label;
    VMVALUE offset, link;
    putcbyte(c, OP_BR);
    offset = codeaddr(c);
    link = label->fixups;
    if (label->state == LS_PLACED)
        link = label->offset - offset - sizeof(VMVALUE);
    else
        label->fixups = offset;
    putcword(c, link);
}

/* code_asm_statement - generate code for an ASM statement */
static void code_asm_statement(ParseContext *c, ParseTreeNode *node)
{
    int length = node->u.asmStatement.length;
    if (c->cptr + length >= c->ctop)
        Fatal(c->sys, "Bytecode buffer overflow");
    memcpy(c->cptr, node->u.asmStatement.code, length);
    c->cptr += length;
}

/* code_statement_list - code a list of statements */
static void code_statement_list(ParseContext *c, NodeListEntry *entry)
{
    while (entry) {
        PVAL pv;
        code_expr(c, entry->node, &pv);
        entry = entry->next;
    }
}

/* code_shortcircuit - generate code for a conjunction or disjunction of boolean expressions */
static void code_shortcircuit(ParseContext *c, int op, ParseTreeNode *expr)
{
    NodeListEntry *entry = expr->u.exprList.exprs;
    int end = 0;

    code_rvalue(c, entry->node);
    entry = entry->next;

    do {
        putcbyte(c, op);
        end = putcword(c, end);
        code_rvalue(c, entry->node);
    } while ((entry = entry->next) != NULL);

    fixupbranch(c, end, codeaddr(c));
}

/* code_call - code a function call */
static void code_call(ParseContext *c, ParseTreeNode *expr)
{
    NodeListEntry *arg;
    
    /* code each argument expression */
    for (arg = expr->u.functionCall.args; arg != NULL; arg = arg->next)
        code_rvalue(c, arg->node);

    /* get the value of the function */
    code_rvalue(c, expr->u.functionCall.fcn);

    /* call the function */
    putcbyte(c, OP_PUSHJ);
    if (expr->u.functionCall.argc > 0) {
        putcbyte(c, OP_CLEAN);
        putcbyte(c, expr->u.functionCall.argc);
    }
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

/* PushGenBlock - push a generate block on the stack */
static void PushGenBlock(ParseContext *c, BlockType type)
{
    if (++c->gptr >= c->gtop)
        Fatal(c->sys, "statements too deeply nested");
    c->gptr->type = type;
}

/* PopGenBlock - pop a generate block off the stack */
static void PopGenBlock(ParseContext *c)
{
    --c->gptr;
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
