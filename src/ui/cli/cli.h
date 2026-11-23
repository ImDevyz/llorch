#pragma once
#include "ui.h"

#define COLOR_USER    "\033[1;36m"
#define COLOR_MODEL   "\033[1;37m"
#define COLOR_SYSTEM  "\033[1;33m"
#define COLOR_ERROR   "\033[1;31m"
#define COLOR_RESET   "\033[0m"
#define COLOR_THINK   "\033[90m"

struct cmd {
    const char *name;
    sc (*handler)(char *, char *);
    const char *dir;
    const char *ext;
};

void print_box();
