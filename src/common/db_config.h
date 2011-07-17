/* db_config.h - system configuration for a simple virtual machine
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __DB_CONFIG_H__
#define __DB_CONFIG_H__

#include <stdio.h>

/* configuration variables

RAMSIZE		the total amount of RAM available to xbasic
DATSIZE		the size of the heap used by the compiler
MAXCODE		the size of the bytecode staging buffer used by the compiler

*/

/* support for PIC24HJ128GP502 or dsPIC33FJ128GP802 */
#if defined(PIC24) || defined(dsPIC33)
#define WORD_SIZE_16
#define NO_STDINT
#define FAR_DATA		__attribute__((far))
#define FLASH_SPACE		const
#define RAMSIZE         (5 * 1024)
#define DATSIZE         1500
#define MAXCODE         (8 * 1024)

/* support for the Propeller under Catalina C */
#elif defined(PROPELLER_CAT)
#define WORD_SIZE_32
#define FAR_DATA
#define FLASH_SPACE		const
#ifdef SMALL_MEMORY
#define RAMSIZE         (16 * 1024)
#define DATSIZE         (4 * 1024)
#define MAXCODE         (4 * 1024)
#else
#define RAMSIZE         (32 * 1024)
#define DATSIZE         (8 * 1024)
#define MAXCODE         (8 * 1024)
#endif
#define NEED_STRCASECMP

/* support for the Propeller under ZOG */
#elif defined(PROPELLER_ZOG)
#define WORD_SIZE_32
#define FAR_DATA
#define FLASH_SPACE		const
#define RAMSIZE         (32 * 1024)
#define DATSIZE         (8 * 1024)
#define MAXCODE         (8 * 1024)
#define NEED_STRCASECMP

/* support for windows */
#elif defined(WIN32)
#define WORD_SIZE_32
#define FAR_DATA
#define FLASH_SPACE		const
#define RAMSIZE         (32 * 1024)
#define DATSIZE         (8 * 1024)
#define MAXCODE         (8 * 1024)
#define NEED_STRCASECMP

/* support for posix */
#else
#define WORD_SIZE_32
#define FAR_DATA
#define FLASH_SPACE		const
#define RAMSIZE         (128 * 1024)
#define DATSIZE         (32 * 1024)
#define MAXCODE         (16 * 1024)

#endif

/* for all propeller platforms */
#define HUB_BASE        0x00000000
#define HUB_SIZE        (32 * 1024)
#define COG_BASE        0x10000000
#define RAM_BASE        0x20000000
#define FLASH_BASE      0x30000000

#ifdef NO_STDINT
typedef long int32_t;
typedef unsigned long uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif

#ifdef WORD_SIZE_16
typedef int16_t VMVALUE;
typedef uint16_t VMUVALUE;
#endif

#ifdef WORD_SIZE_32
typedef int32_t VMVALUE;
typedef uint32_t VMUVALUE;
#endif

#ifndef TRUE
#define TRUE        1
#define FALSE       0
#endif

#define VMTRUE		TRUE
#define VMFALSE   	FALSE

#define RCFAST      0x00
#define RCSLOW      0x01
#define XINPUT      0x22
#define XTAL1       0x2a
#define XTAL2       0x32
#define XTAL3       0x3a
#define PLL1X       0x41
#define PLL2X       0x42
#define PLL4X       0x43
#define PLL8X       0x44
#define PLL16X      0x45

#ifdef NEED_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#endif

#define ALIGN_MASK				(sizeof(VMVALUE) - 1)
#define ROUND_TO_WORDS(x)       (((x) + ALIGN_MASK) & ~ALIGN_MASK)

#define VMCODEBYTE(p)           *(p)
#define VMINTRINSIC(i)          Intrinsics[i]

/* memory section */
typedef struct Section Section;
struct Section {
    VMUVALUE base;      // base address
    VMUVALUE size;      // maximum size
    VMUVALUE offset;    // next available offset
    FILE *fp;           // image or scratch file pointer
    Section *next;      // next section
    char name[1];       // section name
};

typedef struct BoardConfig BoardConfig;
struct BoardConfig {
    uint32_t clkfreq;
    uint8_t clkmode;
    uint32_t baudrate;
    uint8_t rxpin;
    uint8_t txpin;
    uint8_t tvpin;
    char *cacheDriver;
    VMUVALUE cacheSize;
    VMUVALUE cacheParam1;
    VMUVALUE cacheParam2;
    char *defaultTextSection;
    char *defaultDataSection;
    int sectionCount;
    Section *sections;
    BoardConfig *next;
    char name[1];
};

void ParseConfigurationFile(char *path);
BoardConfig *GetBoardConfig(char *name);
Section *GetSection(BoardConfig *config, char *name);

#endif
