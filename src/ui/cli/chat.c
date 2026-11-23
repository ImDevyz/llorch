#include "ui.h"
#include "cli.h"
#include <string.h>
#include <stdio.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <stdarg.h>

extern char *ChatName;
extern char *ToolName;
extern char *ModelName;
extern struct json_object *messages;

void print(char* text, bool is_think) {
    if (is_think) printf(COLOR_THINK "%s" COLOR_RESET, text);
    else printf("%s", text);
}

void print_chat() {
    int count = json_object_array_length(messages);
    for (int i = 0; i < count; i++) {
        json_object *item = json_object_array_get_idx(messages, i);

        json_object *jrole = NULL, *jcontent = NULL;
        json_object_object_get_ex(item, "role", &jrole);
        json_object_object_get_ex(item, "content", &jcontent);
        
        const char *role = json_object_get_string(jrole);
        const char *content = json_object_get_string(jcontent);
        if (!role || !content) continue;

        print_box();
        if (strcmp(role, "user") == 0) {
            printf(COLOR_USER "\n[%s] You:\n" COLOR_RESET, ChatName);
        } else if (strcmp(role, "assistant") == 0) {
            printf(COLOR_MODEL "\n[%s] %s:\n" COLOR_RESET, ChatName, ModelName);
        } else {
            printf(COLOR_SYSTEM "\n[%s] llorch:\n" COLOR_RESET, ToolName);
        }
        printf("%s\n", content);
    }
}

void modelans() {
    print_box();
    printf(COLOR_MODEL "[%s] %s:\n" COLOR_RESET, ChatName, ModelName);
}

void sysmsg(const char* text, ...) {
    va_list ap; va_start(ap, text);
    printf(COLOR_SYSTEM); vprintf(text, ap); printf(COLOR_RESET);
    fflush(stdout); va_end(ap);
}

void syserr(const char* text, ...) {
    va_list ap; va_start(ap, text);
    printf(COLOR_ERROR); vprintf(text, ap); printf(COLOR_RESET);
    fflush(stdout); va_end(ap);
}
