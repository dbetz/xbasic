#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "db_compiler.h"
#include "db_loader.h"
#include "db_packet.h"
#include "mem_malloc.h"

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

static void Usage(void);
static char *ConstructOutputName(const char *infile, char *outfile, char *ext);

static void MyInfo(System *sys, const char *fmt, va_list ap);
static void MyError(System *sys, const char *fmt, va_list ap);
static SystemOps myOps = {
    MyInfo,
    MyError
};

int main(int argc, char *argv[])
{
    char *infile = NULL, outfile[PATH_MAX];
    char *port, *board, *p;
    BoardConfig *config;
    int writeEepromLoader = FALSE;
    int runImage = FALSE;
    int terminalMode = FALSE;
    int step = FALSE;
    int compilerFlags = 0;
    System *sys;
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
                writeEepromLoader = TRUE;
                break;
            case 's':
                step = TRUE;
                // fall through
            case 'r':
                runImage = TRUE;
                break;
            case 't':
                terminalMode = TRUE;
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
                xbAddToPath(p);
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
    
    /* make sure an input file was specified */
    if (!infile)
        Usage();

    /* create the output file name */
    ConstructOutputName(infile, outfile, ".bai");
    
    /* make sure -e and -r aren't used together */
    if (writeEepromLoader && runImage) {
        fprintf(stderr, "error: writing the eeprom loader and running the program are mutually exclusive\n");
        return 1;
    }
        
    /* initialize the memory allocator */
    if (!(sys = MemInit())) {
        fprintf(stderr, "error: memory initialization failed\n");
        return 1;
    }
    sys->ops = &myOps;
        
    /* add the XB_INC environment path */
    xbAddEnvironmentPath();
    
    /* load the board configuration file */
    ParseConfigurationFile(sys, "xbasic.cfg");

    /* setup for the selected board */
    if (!(config = GetBoardConfig(board))) {
        fprintf(stderr, "error: no board type: %s\n", board);
        return 1;
    }
    
    /* initialize the compiler */
    if (!xbInit(sys, config, MAXCODE)) {
        fprintf(stderr, "error: compiler initialization failed\n");
        return 1;
    }
        
    /* compile the source file */
    if (!xbCompile(infile, outfile, compilerFlags))
        return 1;
        
    /* free allocated memory */
    MemFree(sys);
    
    /* open the port if necessary */
    if (runImage || writeEepromLoader || terminalMode) {
        if (!InitPort(port)) {
            fprintf(stderr, "error: opening serial port\n");
            return 1;
        }
    }
    
    /* load the compiled image if necessary */
    if (runImage || (writeEepromLoader && config->cacheDriver)) {
        if (!LoadImage(sys, config, port, outfile)) {
            fprintf(stderr, "error: load failed\n");
            return 1;
        }
    }
        
    /* flash the program into eeprom if requested */
    if (writeEepromLoader) {
        if (config->cacheDriver)
            WriteFlashLoaderToEEPROM(sys, config, port);
        else
            WriteHubLoaderToEEPROM(sys, config, port, outfile);
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
static char *ConstructOutputName(const char *infile, char *outfile, char *ext)
{
    char *end = strrchr(infile, '.');
    if (end && !strchr(end, '/') && !strchr(end, '\\')) {
        strncpy(outfile, infile, end - infile);
        outfile[end - infile] = '\0';
    }
    else
        strcpy(outfile, infile);
    strcat(outfile, ext);
    return outfile;
}

static void MyInfo(System *sys, const char *fmt, va_list ap)
{
    vfprintf(stdout, fmt, ap);
}

static void MyError(System *sys, const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
}

