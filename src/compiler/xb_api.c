#include "db_compiler.h"
#include "xb_api.h"

/* compiler context */
static ParseContext *c = NULL;

static void SourceRewind(void *cookie);
static int SourceGetLine(void *cookie, char *buf, int len);

int xbInit(System *sys, BoardConfig *config, size_t maxCode)
{
    /* initialize the compiler */
    if (!(c = InitCompiler(sys, config, maxCode)))
        return FALSE;
    
    /* return successfully */
    return TRUE;
}

int xbCompile(const char *infile, const char *outfile, int flags)
{
    FILE *ifp;
    
    /* open the input file */
    if (!(ifp = xbOpenFile(c->sys, infile, "r"))) {
        fprintf(stderr, "error: can't open '%s'\n", infile);
        return FALSE;
    }
    
    /* store the compiler flags */
    c->flags = flags;
    
    /* setup source input */
    c->mainFile.u.main.rewind = SourceRewind;
    c->mainFile.u.main.getLine = SourceGetLine;
    c->mainFile.u.main.getLineCookie = ifp;
    
    /* compile the source file */
    if (!Compile(c, outfile)) {
        fprintf(stderr, "error: compile failed\n");
        return FALSE;
    }
    
    /* close the files and remove the data section temporary file */
    fclose(ifp);

    /* return successfully */
    return TRUE;
}

static void SourceRewind(void *cookie)
{
    xbSeekFile(cookie, 0, SEEK_SET);
}

static int SourceGetLine(void *cookie, char *buf, int len)
{
	return xbGetLine(cookie, buf, len) != NULL;
}
