#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db_spin.h"
#include "db_image.h"

#define CLKMODE_EXTERN_MODE 0x20
#define CLKMODE_RCFAST      0x00
#define CLKMODE_RCSLOW      0x01
#define CLKMODE_EXTERN_MASK 0xb8
#define CLKMODE_XINPUT      0x20
#define CLKMODE_XTAL1       0x28
#define CLKMODE_XTAL2       0x30
#define CLKMODE_XTAL3       0x38
#define CLKMODE_PLL_MASK    0x47
#define CLKMODE_NO_PLL      0x02
#define CLKMODE_PLL1X       0x43
#define CLKMODE_PLL2X       0x44
#define CLKMODE_PLL4X       0x45
#define CLKMODE_PLL8X       0x46
#define CLKMODE_PLL16X      0x47

/* xbasic vm code */
extern uint8_t xbasic_vm_array[];
extern int xbasic_vm_size;

/* prototypes */
static char *ClkModeStr(uint8_t clkmode);

/* WriteSpinFile - write a spin file containing the generated binary image */
void WriteSpinFile(BoardConfig *config, const char *imagefile, const char *spinfile, int flashBoot)
{
    ImageFileHdr hdr;
    FILE *ifp, *ofp;
    int byte, cnt, remaining;
    char *clkmode;
    uint8_t *p;
    
    if (!(clkmode = ClkModeStr(config->clkmode))) {
        fprintf(stderr, "error: invalid clock mode 0x%02x\n", config->clkmode);
        exit(1);
    }
    
    if (!(ifp = fopen(imagefile, "rb"))) {
        fprintf(stderr, "error: can't open '%s'\n", imagefile);
        exit(1);
    }
    
    if (fread(&hdr, 1, sizeof(hdr), ifp) != sizeof(hdr)) {
        fprintf(stderr, "error: bad file header: %s", imagefile);
        exit(1);
    }
    
    fseek(ifp, 0, SEEK_SET);
    
    if (!(ofp = fopen(spinfile, "wb"))) {
        fprintf(stderr, "error: can't create '%s'\n", spinfile);
        exit(1);
    }
    
    fprintf(ofp, "CON\n\n");
    fprintf(ofp, "  _clkmode = %s\n", clkmode);
    fprintf(ofp, "  _clkfreq = %d\n\n", config->clkfreq);
    fprintf(ofp, "  hub_memory_size = 32 * 1024\n");
    fprintf(ofp, "  vm_mbox_size = runtime#_MBOX_SIZE * 4\n");
    fprintf(ofp, "  vm_state_size = runtime#_STATE_SIZE * 4\n");
    
    if (flashBoot) {
        fprintf(ofp, "  cache_size = %d\n", config->cacheSize);
        fprintf(ofp, "  cache_mbox_size = cacheint#_MBOX_SIZE * 4\n\n");
        fprintf(ofp, "  cache = hub_memory_size - cache_size\n");
        fprintf(ofp, "  cache_mbox = cache - cache_mbox_size\n");
        fprintf(ofp, "  vm_mbox = cache_mbox - vm_mbox_size\n");
        fprintf(ofp, "  vm_state = vm_mbox - vm_state_size\n");
        fprintf(ofp, "  data_end = vm_state\n\n");
        fprintf(ofp, "  loader_stack_size = 32 * 4\n");
    }
    else {
        fprintf(ofp, "  stack_size = %d\n", hdr.stackSize);
        fprintf(ofp, "\n");
        fprintf(ofp, "  vm_mbox = hub_memory_size - vm_mbox_size\n");
        fprintf(ofp, "  vm_state = vm_mbox - vm_state_size\n");
    }

    fprintf(ofp, "\nOBJ\n\n");
    fprintf(ofp, "  runtime : \"vm_runtime\"\n");
    if (flashBoot)
        fprintf(ofp, "  cacheint : \"cache_interface\"\n");

    if (flashBoot) {
        fprintf(ofp, "\nPUB start | data, cache_line_mask\n");
        fprintf(ofp, "  data := @result + loader_stack_size\n");
        fprintf(ofp, "  cache_line_mask := cacheint.start(@cache_code, cache_mbox, cache, %d, %d)\n", config->cacheParam1, config->cacheParam2);
        fprintf(ofp, "  runtime.init(vm_mbox, vm_state, @vm_code, data, cache_mbox, cache_line_mask)\n");
        fprintf(ofp, "  runtime.load(vm_mbox, vm_state, runtime#FLASH_BASE, data_end)\n", hdr.stackSize);
    }
    else {
        fprintf(ofp, "\nPUB start\n");
        fprintf(ofp, "  runtime.init(vm_mbox, vm_state, @vm_code, @image, 0, 0)\n");
        fprintf(ofp, "  runtime.load(vm_mbox, vm_state, runtime#HUB_BASE, @stack + stack_size * 4)\n", hdr.stackSize);
    }
    fprintf(ofp, "  runtime.init_serial(%d, %d, %d)\n", config->baudrate, config->rxpin, config->txpin);
    fprintf(ofp, "  waitcnt(clkfreq+cnt) ' this is a hack!\n");
    fprintf(ofp, "  runtime.run(vm_mbox, vm_state)\n");

    fprintf(ofp, "\nDAT\n");

    if (!flashBoot) {
        fprintf(ofp, "\nimage\n");
        cnt = 0;
        while ((byte = getc(ifp)) != EOF) {
            if (cnt == 0)
                fprintf(ofp, "  byte ");
            else
                fprintf(ofp, ", ");
            fprintf(ofp, "$%02x", byte);
            if (++cnt == 16) {
                fprintf(ofp, "\n");
                cnt = 0;
            }
        }
        if (cnt > 0)
            putc('\n', ofp);
        fprintf(ofp, "\nstack\n");
        fprintf(ofp, "  long 0[stack_size]\n");
    }
        
    fclose(ifp);

    fprintf(ofp, "\nvm_code\n");
    p = xbasic_vm_array;
    remaining = xbasic_vm_size;
    cnt = 0;
    while (--remaining >= 0) {
        byte = *p++;
        if (cnt == 0)
            fprintf(ofp, "  byte ");
        else
            fprintf(ofp, ", ");
        fprintf(ofp, "$%02x", byte);
        if (++cnt == 16) {
            fprintf(ofp, "\n");
            cnt = 0;
        }
    }
    if (cnt > 0)
        putc('\n', ofp);
        
    if (flashBoot) {
        fprintf(ofp, "\ncache_code\n");
        p = config->cacheDriver->code;
        remaining = *config->cacheDriver->pSize;
        cnt = 0;
        while (--remaining >= 0) {
            byte = *p++;
            if (cnt == 0)
                fprintf(ofp, "  byte ");
            else
                fprintf(ofp, ", ");
            fprintf(ofp, "$%02x", byte);
            if (++cnt == 16) {
                fprintf(ofp, "\n");
                cnt = 0;
            }
        }
        if (cnt > 0)
            putc('\n', ofp);
    }
    
    fclose(ofp);
}
    
