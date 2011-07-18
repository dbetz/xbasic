#include <stdio.h>
#include <string.h>
#include "db_system.h"
#include "db_vm.h"

typedef struct {
    FILE *fp;
    int lineNumber;
} SourceInfo;

FAR_DATA uint8_t space[RAMSIZE];

static FILE *ifp;

int main(int argc, char *argv[])
{
    size_t freeSize, stackSize;
    Interpreter *i;
    ImageHdr *image;
    char *infile;
    System sys;
    
    if (argc != 2) {
        fprintf(stderr, "usage: xbint <file>\n");
        return 1;
    }
    infile = argv[1];
    
    if (!(ifp = fopen(infile, "rb"))) {
        fprintf(stderr, "error: can't open '%s'\n", infile);
        return 1;
    }
    
    InitFreeSpace(&sys, space, sizeof(space));
    
    image = LoadImage(&sys);

    i = (Interpreter *)AllocateAllFreeSpace(&sys, &freeSize);
    stackSize = (freeSize - sizeof(Interpreter)) / sizeof(VMVALUE);
    if (stackSize <= 0)
        VM_printf("insufficient memory\n");
    else {
        InitInterpreter(i, stackSize);
        Execute(i, image);
    }
    
    fclose(ifp);
    
    return 0;
}

void ImageFileRewind(void)
{
    fseek(ifp, 0, SEEK_SET);
}

int ImageFileRead(uint8_t *buf, int size)
{
    return fread(buf, 1, size, ifp) == size;
}
