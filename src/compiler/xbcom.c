#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "db_system.h"
#include "db_compiler.h"
#include "db_loader.h"
#include "db_packet.h"

/* defaults */
#if defined(CYGWIN) || defined(WIN32)
#define DEF_PORT    "COM1"
#endif
#ifdef LINUX
#define DEF_PORT    "/dev/ttyUSB0"
#endif
#ifdef MACOSX
#define DEF_PORT    "/dev/tty.usbserial-A3TNQL8K"
#endif
#define DEF_BOARD   "c3"

FAR_DATA uint8_t space[RAMSIZE];

static void Usage(void);
static void ConstructOutputName(const char *infile, char *outfile, char *ext);
static void SourceRewind(void *cookie);
static int SourceGetLine(void *cookie, char *buf, int len);

int main(int argc, char *argv[])
{
    char *infile = NULL, outfile[FILENAME_MAX];
    char *port, *board, *p;
    BoardConfig *config;
    int writeEepromLoader = VMFALSE;
    int runImage = VMFALSE;
    int terminalMode = VMFALSE;
    int step = VMFALSE;
    int compilerFlags = 0;
    ParseContext *c;
    System sys;
    FILE *ifp;
    int i;
    
    /* get the environment settings */
    if (!(port = getenv("PORT")))
        port = DEF_PORT;
    if (!(board = getenv("BOARD")))
        board = DEF_BOARD;
        
    /* get the arguments */
    for(i = 1; i < argc; ++i) {

        /* handle switches */
        if(argv[i][0] == '-') {
            switch(argv[i][1]) {
            case 'b':   // select a target board
                if (argv[i][2])
                    board = &argv[i][2];
                else if (++i < argc)
                    board = argv[i];
                else
                    Usage();
                break;
            case 'p':
                if(argv[i][2])
                    port = &argv[i][2];
                else if(++i < argc)
                    port = argv[i];
                else
                    Usage();
                if (isdigit((int)port[0])) {
#if defined(CYGWIN) || defined(WIN32)
                    static char buf[10];
                    sprintf(buf, "COM%d", atoi(port));
                    port = buf;
#endif
#ifdef LINUX
                    static char buf[10];
                    sprintf(buf, "/dev/ttyUSB%d", atoi(port));
                    port = buf;
#endif
                }
                break;
            case 'e':
                writeEepromLoader = VMTRUE;
                break;
            case 's':
                step = VMTRUE;
                // fall through
            case 'r':
                runImage = VMTRUE;
                break;
            case 't':
                terminalMode = VMTRUE;
                break;
            case 'd':
                compilerFlags |= COMPILER_DEBUG;
                break;
            case 'v':
                compilerFlags |= COMPILER_INFO;
                break;
            case 'I':
                if(argv[i][2])
                    p = &argv[i][2];
                else if(++i < argc)
                    p = argv[i];
                else
                    Usage();
                VM_setpath(p);
                break;
            default:
                Usage();
                break;
            }
        }

        /* handle the input filename */
        else {
            if (infile)
                Usage();
            infile = argv[i];
        }
    }
    
    ParseConfigurationFile("xbasic.cfg");

    /* make sure an input file was specified */
    if (!infile)
        Usage();
        
    /* make sure -e and -r aren't used together */
    if (writeEepromLoader && runImage) {
        fprintf(stderr, "error: writing the eeprom loader and running the program are mutually exclusive\n");
        return 1;
    }
        
    /* setup for the selected board */
    if (!(config = GetBoardConfig(board))) {
        fprintf(stderr, "error: no board type: %s\n", board);
        return 1;
    }
    
    /* open the input file */
    if (!(ifp = fopen(infile, "r"))) {
        fprintf(stderr, "error: can't open '%s'\n", infile);
        return 1;
    }
    
    /* create the output file name */
    ConstructOutputName(infile, outfile, ".bai");
    
    /* initialize the memory allocator */
    InitFreeSpace(&sys, space, sizeof(space));

    /* initialize the compiler */
    if (!(c = InitCompiler(&sys, config, MAXCODE))) {
        fprintf(stderr, "error: compiler initialization failed\n");
        return 1;
    }
    c->flags = compilerFlags;
    
    /* setup source input */
    c->mainFile.u.main.rewind = SourceRewind;
    c->mainFile.u.main.getLine = SourceGetLine;
    c->mainFile.u.main.getLineCookie = ifp;
    
    /* compile the source file */
    if (!Compile(c, outfile)) {
        fprintf(stderr, "error: compile failed\n");
        return 1;
    }
    
    /* close the files and remove the data section temporary file */
    fclose(ifp);
    
    /* open the port if necessary */
    if (runImage || writeEepromLoader || terminalMode) {
        if (!InitPort(port)) {
            fprintf(stderr, "error: opening serial port\n");
            return 1;
        }
    }
    
    /* load the compiled image if necessary */
    if (runImage || (writeEepromLoader && config->cacheDriver)) {
        if (!LoadImage(config, port, outfile)) {
            fprintf(stderr, "error: load failed\n");
            return 1;
        }
    }
        
    /* flash the program into eeprom if requested */
    if (writeEepromLoader) {
        if (config->cacheDriver)
            WriteFlashLoaderToEEPROM(config, port);
        else
            WriteHubLoaderToEEPROM(config, port, outfile);
    }
    
    /* run the loaded image if requested */
    if (runImage) {
        if (!RunLoadedProgram(step)) {
            fprintf(stderr, "error: run failed\n");
            return 1;
        }
    }
    
    /* enter terminal mode if requested */
    if (terminalMode)
        TerminalMode();
    
    return 0;
}

/* Usage - display a usage message and exit */
static void Usage(void)
{
    fprintf(stderr, "\
usage: xbcom\n\
         [ -b <type> ]   select target board (c3 | ssf | hub | hub96) (default is hub)\n\
         [ -p <port> ]   serial port (default is %s)\n\
         [ -w ]          write a spin source file\n\
         [ -e ]          write loader to eeprom\n\
         [ -r ]          load and run the compiled program\n\
         [ -t ]          enter terminal mode after running the program\n\
         [ -d ]          display compiler debug information\n\
         [ -v ]          display verbose compiler statistics\n\
         [ -I <path> ]   set the path for include files\n\
         <name>          file to compile\n\
", DEF_PORT);
    exit(1);
}

/* ConstructOutputName - construct an output filename from an input filename */
static void ConstructOutputName(const char *infile, char *outfile, char *ext)
{
    char *end = strrchr(infile, '.');
    if (end && !strchr(end, '/') && !strchr(end, '\\')) {
        strncpy(outfile, infile, end - infile);
        outfile[end - infile] = '\0';
    }
    else
        strcpy(outfile, infile);
    strcat(outfile, ext);
}

static void SourceRewind(void *cookie)
{
    FILE *fp = (FILE *)cookie;
    fseek(fp, 0, SEEK_SET);
}

static int SourceGetLine(void *cookie, char *buf, int len)
{
    FILE *fp = (FILE *)cookie;
	return fgets(buf, len, fp) != NULL;
}