static char *ClkModeStr(uint8_t clkmode)
{
    static char buf[20];
    if (clkmode & CLKMODE_EXTERN_MODE) {
        switch (clkmode & CLKMODE_EXTERN_MASK) {
        case CLKMODE_XINPUT:
            strcpy(buf, "XINPUT");
            break;
        case CLKMODE_XTAL1:
            strcpy(buf, "XTAL1");
            break;
        case CLKMODE_XTAL2:
            strcpy(buf, "XTAL2");
            break;
        case CLKMODE_XTAL3:
            strcpy(buf, "XTAL3");
            break;
        default:
            return NULL;
        }
        switch (clkmode & CLKMODE_PLL_MASK) {
        case CLKMODE_NO_PLL:
            // nothing to do
            break;
        case CLKMODE_PLL1X:
            strcat(buf, " + PLL1X");
            break;
        case CLKMODE_PLL2X:
            strcat(buf, " + PLL2X");
            break;
        case CLKMODE_PLL4X:
            strcat(buf, " + PLL4X");
            break;
        case CLKMODE_PLL8X:
            strcat(buf, " + PLL8X");
            break;
        case CLKMODE_PLL16X:
            strcat(buf, " + PLL16X");
            break;
        default:
            return NULL;
        }
    }
    else {
        switch (clkmode) {
        case CLKMODE_RCFAST:
            strcpy(buf, "RCFAST");
            break;
        case CLKMODE_RCSLOW:
            strcpy(buf, "RCSLOW");
            break;
        default:
            return NULL;
        }
    }
    return buf;
}
