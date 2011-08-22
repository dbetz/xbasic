#ifndef __MEM_MALLOC_H__
#define __MEM_MALLOC_H__

#include "db_system.h"

System *MemInit(void);
void MemFree(System *sys);

#endif
