#include "ui.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "commands.h"
#include "server.h"
#include "completion.h"
#include "cli.h"

extern char *ModelName;
extern char *ChatName;
extern char *ToolName;

sc help(char* nil1, char* nil2) {
    (void)nil1; (void)nil2;
    printf("To write a command use '/[command] [arguments]'\n");
    printf("- help              List of commands\n");
    printf("- load <path>       Start llama-server\n");
    printf("- mkchat <name>     Create chat\n");
    printf("- rmchat <name>     Delete chat\n");
    printf("- open <name>       Open chat\n");
    printf("- exit              Quit\n");
    printf("\nKeys:\n");
    printf("  Shift/Alt+Enter   New line\n");
    printf("  Ctrl+C            Stop generation\n");
    printf("  Ctrl+T 0..3       Reasoning: off/low/medium/high\n");
    printf("  Ctrl+T Ctrl+T     Show/hide reasoning\n");
    return OK;
}

const struct cmd cmds[] = {
    {"help", help, NULL, NULL},
    {"load", load_model, "Models", ".gguf"},
    {"mkchat", mkchat, NULL, NULL},
    {"rmchat", rmchat, "Chats", ".jsonl"},
    {"open", chopen, "Chats", ".jsonl"},
    {"renchat", renchat, "Chats", ".jsonl"},
    {"addmodel", addmodel, NULL, NULL},
    {"exit", pexit, NULL, NULL},
    {NULL, NULL, NULL, NULL}
};

void print_box() {
    printf("\n");
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        for (int i = 0; i < w.ws_col; i++) printf("─");
    }
}

sc hcomm(char *buf) {
    print_box();
    printf(COLOR_SYSTEM "\n[%s] llorch:\n" COLOR_RESET, ToolName);

    char *cmd = buf + 1;
    char *a1 = strchr(cmd, ' ');
    if (a1) *a1++ = '\0';
    char *a2 = a1 ? strchr(a1, ' ') : NULL;
    if (a2) *a2++ = '\0';

    sc code;
    for (int i = 0; cmds[i].name; i++) {
        if (!strcmp(cmd, cmds[i].name)) {
            code = cmds[i].handler(a1, a2);
            return code;
        }
    }

    printf("Undefined command\n");
    return ERR;
}

void ui_start() {
    completion_set();
    using_history();
    system("clear");
    printf(COLOR_SYSTEM "Welcome to llorch!\n" COLOR_RESET);

    while (1) {
        printf("\n");
        print_box();
        char prompt[256];
        snprintf(prompt, sizeof(prompt), "\n\001%s\002[%s] You:\n\001%s\002", COLOR_USER, ChatName, COLOR_RESET);

        char *line = readline(prompt);
        if (!line) {
            printf("\nBye!\n\n");
            break;
        }
        if (*line) add_history(line);

        if (line[0] == '/') {
            sc code = hcomm(line);
            if (code == QUIT) {
                free(line);
                break;
            }
        } else if (line[0]) hchat(line);

        free(line);
    }
}
