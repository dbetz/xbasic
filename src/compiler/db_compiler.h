/* db_compiler.h - definitions for a simple basic compiler
 *
 * Copyright (c) 2009 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __DB_COMPILER_H__
#define __DB_COMPILER_H__

#include <stdio.h>
#include <setjmp.h>
#include "db_config.h"
#include "db_image.h"
#include "db_system.h"
#include "xb_api.h"

#ifdef WIN32
#define strcasecmp  _stricmp
#endif

/* program limits */
#define MAXLINE             128
#define MAXTOKEN            32
#define DEFAULT_STACK_SIZE  (64 * sizeof(VMVALUE))

/* forward type declarations */
typedef struct Type Type;
typedef struct SymbolTable SymbolTable;
typedef struct Symbol Symbol;
typedef struct IncludedFile IncludedFile;
typedef struct Dependency Dependency;
typedef struct String String;
typedef struct ParseTreeNode ParseTreeNode;
typedef struct NodeListEntry NodeListEntry;
typedef struct CaseListEntry CaseListEntry;

/* lexical tokens */
enum {
    T_NONE,
    T_REM = 0x100,  /* keywords start here */
    T_INCLUDE,
    T_OPTION,
    T_DEF,
    T_DIM,
    T_AS,
    T_IN,
    T_LET,
    T_IF,
    T_THEN,
    T_ELSE,
    T_SELECT,
    T_CASE,
    T_END,
    T_FOR,
    T_TO,
    T_STEP,
    T_NEXT,
    T_DO,
    T_WHILE,
    T_UNTIL,
    T_LOOP,
    T_GOTO,
    T_MOD,
    T_AND,
    T_OR,
    T_XOR,
    T_NOT,
    T_STOP,
    T_RETURN,
    T_INPUT,
    T_PRINT,
    T_ASM,
    T_ELSE_IF,  /* compound keywords */
    T_END_DEF,
    T_END_IF,
    T_END_SELECT,
    T_END_ASM,
    T_DO_WHILE,
    T_DO_UNTIL,
    T_LOOP_WHILE,
    T_LOOP_UNTIL,
    T_LE,       /* non-keyword tokens */
    T_NE,
    T_GE,
    T_SHL,
    T_SHR,
    T_IDENTIFIER,
    T_NUMBER,
    T_STRING,
    T_EOL,
    T_EOF
};

typedef enum {
    BLOCK_FUNCTION,
    BLOCK_IF,
    BLOCK_ELSE,
    BLOCK_SELECT,
    BLOCK_CASE,
    BLOCK_FOR,
    BLOCK_DO
} BlockType;

typedef struct {
    BlockType type;
    ParseTreeNode *node;
    NodeListEntry **pNextStatement;
} Block;

typedef enum {
    GEN_BLOCK_SELECT
} GenBlockType;

typedef struct {
    GenBlockType type;
    union {
        struct {
            int first;
            VMUVALUE nxt;
            VMUVALUE end;
        } selectBlock;
    } u;
} GenBlock;

struct String {
    String *next;
    int placed;
    VMUVALUE offset;
    uint8_t value[1];
};

/* label states */
typedef enum {
    LS_UNDEFINED,
    LS_DEFINED,
    LS_PLACED
} LabelState;

typedef struct Label Label;
struct Label {
    Label *next;
    LabelState state;
    VMUVALUE offset;
    VMUVALUE fixups;
    char name[1];
};

/* storage class ids */
typedef enum {
    SC_CONSTANT,
    SC_LOCAL,
    SC_GLOBAL,
    SC_HUB,
    SC_COG
} StorageClass;

#define UNDEF_VALUE 0xffffffff

/* symbol table */
struct SymbolTable {
    Symbol *head;
    Symbol **pTail;
    int count;
};

/* symbol structure */
struct Symbol {
    Symbol *prev;
    Symbol *next;
    StorageClass storageClass;
    Section *section;
    Type *type;
    union {
        struct {
            VMUVALUE offset;
            VMUVALUE fixups;
        } variable;
        VMVALUE value;
        String *string;
    } v;
    char name[1];
};

/* types */
typedef enum {
    TYPE_INTEGER,
    TYPE_BYTE,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_FUNCTION
} TypeID;

/* type definition */
struct Type {
    TypeID  id;
    union {
        struct {
            Type *elementType;
        } arrayInfo;
        struct {
            Type *targetType;
        } pointerInfo;
        struct {
            Type *returnType;
            SymbolTable arguments;
            Dependency *dependencies;
        } functionInfo;
    } u;
};

