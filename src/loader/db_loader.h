#ifndef __DB_LOADER_H__
#define __DB_LOADER_H__

#include "db_image.h"

int InitPort(char *port);
int LoadImage(BoardConfig *config, char *port, char *path);
int WriteHubLoaderToEEPROM(BoardConfig *config, char *port, char *path);
int WriteFlashLoaderToEEPROM(BoardConfig *config, char *port);
int RunLoadedProgram(int step);

#endif
