/* db_runtime.c - runtime support for xbasic */

#include "db_vm.h"

#if defined(PROPELLER_CAT)

VMVALUE VM_getreg(int index)
{
    VMVALUE value = 0;
    switch (index) {
    case RG_CNT:
        value = _cnt();
        break;
    case RG_DIRA:
        value = _dira(0, 0);
        break;
    case RG_OUTA:
        value = _outa(0, 0);
        break;
    case RG_INA:
        value = _ina();
        break;
    case RG_CLKFREQ:
        value = _clockfreq();
        break;
    }
	return value;
}

void VM_setreg(int index, VMVALUE value)
{
    switch (index) {
    case RG_DIRA:
        _dirb(0xffffffff, value);
        break;
    case RG_OUTA:
        _outb(0xffffffff, value);
        break;
    }
}

#elif defined(PROPELLER_ZOG)

VMVALUE VM_getreg(int index)
{
    VMVALUE value = 0;
    switch (index) {
    case RG_CNT:
        value = CNT;
        break;
    case RG_DIRA:
        value = DIRA;
        break;
    case RG_OUTA:
        value = OUTA;
        break;
    case RG_INA:
        value = INA;
        break;
    case RG_CLKFREQ:
        value = CLKFREQ;
        break;
    }
	return value;
}

void VM_setreg(int index, VMVALUE value)
{
    switch (index) {
    case RG_DIRA:
        DIRA = value;
        break;
    case RG_OUTA:
        OUTA = value;
        break;
    }
}

#else // posix

VMVALUE VM_getreg(int index)
{
    VMVALUE value;
    printf("REG %d? ", index);
    scanf("%d", &value);
    return value;
}

void VM_setreg(int index, VMVALUE value)
{
    printf("REG %d = %08x\n", index, value);
}

#endif // posix
