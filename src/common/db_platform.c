/* db_platform.c - platform interfaces */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
