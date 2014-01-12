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

/* ParseExpr - handle the OR operator */
ParseTreeNode *ParseExpr(ParseContext *c)
{
    ParseTreeNode *node;
    int tkn;
    node = ParseExpr2(c);
    if ((tkn = GetToken(c)) == T_OR) {
        ParseTreeNode *node2 = NewParseTreeNode(c, NodeTypeDisjunction);
        NodeListEntry *entry, **pLast;
        node2->type = &c->integerType;
        node2->u.exprList.exprs = entry = (NodeListEntry *)xbLocalAlloc(c->sys, sizeof(NodeListEntry));
        entry->node = node;
        entry->next = NULL;
        pLast = &entry->next;
        do {
            entry = (NodeListEntry *)xbLocalAlloc(c->sys, sizeof(NodeListEntry));
            entry->node = ParseExpr2(c);
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
    int tkn;
    node = ParseExpr3(c);
    if ((tkn = GetToken(c)) == T_AND) {
        ParseTreeNode *node2 = NewParseTreeNode(c, NodeTypeConjunction);
        NodeListEntry *entry, **pLast;
        node2->type = &c->integerType;
        node2->u.exprList.exprs = entry = (NodeListEntry *)xbLocalAlloc(c->sys, sizeof(NodeListEntry));
        entry->node = node;
        entry->next = NULL;
        pLast = &entry->next;
        do {
            entry = (NodeListEntry *)xbLocalAlloc(c->sys, sizeof(NodeListEntry));
            entry->node = ParseExpr2(c);
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
    int tkn;
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
    int tkn;
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
    int tkn;
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
    int tkn;
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
    int tkn;
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
    int tkn;
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
    int tkn;
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
    int tkn;
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
    int tkn;
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
    default:
        // nothing to do
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
    int tkn;

    /* intialize the function call node */
    node->type = functionNode->type->u.functionInfo.returnType;
    node->u.functionCall.fcn = functionNode;
    node->u.functionCall.args = NULL;
    arg = functionNode->type->u.functionInfo.arguments.head;
    
    /* parse the optional argument list */
    if ((tkn = GetToken(c)) == '(') {
        do {
            NodeListEntry *actual;

            /* allocate an actual argument structure and push it onto the list of arguments */
            actual = (NodeListEntry *)xbLocalAlloc(c->sys, sizeof(NodeListEntry));
            actual->node = ParseExpr(c);
            actual->next = node->u.functionCall.args;
            node->u.functionCall.args = actual;

            /* check the argument count and type */
            if (!arg)
                ParseError(c, "too many arguments");
            else if (!CompareTypes(actual->node->type, arg->type))
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
    int tkn;
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
    if (c->function && (symbol = FindSymbol(&c->function->u.functionDefinition.locals, name)) != NULL) {
        node = NewParseTreeNode(c, NodeTypeLocalRef);
        node->type = symbol->type;
        node->u.localRef.offset = symbol->v.variable.offset;
    }

    /* handle function arguments */
    else if (c->functionType && (symbol = FindSymbol(&c->functionType->u.functionInfo.arguments, name)) != NULL) {
        node = NewParseTreeNode(c, NodeTypeLocalRef);
        node->type = symbol->type;
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
                AddDependency(c, symbol);
                break;
            case TYPE_ARRAY:
                node = NewParseTreeNode(c, NodeTypeArrayLit);
                node->type = ArrayTypeToPointerType(c, symbol->type);
                node->u.arrayLit.symbol = symbol;
                AddDependency(c, symbol);
                break;
            case TYPE_INTEGER:
                node = NewParseTreeNode(c, NodeTypeIntegerLit);
                node->type = symbol->type;
                node->u.integerLit.value = symbol->v.value;
                break;
            default:
                ParseError(c, "unknown symbol type");
                node = NULL; // not reached
                break;
            }
        }
        else {
            node = NewParseTreeNode(c, NodeTypeGlobalRef);
            node->type = symbol->type;
            node->u.globalRef.symbol = symbol;
            AddDependency(c, symbol);
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
            AddDependency(c, symbol);
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
    ParseTreeNode *node = (ParseTreeNode *)xbLocalAlloc(c->sys, sizeof(ParseTreeNode));
    memset(node, 0, sizeof(ParseTreeNode));
    node->nodeType = type;
    return node;
}

/* AddNodeToList - add a node to a parse tree node list */
void AddNodeToList(ParseContext *c, NodeListEntry ***ppNextEntry, ParseTreeNode *node)
{
    NodeListEntry *entry = (NodeListEntry *)xbLocalAlloc(c->sys, sizeof(NodeListEntry));
    entry->node = node;
    entry->next = NULL;
    **ppNextEntry = entry;
    *ppNextEntry = &entry->next;
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

static void PrintNodeList(NodeListEntry *entry, int indent);
static void PrintCaseList(CaseListEntry *entry, int indent);

void PrintNode(ParseTreeNode *node, int indent)
{
	printf("%*s", indent, "");
    switch (node->nodeType) {
    case NodeTypeFunctionDefinition:
        printf("FunctionDefinition: %s\n", node->type ? node->u.functionDefinition.symbol->name : "<main>");
        PrintNodeList(node->u.functionDefinition.bodyStatements, indent + 2);
        break;
    case NodeTypeLetStatement:
        printf("Let\n");
        printf("%*slvalue\n", indent + 2, "");
        PrintNode(node->u.letStatement.lvalue, indent + 4);
        printf("%*srvalue\n", indent + 2, "");
        PrintNode(node->u.letStatement.rvalue, indent + 4);
        break;
    case NodeTypeIfStatement:
        printf("If\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.ifStatement.test, indent + 4);
        printf("%*sthen\n", indent + 2, "");
        PrintNodeList(node->u.ifStatement.thenStatements, indent + 4);
        if (node->u.ifStatement.elseStatements) {
            printf("%*selse\n", indent + 2, "");
            PrintNodeList(node->u.ifStatement.elseStatements, indent + 4);
        }
        break;
    case NodeTypeSelectStatement:
        printf("Select\n");
        printf("%*sexpr\n", indent + 2, "");
        PrintNode(node->u.selectStatement.expr, indent + 4);
        PrintNodeList(node->u.selectStatement.caseStatements, indent + 2);
        if (node->u.selectStatement.elseStatements) {
            printf("%*sElse\n", indent + 2, "");
            PrintNodeList(node->u.selectStatement.elseStatements->u.caseStatement.bodyStatements, indent + 4);
        }
        break;
    case NodeTypeCaseStatement:
        printf("Case\n");
        printf("%*scases\n", indent + 2, "");
        PrintCaseList(node->u.caseStatement.cases, indent + 4);
        PrintNodeList(node->u.caseStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeForStatement:
        printf("For\n");
        printf("%*svar\n", indent + 2, "");
        PrintNode(node->u.forStatement.var, indent + 4);
        printf("%*sstart\n", indent + 2, "");
        PrintNode(node->u.forStatement.startExpr, indent + 4);
        printf("%*send\n", indent + 2, "");
        PrintNode(node->u.forStatement.endExpr, indent + 4);
        if (node->u.forStatement.stepExpr) {
            printf("%*sstep\n", indent + 2, "");
            PrintNode(node->u.forStatement.stepExpr, indent + 4);
        }
        PrintNodeList(node->u.forStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeDoWhileStatement:
        printf("DoWhile\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.loopStatement.test, indent + 4);
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeDoUntilStatement:
        printf("DoUntil\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.loopStatement.test, indent + 4);
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeLoopStatement:
        printf("Loop\n");
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeLoopWhileStatement:
        printf("LoopWhile\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.loopStatement.test, indent + 4);
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeLoopUntilStatement:
        printf("LoopUntil\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.loopStatement.test, indent + 4);
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeReturnStatement:
        printf("Return\n");
        if (node->u.returnStatement.expr) {
            printf("%*sexpr\n", indent + 2, "");
            PrintNode(node->u.returnStatement.expr, indent + 4);
        }
        break;
    case NodeTypeCallStatement:
        printf("CallStatement\n");
        printf("%*sexpr\n", indent + 2, "");
        PrintNode(node->u.callStatement.expr, indent + 4);
        break;
    case NodeTypeLabelDefinition:
        printf("Label: %s\n", node->u.labelDefinition.label->name);
        break;
    case NodeTypeGotoStatement:
        printf("Goto: %s\n", node->u.gotoStatement.label->name);
        break;
    case NodeTypeEndStatement:
        printf("Return\n");
        break;
    case NodeTypeAsmStatement:
        printf("Asm\n");
        break;
    case NodeTypeGlobalRef:
        printf("GlobalRef: %s\n", node->u.globalRef.symbol->name);
        break;
    case NodeTypeLocalRef:
        printf("LocalRef: %d\n", node->u.localRef.offset);
        break;
    case NodeTypeFunctionLit:
        printf("FunctionLit: %s\n", node->u.functionLit.symbol->name);
        break;
    case NodeTypeArrayLit:
        printf("ArrayLit: %s\n", node->u.arrayLit.symbol->name);
        break;
    case NodeTypeStringLit:
		printf("StringLit: '%s'\n",node->u.stringLit.string->value);
        break;
    case NodeTypeIntegerLit:
		printf("IntegerLit: %d\n",node->u.integerLit.value);
        break;
    case NodeTypeUnaryOp:
        printf("UnaryOp: %d\n", node->u.unaryOp.op);
        printf("%*sexpr\n", indent + 2, "");
        PrintNode(node->u.unaryOp.expr, indent + 4);
        break;
    case NodeTypeBinaryOp:
        printf("BinaryOp: %d\n", node->u.binaryOp.op);
        printf("%*sleft\n", indent + 2, "");
        PrintNode(node->u.binaryOp.left, indent + 4);
        printf("%*sright\n", indent + 2, "");
        PrintNode(node->u.binaryOp.right, indent + 4);
        break;
    case NodeTypeArrayRef:
        printf("ArrayRef\n");
        printf("%*sarray\n", indent + 2, "");
        PrintNode(node->u.arrayRef.array, indent + 4);
        printf("%*sindex\n", indent + 2, "");
        PrintNode(node->u.arrayRef.index, indent + 4);
        break;
    case NodeTypeFunctionCall:
        printf("FunctionCall: %d\n", node->u.functionCall.argc);
        printf("%*sfcn\n", indent + 2, "");
        PrintNode(node->u.functionCall.fcn, indent + 4);
        PrintNodeList(node->u.functionCall.args, indent + 2);
        break;
    case NodeTypeDisjunction:
        printf("Disjunction\n");
        PrintNodeList(node->u.exprList.exprs, indent + 2);
        break;
    case NodeTypeConjunction:
        printf("Conjunction\n");
        PrintNodeList(node->u.exprList.exprs, indent + 2);
        break;
    case NodeTypeAddressOf:
        printf("AddressOf\n");
        printf("%*sexpr\n", indent + 2, "");
        PrintNode(node->u.addressOf.expr, indent + 4);
        break;
    default:
        printf("<unknown node type: %d>\n", node->nodeType);
        break;
    }
}

static void PrintNodeList(NodeListEntry *entry, int indent)
{
    while (entry != NULL) {
        PrintNode(entry->node, indent);
        entry = entry->next;
    }
}

static void PrintCaseList(CaseListEntry *entry, int indent)
{
    while (entry != NULL) {
        if (!entry->fromExpr)
            printf("%*selse\n", indent, "");
        else if (!entry->toExpr) {
            printf("%*sexpr\n", indent, "");
            PrintNode(entry->fromExpr, indent + 2);
        }
        else {
            printf("%*srange\n", indent, "");
            printf("%*sfrom\n", indent + 2, "");
            PrintNode(entry->fromExpr, indent + 4);
            printf("%*sto\n", indent + 2, "");
            PrintNode(entry->toExpr, indent + 4);
        }
        entry = entry->next;
    }
}

