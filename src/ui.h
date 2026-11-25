#pragma once
#include <stdbool.h>

typedef enum {
    OK = 0,
    ERR = 1,
    QUIT = 2
} sc;

void ui_start();
void print(const char* text, bool is_think);
void print_chat();
void sysmsg(const char* text, ...);
void syserr(const char* text, ...);
void modelans();
