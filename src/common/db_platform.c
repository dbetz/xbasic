/* db_platform.c - platform interfaces */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "db_system.h"

/**************/
/* VM_getline */
/**************/

#if defined(PROPELLER_CAT)

#include <catalina_hmi.h>

#define CURSOR	1
#define DEL		0x7f

void VM_getline(char *buf, int size)
{
	int i = 0;
	while (i < size - 1) {
		int ch = k_wait();
		if (ch == '\n') {
			buf[i++] = '\n';
			VM_putchar('\n');
			break;
		}
		else if (ch == '\b' || ch == DEL) {
			if (i > 0) {
				VM_putchar('\b');
				VM_putchar(' ');
				VM_putchar('\b');
				--i;
			}
		}
		else {
			buf[i++] = ch;
			VM_putchar(ch);
		}
	}
	buf[i] = '\0';
}

#else // posix

void VM_getline(char *buf, int size)
{
	fgets(buf, size, stdin);
}

#endif

/**************/
/* VM_getchar */
/**************/

#if defined(PROPELLER_CAT)

int VM_getchar(void)
{
	return -1;
}

#else // posix

int VM_getchar(void)
{
	return getchar();
}

#endif

/**************/
/* VM_putchar */
/**************/

#if defined(PROPELLER_CAT)

void VM_putchar(int ch)
{
	t_char(CURSOR, ch);
}

#else // posix

void VM_putchar(int ch)
{
	putchar(ch);
}

#endif

/****************/
/* VM_AddToPath */
/****************/

typedef struct PathEntry PathEntry;
struct PathEntry {
    PathEntry *next;
    char path[1];
};

static PathEntry *path = NULL;
static PathEntry **pNextPathEntry = &path;

int VM_AddToPath(const char *p)
{
    PathEntry *entry = malloc(sizeof(PathEntry) + strlen(p));
    if (!(entry))
        return VMFALSE;
    strcpy(entry->path, p);
    *pNextPathEntry = entry;
    pNextPathEntry = &entry->next;
    entry->next = NULL;
    return VMTRUE;
}

#if defined(WIN32)

#include <windows.h>
#include <psapi.h>

/* GetProgramPath - get the path relative the application directory */
const char *GetProgramPath(void)
{
    static char fullpath[1024];
    char *p;
    
    /* get the full path to the executable */
    if (!GetModuleFileNameEx(GetCurrentProcess(), NULL, fullpath, sizeof(fullpath)))
        return NULL;

    /* remove the executable filename */
    if ((p = strrchr(fullpath, '\\')) != NULL)
        *p = '\0';

    /* remove the immediate directory containing the executable (usually 'bin') */
    if ((p = strrchr(fullpath, '\\')) != NULL) {
        *p = '\0';
        
        /* check for the 'Release' or 'Debug' build directories used by Visual C++ */
        if (strcmp(&p[1], "Release") == 0 || strcmp(&p[1], "Debug") == 0) {
            if ((p = strrchr(fullpath, '\\')) != NULL)
                *p = '\0';
        }
    }

    /* generate the full path to the 'include' directory */
    strcat(fullpath, "\\include\\");
    return fullpath;
}

#endif

/*************************/
/* VM_AddEnvironmentPath */
/*************************/

#if defined(WIN32)
#define SEP ';'
#else
#define SEP ':'
#endif

int VM_AddEnvironmentPath(void)
{
    char *p, *end;
    
    /* add path entries from the environment */
    if ((p = getenv("XB_INC")) != NULL) {
        while ((end = strchr(p, SEP)) != NULL) {
            *end = '\0';
            if (!VM_AddToPath(p))
                return VMFALSE;
            p = end + 1;
        }
        if (!VM_AddToPath(p))
            return VMFALSE;
    }
    
    /* add the path relative to the location of the executable */
#if defined(WIN32)
    if (!(p = GetProgramPath()))
        if (!VM_AddToPath(p))
            return VMFALSE;
#endif

    return VMTRUE;
}

#include <string.h>

static const char *MakePath(PathEntry *entry, const char *name)
{
    static char fullpath[1024];
    strcpy(fullpath, entry->path);
#if defined(WIN32)
	strcat(fullpath, "\\");
#else
	strcat(fullpath, "/");
#endif
	strcat(fullpath, name);
	return fullpath;
}

/************/
/* VM_fopen */
/************/

FILE *VM_fopen(const char *name, const char *mode)
{
    PathEntry *entry;
    FILE *fp;
    
#if 0
    printf("path:");
    for (entry = path; entry != NULL; entry = entry->next)
        printf(" '%s'", entry->path);
    printf("\n");
#endif
    
    if (!(fp = fopen(name, mode))) {
        for (entry = path; entry != NULL; entry = entry->next)
            if ((fp = fopen(MakePath(entry, name), mode)) != NULL)
                break;
    }
    return fp;
}

/********************/
/* VM_CreateTmpFile */
/********************/

FILE *VM_CreateTmpFile(const char *name, const char *mode)
{
    return fopen(name, mode);
}

/********************/
/* VM_RemoveTmpFile */
/********************/

void VM_RemoveTmpFile(const char *name)
{
    remove(name);
}

/**************/
/* strcasecmp */
/**************/

#if defined(NEED_STRCASECMP)

int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && (tolower(*s1) == tolower(*s2))) {
        ++s1;
        ++s2;
    }
    return tolower((unsigned char) *s1) - tolower((unsigned char) *s2);
}

#endif