/* local fixup structure */
typedef struct LocalFixup LocalFixup;
struct LocalFixup {
    LocalFixup *next;
    Symbol *symbol;
    VMUVALUE chain;
};

/* main code state */
typedef enum {
    MAIN_NOT_DEFINED,
    MAIN_IN_PROGRESS,
    MAIN_DEFINED
} MainState;

typedef void RewindFcn(void *cookie);
typedef int GetLineFcn(void *cookie, char *buf, int len);
#define GET_NULL ((GetLineFcn *)0)

/* main file */
typedef struct {
    RewindFcn *rewind;          /* function to rewind to the start of the source program */
    GetLineFcn *getLine;        /* function to get a line from the source program */
    void *getLineCookie;        /* cookie for the rewind and getLine functions */
} MainFile;

/* current include file */
typedef struct {
    IncludedFile *file;
    void *fp;
} CurrentIncludeFile;

/* parse file */
typedef struct ParseFile ParseFile;
struct ParseFile {
    ParseFile *next;            /* next file in stack */
    union {
        CurrentIncludeFile file;
        MainFile main;
    } u;
    int lineNumber;             /* current line number */
};

/* included file */
struct IncludedFile {
    IncludedFile *next;         /* next included file */
    char name[1];               /* file name */
};

/* dependency */
struct Dependency {
    Symbol *symbol;
    Dependency *next;
};

/* parse context */
typedef struct {
    jmp_buf errorTarget;            /* error target */
    System *sys;                    /* system interface */
    BoardConfig *config;            /* board configuration */
    int flags;                      /* compiler flags */
    ParseFile mainFile;             /* scan - main input file */
    ParseFile *currentFile;         /* scan - current input file */
    IncludedFile *includedFiles;    /* scan - list of files that have already been included */
    IncludedFile *currentInclude;   /* scan - file currently being included */
    char lineBuf[MAXLINE];          /* scan - line buffer */
    char *linePtr;                  /* scan - pointer to the current character */
    int savedToken;                 /* scan - lookahead token */
    int tokenOffset;                /* scan - offset to the start of the current token */
    char token[MAXTOKEN];           /* scan - current token string */
    VMVALUE value;                  /* scan - current token integer value */
    int inComment;                  /* scan - inside of a slash/star comment */
    Type stringType;                /* parse - string type */
    Type integerType;               /* parse - integer type */
    Type integerArrayType;          /* parse - integer array type */
    Type integerPointerType;        /* parse - integer pointer type */
    Type byteType;                  /* parse - byte type */
    Type byteArrayType;             /* parse - byte array type */
    Type bytePointerType;           /* parse - byte pointer type */
    SymbolTable globals;            /* parse - global variables and constants */
    String *strings;                /* parse - string constants */
    Type *functionType;             /* parse - in a function definition */
    ParseTreeNode *function;        /* parse - function currently being compiled */
    Dependency *dependencies;       /* parse - dependencies for the function currently being compiled */
    Dependency **pNextDependency;   /* parse - place to store the next dependency */
    MainState mainState;            /* parse - state of main code processing */
    VMUVALUE mainCode;              /* parse - main code offset into text space */
    Dependency *mainDependencies;   /* parse - main code dependencies */
    LocalFixup *symbolFixups;       /* parse - list of symbol fixups for the current code or data structure */
    Block blockBuf[10];             /* parse - stack of nested blocks */
    Block *bptr;                    /* parse - current block */
    Block *btop;                    /* parse - top of block stack */
    int stackSize;                  /* parse - interpreter stack size */
    int pass;                       /* parse - compiler pass in progress */
    GenBlock genBlockBuf[10];       /* generate - stack of nested generator blocks */
    GenBlock *gptr;                 /* generate - current generator block */
    GenBlock *gtop;                 /* generate - top of generator block stack */
    Section *textTarget;            /* generate - section where text will be placed */
    Section *dataTarget;            /* generate - section where data will be placed */
    uint8_t *cptr;                  /* generate - next available code staging buffer position */
    uint8_t *ctop;                  /* generate - top of code staging buffer */
    uint8_t *codeBuf;               /* generate - code staging buffer */
} ParseContext;

/* partial value */
typedef struct PVAL PVAL;

/* partial value function codes */
typedef enum {
    PV_LOAD,
    PV_STORE,
    PV_REFERENCE
} PValOp;

typedef void GenFcn(ParseContext *c, PValOp op, PVAL *pv);
#define GEN_NULL    ((GenFcn *)0)

/* partial value structure */
struct PVAL {
    Type *type;
    GenFcn *fcn;
    union {
        Symbol *sym;
        String *str;
        VMVALUE val;
    } u;
};

