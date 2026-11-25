#pragma once
#include <stdbool.h>

typedef struct {
    char models_dir[4096];
    char chats_dir[4096];
    int  port;
    int  n_ctx;
    int  reserve;
    char** server_flags;
    char* default_sysprompt;
    int  flags_count;
} cfg;

bool cfgload(void);
void cfgfree(cfg* config);
