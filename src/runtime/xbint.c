#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db_system.h"
#include "mem_malloc.h"
#include "db_vm.h"

static void MyInfo(System *sys, const char *fmt, va_list ap);
static void MyError(System *sys, const char *fmt, va_list ap);
static SystemOps myOps = {
    MyInfo,
    MyError
};

int main(int argc, char *argv[])
{
    ImageHdr *image;
    Interpreter *i;
    char *infile;
    System *sys;
    
    if (argc != 2) {
        fprintf(stderr, "usage: xbint <file>\n");
        return 1;
    }
    infile = argv[1];
    
    sys = MemInit();
    sys->ops = &myOps;

    if (!(image = LoadImage(sys, infile)))
        Fatal(sys, "can't load image '%s'", infile);

    if (!(i = (Interpreter *)InitInterpreter(sys, image)))
        Fatal(sys, "insufficient memory");
        
    Execute(i, image);
    
    return 0;
}

static void MyInfo(System *sys, const char *fmt, va_list ap)
{
    vfprintf(stdout, fmt, ap);
}

static void MyError(System *sys, const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
}

void Fatal(System *sys, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    xbError(sys, "error: ");
    xbErrorV(sys, fmt, ap);
    xbError(sys, "\n");
    va_end(ap);
    exit(1);
}