/* parse tree node types */
typedef enum {
    NodeTypeFunctionDefinition,
    NodeTypeLetStatement,
    NodeTypeIfStatement,
    NodeTypeSelectStatement,
    NodeTypeCaseStatement,
    NodeTypeForStatement,
    NodeTypeDoWhileStatement,
    NodeTypeDoUntilStatement,
    NodeTypeLoopStatement,
    NodeTypeLoopWhileStatement,
    NodeTypeLoopUntilStatement,
    NodeTypeReturnStatement,
    NodeTypeCallStatement,
    NodeTypeLabelDefinition,
    NodeTypeGotoStatement,
    NodeTypeEndStatement,
    NodeTypeAsmStatement,
    NodeTypeGlobalRef,
    NodeTypeLocalRef,
    NodeTypeFunctionLit,
    NodeTypeArrayLit,
    NodeTypeStringLit,
    NodeTypeIntegerLit,
    NodeTypeUnaryOp,
    NodeTypeBinaryOp,
    NodeTypeArrayRef,
    NodeTypeFunctionCall,
    NodeTypeDisjunction,
    NodeTypeConjunction,
    NodeTypeAddressOf
} NodeType;

/* parse tree node structure */
struct ParseTreeNode {
    NodeType nodeType;
    Type *type;
    union {
        struct {
            Symbol *symbol;
            SymbolTable locals;
            Label *labels;
            int localOffset;
            NodeListEntry *bodyStatements;
        } functionDefinition;
        struct {
            ParseTreeNode *lvalue;
            ParseTreeNode *rvalue;
        } letStatement;
        struct {
            ParseTreeNode *test;
            NodeListEntry *thenStatements;
            NodeListEntry *elseStatements;
        } ifStatement;
        struct {
            ParseTreeNode *expr;
            NodeListEntry *caseStatements;
            ParseTreeNode *elseStatements;
        } selectStatement;
        struct {
            CaseListEntry *cases;
            NodeListEntry *bodyStatements;
        } caseStatement;
        struct {
            ParseTreeNode *var;
            ParseTreeNode *startExpr;
            ParseTreeNode *endExpr;
            ParseTreeNode *stepExpr;
            NodeListEntry *bodyStatements;
        } forStatement;
        struct {
            ParseTreeNode *test;
            NodeListEntry *bodyStatements;
        } loopStatement;
        struct {
            ParseTreeNode *expr;
        } returnStatement;
        struct {
            ParseTreeNode *expr;
        } callStatement;
        struct {
            Label *label;
        } labelDefinition;
        struct {
            Label *label;
        } gotoStatement;
        struct {
            uint8_t *code;
            int length;
        } asmStatement;
        struct {
            Symbol *symbol;
        } globalRef;
        struct {
            int offset;
        } localRef;
        struct {
            Symbol *symbol;
        } arrayLit;
        struct {
            Symbol *symbol;
        } functionLit;
        struct {
            String *string;
        } stringLit;
        struct {
            VMVALUE value;
        } integerLit;
        struct {
            int op;
            ParseTreeNode *expr;
        } unaryOp;
        struct {
            int op;
            ParseTreeNode *left;
            ParseTreeNode *right;
        } binaryOp;
        struct {
            ParseTreeNode *array;
            ParseTreeNode *index;
        } arrayRef;
        struct {
            ParseTreeNode *fcn;
            NodeListEntry *args;
            int argc;
        } functionCall;
        struct {
            NodeListEntry *exprs;
        } exprList;
        struct {
            ParseTreeNode *expr;
        } addressOf;
    } u;
};

/* node list entry structure */
struct NodeListEntry {
    ParseTreeNode *node;
    NodeListEntry *next;
};

/* case list entry structure */
struct CaseListEntry {
    ParseTreeNode *fromExpr;
    ParseTreeNode *toExpr;
    CaseListEntry *next;
};

/* db_heap.c (currently in ibasic.c) */
void HeapInit(uint8_t *heap, size_t heapSize);
void HeapReset(void);
uint8_t *HeapAlloc(size_t size);

/* db_compiler.c */
ParseContext *InitCompiler(System *sys, BoardConfig *config, size_t codeBufSize);
int Compile(ParseContext *c, const char *name);
void StoreCode(ParseContext *c);
void AddIntrinsic(ParseContext *c, char *name, char *argTypes, char *retType, int index);
void AddRegister(ParseContext *c, char *name, VMUVALUE addr);
String *AddString(ParseContext *c, char *value);
VMUVALUE AddStringRef(ParseContext *c, String *str);
VMUVALUE AddLocalSymbolFixup(ParseContext *c, Symbol *symbol, VMUVALUE offset);
void Fatal(ParseContext *c, const char *fmt, ...);

