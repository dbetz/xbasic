#ifndef __XB_API_H__
#define __XB_API_H__

/* compiler flags */
#define COMPILER_DEBUG  (1 << 0)
#define COMPILER_INFO   (1 << 1)

int xbInit(System *sys, BoardConfig *config, size_t maxCode);
int xbCompile(const char *infile, const char *outfile, int flags);

#endif
