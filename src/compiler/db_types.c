/* db_types.c - type functions
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "db_compiler.h"

/* prototypes */
static VMUVALUE ValueByteSize(Type *type);

/* NewGlobalType - allocate a new global type */
Type *NewGlobalType(ParseContext *c, TypeID id)
{
    Type *type = (Type *)GlobalAlloc(c, sizeof(Type));
    type->id = id;
    return type;
}

/* ArrayTypeToPointerType - convert an array type to a pointer type */
Type *ArrayTypeToPointerType(ParseContext *c, Type *type)
{
    Type *pointerType;
    switch (type->u.arrayInfo.elementType->id) {
    case TYPE_INTEGER:
        pointerType = &c->integerPointerType;
        break;
    case TYPE_BYTE:
        pointerType = &c->bytePointerType;
        break;
    default:
        ParseError(c, "Internal error");
        pointerType = NULL; // never reached
        break;
    }
    return pointerType;
}

/* RValueType - get the r-value type of an expression */
static int RValueType(Type *type)
{
    int id;
    switch (type->id) {
    case TYPE_ARRAY:
        id = type->u.arrayInfo.elementType->id;
        break;
    case TYPE_POINTER:
        id = type->u.pointerInfo.targetType->id;
        break;
    default:
        id = type->id;
        break;
    }
    return id;
}

/* CompareTypes - compare two types to see if they are equivilent */
int CompareTypes(Type *type1, Type *type2)
{
    int match;

    /* make sure the type ids match */
    if (RValueType(type1) == RValueType(type2)) {
        switch (type1->id) {
        case TYPE_ARRAY:
        case TYPE_POINTER:
            match = CompareTypes(type1->u.arrayInfo.elementType, type2->u.arrayInfo.elementType);
            break;
        default:
            match = VMTRUE;
            break;
        }
    }

    /* check for compatible integer types */
    else if (IsIntegerType(type1) && IsIntegerType(type2))
         match = VMTRUE;
         
    /* no type id match */
    else
        match = VMFALSE;

    return match;
}

/* ValueSize - determine the size of a variable in words */
VMUVALUE ValueSize(Type *type, VMUVALUE size)
{
    int valueSize, sizeInBytes;

    switch (type->id) {
    case TYPE_INTEGER:
    case TYPE_STRING:
    case TYPE_POINTER:
        valueSize = 1;
        break;
    case TYPE_ARRAY:
        sizeInBytes = size * ValueByteSize(type->u.arrayInfo.elementType);
        valueSize = ROUND_TO_WORDS(sizeInBytes) / sizeof(VMVALUE);
        break;
    default:
        break;
    }

    return valueSize;
}

/* ValueByteSize - determine the size of a scalar type in bytes */
static VMUVALUE ValueByteSize(Type *type)
{
    int size;

    switch (type->id) {
    case TYPE_INTEGER:
    case TYPE_STRING:
    case TYPE_POINTER:
        size = sizeof(VMVALUE);
        break;
    case TYPE_BYTE:
        size = 1;
        break;
    default:
        break;
    }

    return size;
}

/* IsIntegerType - verify that an expression has an integer type (integer or byte) */
int IsIntegerType(Type *type)
{
    return type->id == TYPE_INTEGER || type->id == TYPE_BYTE;
}

