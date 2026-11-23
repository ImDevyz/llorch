#pragma once
#include "ui.h"

sc mkchat(char *name, char* arg2);
sc rmchat(char *name, char* arg2);
sc chopen(char *name, char* arg2);
sc resolve(const char *path, char *out, int out_size);
sc renchat(char* OldName, char* NewName);
sc addmodel(char* gguf_path, char* name);
sc pexit(char* arg1, char* arg2);
