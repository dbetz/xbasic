#ifndef __DB_VMDEBUG_H__
#define __DB_VMDEBUG_H__

#include "db_config.h"
#include "db_system.h"

/* instruction output formats */
#define FMT_NONE        0
#define FMT_BYTE        1
#define FMT_SBYTE       2
#define FMT_WORD        3
#define FMT_BR          4

typedef struct {
    int code;
    char *name;
    int fmt;
} OTDEF;

extern FLASH_SPACE OTDEF OpcodeTable[];

void DecodeFunction(System *sys, VMUVALUE base, const uint8_t *code, int len);
int DecodeInstruction(System *sys, VMUVALUE addr, const uint8_t *lc);

#endif