/* db_statement.c */
void ParseStatement(ParseContext *c, int tkn);
void EndFunction(ParseContext *c);
void CheckLabels(ParseContext *c);
void DumpLabels(ParseContext *c);

/* db_expr.c */
ParseTreeNode *ParseExpr(ParseContext *c);
ParseTreeNode *ParsePrimary(ParseContext *c);
ParseTreeNode *GetSymbolRef(ParseContext *c, char *name);
ParseTreeNode *NewParseTreeNode(ParseContext *c, int type);
void AddNodeToList(ParseContext *c, NodeListEntry ***ppNextEntry, ParseTreeNode *node);
void PrintNode(ParseTreeNode *node, int indent);
int IsIntegerLit(ParseTreeNode *node);
int IsStringLit(ParseTreeNode *node);

/* db_scan.c */
void RewindInput(ParseContext *c);
int PushFile(ParseContext *c, const char *name);
void ClearIncludedFiles(ParseContext *c);
void CloseParseContext(ParseContext *c);
int GetLine(ParseContext *c);
void FRequire(ParseContext *c, int requiredToken);
void Require(ParseContext *c, int token, int requiredToken);
int GetToken(ParseContext *c);
void SaveToken(ParseContext *c, int token);
char *TokenName(int token);
int SkipSpaces(ParseContext *c);
int GetChar(ParseContext *c);
void UngetC(ParseContext *c);
int IdentifierCharP(int ch);
int NumberToken(ParseContext *c, int ch);
int HexNumberToken(ParseContext *c);
int BinaryNumberToken(ParseContext *c);
int StringToken(ParseContext *c);
int CharToken(ParseContext *c);
void *GlobalAlloc(ParseContext *c, size_t size);
void *LocalAlloc(ParseContext *c, size_t size);
void ParseError(ParseContext *c, char *fmt, ...);

/* db_symbols.c */
void InitSymbolTable(SymbolTable *table);
void AddDependency(ParseContext *c, Symbol *symbol);
Symbol *AddGlobalSymbol(ParseContext *c, const char *name, StorageClass storageClass, Type *type, Section *section);
Symbol *AddGlobalOffset(ParseContext *c, const char *name, StorageClass storageClass, Type *type, VMUVALUE offset);
Symbol *AddGlobalConstantInteger(ParseContext *c, const char *name, VMVALUE value);
Symbol *AddGlobalConstantString(ParseContext *c, const char *name, String *string);
Symbol *AddFormalArgument(ParseContext *c, SymbolTable *table, const char *name, Type *type, VMUVALUE offset);
Symbol *AddLocal(ParseContext *c, const char *name, Type *type, VMUVALUE value);
Symbol *FindSymbol(SymbolTable *table, const char *name);
int IsConstant(Symbol *symbol);
void DumpSymbols(ParseContext *c, SymbolTable *table, char *tag);

/* db_types.c */
Type *NewGlobalType(ParseContext *c, TypeID id);
Type *ArrayTypeToPointerType(ParseContext *c, Type *type);
int CompareTypes(Type *type1, Type *type2);
VMUVALUE ValueSize(Type *type, VMUVALUE size);
int IsIntegerType(Type *type);

/* db_generate.c */
void Generate(ParseContext *c, ParseTreeNode *expr);
void code_expr(ParseContext *c, ParseTreeNode *expr, PVAL *pv);
void code_global(ParseContext *c, PValOp fcn, PVAL *pv);
void code_local(ParseContext *c, PValOp fcn, PVAL *pv);
VMUVALUE codeaddr(ParseContext *c);
VMUVALUE putcbyte(ParseContext *c, int b);
VMUVALUE putcword(ParseContext *c, VMVALUE w);
VMVALUE rd_cword(ParseContext *c, VMUVALUE off);
void wr_cword(ParseContext *c, VMUVALUE off, VMVALUE w);
int merge(ParseContext *c, VMUVALUE chn, VMUVALUE chn2);
void fixup(ParseContext *c, VMUVALUE chn, VMUVALUE val);
void fixupbranch(ParseContext *c, VMUVALUE chn, VMUVALUE val);

/* db_wrimage.c */
int StartImage(ParseContext *c, const char *name);
int BuildImage(ParseContext *c, const char *name);
VMUVALUE WriteSection(ParseContext *c, Section *section, const uint8_t *buf, VMUVALUE size);
VMUVALUE ReadSectionOffset(ParseContext *c, Section *section, VMUVALUE offset);
void WriteSectionOffset(ParseContext *c, Section *section, VMUVALUE offset, VMUVALUE value);

#endif

