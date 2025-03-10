#ifndef _CLIOPTS_H
#define _CLIOPTS_H

#include "platform.h"

struct PCCLIOptions  {
    unsigned int SkipIntro;
    unsigned int SkipGreeter;
    unsigned int FullScreen;
    unsigned int PoolSize;
    unsigned int SyncFrames;
    unsigned int ExtractOnly;
    char ConfigFile[SYS_MAX_PATH];
    char SavePath[SYS_MAX_PATH];
    char RomPath[SYS_MAX_PATH];
    char GameDir[SYS_MAX_PATH];
};

extern struct PCCLIOptions gCLIOpts;

void parse_cli_opts(int argc, char* argv[]);

#endif // _CLIOPTS_H
