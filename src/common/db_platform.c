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

/**************/
/* VM_setpath */
/**************/

static char path[1024] = "";

void VM_setpath(const char *p)
{
    strcpy(path, p);
}

/***************/
/* VM_fullpath */
/***************/

#if defined(WIN32)

#include <windows.h>
#include <psapi.h>

/* VM_fullpath - get the full path to a file in the application directory */
const char *VM_fullpath(const char *name)
{
    static char fullpath[1024];
    char *p;
    
    /* first check for command line path */
    if (path[0]) {
        strcpy(fullpath, path);
        strcat(fullpath, "\\");
        strcat(fullpath, name);
        return fullpath;
    }
    
	/* first check for the XB_INC environment variable */
    else if ((p = getenv("XB_INC")) != NULL) {
        strcpy(fullpath, p);
        strcat(fullpath, "\\");
        strcat(fullpath, name);
        return fullpath;
    }

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

    /* generate the full path to the file */
    strcat(fullpath, "\\include\\");
    strcat(fullpath, name);
    return fullpath;
}

#else

#include <string.h>

const char *VM_fullpath(const char *name)
{
    static char fullpath[1024];
    char *p;
    
    /* first check for command line path */
    if (path[0])
        p = path;
    
    /* check for the XB_INC environment variable */
    else if (!(p = getenv("XB_INC")))
		return NULL;
        
    /* generate the full path to the file */
    strcpy(fullpath, p);
	strcat(fullpath, "/");
	strcat(fullpath, name);
	return fullpath;
}

#endif

/************/
/* VM_fopen */
/************/

FILE *VM_fopen(const char *name, const char *mode)
{
    FILE *fp;
    if (!(fp = fopen(name, mode))) {
        const char *path = VM_fullpath(name);
        if (path)
            fp = fopen(path, mode);
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
