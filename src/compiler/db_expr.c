/* db_expr.c - expression parser
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "db_compiler.h"

/* local function prototypes */
static ParseTreeNode *ParseExpr2(ParseContext *c);
static ParseTreeNode *ParseExpr3(ParseContext *c);
static ParseTreeNode *ParseExpr4(ParseContext *c);
static ParseTreeNode *ParseExpr5(ParseContext *c);
static ParseTreeNode *ParseExpr6(ParseContext *c);
static ParseTreeNode *ParseExpr7(ParseContext *c);
static ParseTreeNode *ParseExpr8(ParseContext *c);
static ParseTreeNode *ParseExpr9(ParseContext *c);
static ParseTreeNode *ParseExpr10(ParseContext *c);
static ParseTreeNode *ParseExpr11(ParseContext *c);
static ParseTreeNode *ParseSimplePrimary(ParseContext *c);
static ParseTreeNode *ParseArrayReference(ParseContext *c, ParseTreeNode *arrayNode);
static ParseTreeNode *ParseCall(ParseContext *c, ParseTreeNode *functionNode);
static ParseTreeNode *MakeUnaryOpNode(ParseContext *c, int op, ParseTreeNode *expr);
static ParseTreeNode *MakeBinaryOpNode(ParseContext *c, int op, ParseTreeNode *left, ParseTreeNode *right);

/* ParseRValue - parse and generate code for an r-value */
Type *ParseRValue(ParseContext *c)
{
    return code_rvalue(c, ParseExpr(c));
}

/* ParseExpr - handle the OR operator */
ParseTreeNode *ParseExpr(ParseContext *c)
{
    ParseTreeNode *node;
    Token tkn;
    node = ParseExpr2(c);
    if ((tkn = GetToken(c)) == T_OR) {
        ParseTreeNode *node2 = NewParseTreeNode(c, NodeTypeDisjunction);
        ExprListEntry *entry, **pLast;
        node2->type = &c->integerType;
        node2->u.exprList.exprs = entry = (ExprListEntry *)LocalAlloc(c, sizeof(ExprListEntry));
        entry->expr = node;
        entry->next = NULL;
        pLast = &entry->next;
        do {
            entry = (ExprListEntry *)LocalAlloc(c, sizeof(ExprListEntry));
            entry->expr = ParseExpr2(c);
            entry->next = NULL;
            *pLast = entry;
            pLast = &entry->next;
        } while ((tkn = GetToken(c)) == T_OR);
        node = node2;
    }
    SaveToken(c, tkn);
    return node;
}

/* ParseExpr2 - handle the AND operator */
static ParseTreeNode *ParseExpr2(ParseContext *c)
{
    ParseTreeNode *node;
    Token tkn;
    node = ParseExpr3(c);
    if ((tkn = GetToken(c)) == T_AND) {
        ParseTreeNode *node2 = NewParseTreeNode(c, NodeTypeConjunction);
        ExprListEntry *entry, **pLast;
        node2->type = &c->integerType;
        node2->u.exprList.exprs = entry = (ExprListEntry *)LocalAlloc(c, sizeof(ExprListEntry));
        entry->expr = node;
        entry->next = NULL;
        pLast = &entry->next;
        do {
            entry = (ExprListEntry *)LocalAlloc(c, sizeof(ExprListEntry));
            entry->expr = ParseExpr2(c);
            entry->next = NULL;
            *pLast = entry;
            pLast = &entry->next;
        } while ((tkn = GetToken(c)) == T_AND);
        node = node2;
    }
    SaveToken(c, tkn);
    return node;
}

