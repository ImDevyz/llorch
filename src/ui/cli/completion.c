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
#include "cli.h"

extern const struct cmd cmds[];

static int rl_nl(int count, int key) {
    (void)count; (void)key;
    rl_insert_text("\n");
    return 0;
}

static char *llgen(const char *text, int state) {
    static int i; static size_t len; static DIR *d;
    static const char *ext;

    if (!state) {
        i = 0; len = strlen(text); ext = NULL;
        if (d) { closedir(d); d = NULL; }

        char *sp = strchr(rl_line_buffer, ' ');
        if (rl_line_buffer[0] == '/' && sp) {
            size_t clen = (size_t)(sp - (rl_line_buffer + 1));
            for (int k = 0; cmds[k].name; k++) {
                if (strlen(cmds[k].name) == clen &&
                    !strncmp(rl_line_buffer + 1, cmds[k].name, clen) &&
                    cmds[k].dir) {
                    char base[4096], dir[4096];
                    resolve("~/.local/share/llorch", base, sizeof(base));
                    snprintf(dir, sizeof(dir), "%s/%s", base, cmds[k].dir);
                    d = opendir(dir);
                    ext = cmds[k].ext;
                    break;
                }
            }
        }
    }

    if (rl_line_buffer[0] == '/' && !strchr(rl_line_buffer, ' ')) {
        while (cmds[i].name) {
            char buf[64];
            snprintf(buf, sizeof(buf), "/%s", cmds[i++].name);
            if (!strncmp(buf, text, len)) {
                return strdup(buf);
            }
        }
        return NULL;
    }

    if (!d) return NULL;
    struct dirent *e;
    while ((e = readdir(d))) {
        char *dot = strrchr(e->d_name, '.');
        if (!dot || strcmp(dot, ext)) continue;
        size_t n = (size_t)(dot - e->d_name);
        if (n >= len && !strncmp(e->d_name, text, len))
            return strndup(e->d_name, n);
    }
    closedir(d); d = NULL;
    return NULL;
}

static void kbd_off(void) {
    printf("\x1b[<1u");
    fflush(stdout);
}

void completion_set() {
    rl_completion_entry_function   = llgen;
    rl_basic_word_break_characters = " \t\n";

    printf("\x1b[>1u");
    atexit(kbd_off);
    rl_bind_keyseq("\\e[13;2u", rl_nl);
    rl_bind_keyseq("\\e\r",     rl_nl);
    rl_bind_keyseq("\\e\n",     rl_nl);
}