/* ParseExpr3 - handle the BXOR operator */
static ParseTreeNode *ParseExpr3(ParseContext *c)
{
    ParseTreeNode *expr;
    Token tkn;
    expr = ParseExpr4(c);
    while ((tkn = GetToken(c)) == '^')
        expr = MakeBinaryOpNode(c, OP_BXOR, expr, ParseExpr4(c));
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr4 - handle the BOR operator */
static ParseTreeNode *ParseExpr4(ParseContext *c)
{
    ParseTreeNode *expr;
    Token tkn;
    expr = ParseExpr5(c);
    while ((tkn = GetToken(c)) == '|')
        expr = MakeBinaryOpNode(c, OP_BOR, expr, ParseExpr5(c));
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr5 - handle the BAND operator */
static ParseTreeNode *ParseExpr5(ParseContext *c)
{
    ParseTreeNode *expr;
    Token tkn;
    expr = ParseExpr6(c);
    while ((tkn = GetToken(c)) == '&')
        expr = MakeBinaryOpNode(c, OP_BAND, expr, ParseExpr6(c));
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr6 - handle the '=' and '<>' operators */
static ParseTreeNode *ParseExpr6(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    Token tkn;
    expr = ParseExpr7(c);
    while ((tkn = GetToken(c)) == '=' || tkn == T_NE) {
        int op;
        expr2 = ParseExpr7(c);
        switch (tkn) {
        case '=':
            op = OP_EQ;
            break;
        case T_NE:
            op = OP_NE;
            break;
        default:
            /* not reached */
            op = 0;
            break;
        }
        expr = MakeBinaryOpNode(c, op, expr, expr2);
    }
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr7 - handle the '<', '<=', '>=' and '>' operators */
static ParseTreeNode *ParseExpr7(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    Token tkn;
    expr = ParseExpr8(c);
    while ((tkn = GetToken(c)) == '<' || tkn == T_LE || tkn == T_GE || tkn == '>') {
        int op;
        expr2 = ParseExpr8(c);
        switch (tkn) {
        case '<':
            op = OP_LT;
            break;
        case T_LE:
            op = OP_LE;
            break;
        case T_GE:
            op = OP_GE;
            break;
        case '>':
            op = OP_GT;
            break;
        default:
            /* not reached */
            op = 0;
            break;
        }
        expr = MakeBinaryOpNode(c, op, expr, expr2);
    }
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr8 - handle the '<<' and '>>' operators */
static ParseTreeNode *ParseExpr8(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    Token tkn;
    expr = ParseExpr9(c);
    while ((tkn = GetToken(c)) == T_SHL || tkn == T_SHR) {
        int op;
        expr2 = ParseExpr9(c);
        switch (tkn) {
        case T_SHL:
            op = OP_SHL;
            break;
        case T_SHR:
            op = OP_SHR;
            break;
        default:
            /* not reached */
            op = 0;
            break;
        }
        expr = MakeBinaryOpNode(c, op, expr, expr2);
    }
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr9 - handle the '+' and '-' operators */
static ParseTreeNode *ParseExpr9(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    Token tkn;
    expr = ParseExpr10(c);
    while ((tkn = GetToken(c)) == '+' || tkn == '-') {
        int op;
        expr2 = ParseExpr10(c);
        switch (tkn) {
        case '+':
            op = OP_ADD;
            break;
        case '-':
            op = OP_SUB;
            break;
        default:
            /* not reached */
            op = 0;
            break;
        }
        expr = MakeBinaryOpNode(c, op, expr, expr2);
    }
    SaveToken(c, tkn);
    return expr;
}

/* ParseExpr10 - handle the '*', '/' and MOD operators */
static ParseTreeNode *ParseExpr10(ParseContext *c)
{
    ParseTreeNode *node, *node2;
    Token tkn;
    node = ParseExpr11(c);
    while ((tkn = GetToken(c)) == '*' || tkn == '/' || tkn == T_MOD) {
        int op;
        node2 = ParseExpr11(c);
        switch (tkn) {
        case '*':
            op = OP_MUL;
            break;
        case '/':
            op = OP_DIV;
            break;
        case T_MOD:
            op = OP_REM;
            break;
        default:
            /* not reached */
            op = 0;
            break;
        }
        node = MakeBinaryOpNode(c, op, node, node2);
    }
    SaveToken(c, tkn);
    return node;
}

/* ParseExpr11 - handle unary operators */
static ParseTreeNode *ParseExpr11(ParseContext *c)
{
    ParseTreeNode *node;
    Token tkn;
    switch (tkn = GetToken(c)) {
    case '+':
        node = ParsePrimary(c);
        break;
    case '-':
        node = MakeUnaryOpNode(c, OP_NEG, ParsePrimary(c));
        break;
    case T_NOT:
        node = MakeUnaryOpNode(c, OP_NOT, ParsePrimary(c));
        break;
    case '~':
        node = MakeUnaryOpNode(c, OP_BNOT, ParsePrimary(c));
        break;
    case '@':
        node = NewParseTreeNode(c, NodeTypeAddressOf);
        node->u.addressOf.expr = ParsePrimary(c);
        node->type = &c->integerType;
        break;
    default:
        SaveToken(c,tkn);
        node = ParsePrimary(c);
        break;
    }
    return node;
}

/* ParsePrimary - parse function calls and array references */
ParseTreeNode *ParsePrimary(ParseContext *c)
{
    ParseTreeNode *node;
    int tkn;
    node = ParseSimplePrimary(c);
    switch (node->type->id) {
    case TYPE_FUNCTION:
        node = ParseCall(c, node);
        break;
    case TYPE_ARRAY:
    case TYPE_POINTER:
        if ((tkn = GetToken(c)) == '(')
            node = ParseArrayReference(c, node);
        else
            SaveToken(c, tkn);
        break;
    }
    return node;
}

/* ParseArrayReference - parse an array reference */
static ParseTreeNode *ParseArrayReference(ParseContext *c, ParseTreeNode *arrayNode)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeArrayRef);
    node->type = arrayNode->type->u.arrayInfo.elementType;
    node->u.arrayRef.array = arrayNode;
    node->u.arrayRef.index = ParseExpr(c);
    FRequire(c, ')');
    return node;
}

/* ParseCall - parse a function or subroutine call */
static ParseTreeNode *ParseCall(ParseContext *c, ParseTreeNode *functionNode)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeFunctionCall);
    Symbol *arg;
    Token tkn;

    /* intialize the function call node */
    node->type = functionNode->type->u.functionInfo.returnType;
    node->u.functionCall.fcn = functionNode;
    node->u.functionCall.args = NULL;
    arg = functionNode->type->u.functionInfo.arguments.head;
    
    /* parse the optional argument list */
    if ((tkn = GetToken(c)) == '(') {
        do {
            ExprListEntry *actual;

            /* allocate an actual argument structure and push it onto the list of arguments */
            actual = (ExprListEntry *)LocalAlloc(c, sizeof(ExprListEntry));
            actual->expr = ParseExpr(c);
            actual->next = node->u.functionCall.args;
            node->u.functionCall.args = actual;

            /* check the argument count and type */
            if (!arg)
                ParseError(c, "too many arguments");
            else if (!CompareTypes(actual->expr->type, arg->type))
                ParseError(c, "wrong argument type");

            /* move ahead to the next argument */
            ++node->u.functionCall.argc;
            arg = arg->next;
        } while ((tkn = GetToken(c)) == ',');
        Require(c, tkn, ')');
    }

    /* no argument list */
    else
        SaveToken(c, tkn);

    /* make sure there werent' too many arguments specified */
    if (arg)
        ParseError(c, "too few arguments");

    /* return the function call node */
    return node;
}

/* ParseSimplePrimary - parse a primary expression */
static ParseTreeNode *ParseSimplePrimary(ParseContext *c)
{
    ParseTreeNode *node;
    Token tkn;
    switch (tkn = GetToken(c)) {
    case '(':
        node = ParseExpr(c);
        FRequire(c,')');
        break;
    case T_NUMBER:
        node = NewParseTreeNode(c, NodeTypeIntegerLit);
        node->type = &c->integerType;
        node->u.integerLit.value = c->value;
        break;
    case T_STRING:
        node = NewParseTreeNode(c, NodeTypeStringLit);
        node->type = &c->byteArrayType;
        node->u.stringLit.string = AddString(c, c->token);
        break;
    case T_IDENTIFIER:
        node = GetSymbolRef(c, c->token);
        break;
    default:
        ParseError(c, "Expecting a primary expression");
        node = NULL; /* not reached */
        break;
    }
    return node;
}

/* GetSymbolRef - setup a symbol reference */
ParseTreeNode *GetSymbolRef(ParseContext *c, char *name)
{
    ParseTreeNode *node;
    Symbol *symbol;

    /* handle local variables within a function */
    if (c->codeType && (symbol = FindSymbol(&c->locals, name)) != NULL) {
        node = NewParseTreeNode(c, NodeTypeLocalRef);
        node->type = symbol->type;
        node->u.localRef.symbol = symbol;
        node->u.localRef.fcn = code_local;
        node->u.localRef.offset = symbol->v.variable.offset;
    }

    /* handle function arguments */
    else if (c->codeType && (symbol = FindSymbol(&c->codeType->u.functionInfo.arguments, name)) != NULL) {
        node = NewParseTreeNode(c, NodeTypeLocalRef);
        node->type = symbol->type;
        node->u.localRef.symbol = symbol;
        node->u.localRef.fcn = code_local;
        node->u.localRef.offset = symbol->v.variable.offset;
    }

    /* handle global symbols */
    else if ((symbol = FindSymbol(&c->globals, c->token)) != NULL) {
        if (IsConstant(symbol)) {
            switch (symbol->type->id) {
            case TYPE_STRING:
                node = NewParseTreeNode(c, NodeTypeStringLit);
                node->type = &c->byteArrayType;
                node->u.stringLit.string = symbol->v.string;
                break;
            case TYPE_FUNCTION:
                node = NewParseTreeNode(c, NodeTypeFunctionLit);
                node->type = symbol->type;
                node->u.functionLit.symbol = symbol;
                break;
            case TYPE_ARRAY:
                node = NewParseTreeNode(c, NodeTypeArrayLit);
                node->type = ArrayTypeToPointerType(c, symbol->type);
                node->u.arrayLit.symbol = symbol;
                break;
            case TYPE_INTEGER:
                node = NewParseTreeNode(c, NodeTypeIntegerLit);
                node->type = symbol->type;
                node->u.integerLit.value = symbol->v.value;
                break;
            default:
                ParseError(c, "unknown symbol type");
                break;
            }
        }
        else {
            node = NewParseTreeNode(c, NodeTypeGlobalRef);
            node->type = symbol->type;
            node->u.globalRef.symbol = symbol;
            node->u.globalRef.fcn = code_global;
        }
    }

    /* handle undefined symbols */
    else {
        node = NewParseTreeNode(c, NodeTypeGlobalRef);
        if (c->pass == 1)
            node->type = &c->integerType;
        else {
            VMVALUE value = 0;
            symbol = AddGlobalSymbol(c, name, SC_GLOBAL, &c->integerType, c->dataTarget);
            node->type = symbol->type;
            node->u.globalRef.symbol = symbol;
            node->u.globalRef.fcn = code_global;
            c->dataTarget->offset += WriteSection(c, c->dataTarget, (uint8_t *)&value, sizeof(VMVALUE));
        }
    }

    /* return the symbol reference node */
    return node;
}

/* MakeUnaryOpNode - allocate a unary operation parse tree node */
static ParseTreeNode *MakeUnaryOpNode(ParseContext *c, int op, ParseTreeNode *expr)
{
    ParseTreeNode *node;
    if (IsIntegerLit(expr)) {
        node = expr;
        switch (op) {
        case OP_NEG:
            node->u.integerLit.value = -expr->u.integerLit.value;
            break;
        case OP_NOT:
            node->u.integerLit.value = !expr->u.integerLit.value;
            break;
        case OP_BNOT:
            node->u.integerLit.value = ~expr->u.integerLit.value;
            break;
        }
    }
    else if (expr->type->id == TYPE_INTEGER) {
        node = NewParseTreeNode(c, NodeTypeUnaryOp);
        node->type = &c->integerType;
        node->u.unaryOp.op = op;
        node->u.unaryOp.expr = expr;
    }
    else {
        ParseError(c, "Expecting a numeric expression");
        node = NULL; /* not reached */
    }
    return node;
}

/* MakeBinaryOpNode - allocate a binary operation parse tree node */
static ParseTreeNode *MakeBinaryOpNode(ParseContext *c, int op, ParseTreeNode *left, ParseTreeNode *right)
{
    ParseTreeNode *node;
    if (IsIntegerLit(left) && IsIntegerLit(right)) {
        node = left;
        switch (op) {
        case OP_BXOR:
            node->u.integerLit.value = left->u.integerLit.value ^ right->u.integerLit.value;
            break;
        case OP_BOR:
            node->u.integerLit.value = left->u.integerLit.value | right->u.integerLit.value;
            break;
        case OP_BAND:
            node->u.integerLit.value = left->u.integerLit.value & right->u.integerLit.value;
            break;
        case OP_EQ:
            node->u.integerLit.value = left->u.integerLit.value == right->u.integerLit.value;
            break;
        case OP_NE:
            node->u.integerLit.value = left->u.integerLit.value != right->u.integerLit.value;
            break;
        case OP_LT:
            node->u.integerLit.value = left->u.integerLit.value < right->u.integerLit.value;
            break;
        case OP_LE:
            node->u.integerLit.value = left->u.integerLit.value <= right->u.integerLit.value;
            break;
        case OP_GE:
            node->u.integerLit.value = left->u.integerLit.value >= right->u.integerLit.value;
            break;
        case OP_GT:
            node->u.integerLit.value = left->u.integerLit.value > right->u.integerLit.value;
            break;
        case OP_SHL:
            node->u.integerLit.value = left->u.integerLit.value << right->u.integerLit.value;
            break;
        case OP_SHR:
            node->u.integerLit.value = left->u.integerLit.value >> right->u.integerLit.value;
            break;
        case OP_ADD:
            node->u.integerLit.value = left->u.integerLit.value + right->u.integerLit.value;
            break;
        case OP_SUB:
            node->u.integerLit.value = left->u.integerLit.value - right->u.integerLit.value;
            break;
        case OP_MUL:
            node->u.integerLit.value = left->u.integerLit.value * right->u.integerLit.value;
            break;
        case OP_DIV:
            if (right->u.integerLit.value == 0)
                ParseError(c, "division by zero in constant expression");
            node->u.integerLit.value = left->u.integerLit.value / right->u.integerLit.value;
            break;
        case OP_REM:
            if (right->u.integerLit.value == 0)
                ParseError(c, "division by zero in constant expression");
            node->u.integerLit.value = left->u.integerLit.value % right->u.integerLit.value;
            break;
        default:
            goto integerOp;
        }
    }
    else if (IsIntegerType(left->type) && IsIntegerType(right->type)) {
integerOp:
        node = NewParseTreeNode(c, NodeTypeBinaryOp);
        node->type = &c->integerType;
        node->u.binaryOp.op = op;
        node->u.binaryOp.left = left;
        node->u.binaryOp.right = right;
    }
    else {
        ParseError(c, "Expecting a numeric expression");
        node = NULL; /* not reached */
    }
    return node;
}

/* NewParseTreeNode - allocate a new parse tree node */
ParseTreeNode *NewParseTreeNode(ParseContext *c, int type)
{
    ParseTreeNode *node = (ParseTreeNode *)LocalAlloc(c, sizeof(ParseTreeNode));
    memset(node, 0, sizeof(ParseTreeNode));
    node->nodeType = type;
    return node;
}

/* IsIntegerLit - check to see if a node is an integer literal */
int IsIntegerLit(ParseTreeNode *node)
{
    return node->nodeType == NodeTypeIntegerLit;
}

/* IsStringLit - check to see if a node is a string literal */
int IsStringLit(ParseTreeNode *node)
{
    return node->nodeType == NodeTypeStringLit;
}
    
    
