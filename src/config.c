#include "config.h"
#include "commands.h"
#include "json-c/json.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

cfg config = {
    .models_dir = "~/.local/share/llorch/Models",
    .chats_dir = "~/.local/share/llorch/Chats",
    .port = 8080,
    .n_ctx = 16384,
    .reserve = 2048,
    .server_flags = NULL,
    .flags_count = 0
};

bool cfgload() {
    char path[4096];
    if (resolve("~/.config/llorch/config.json", path, sizeof(path)) != 0) return false;

    struct json_object *root = json_object_from_file(path);
    if (!root) {
        char rootdir[4096];
        if (resolve("~/.config/llorch", rootdir, sizeof(rootdir)) != 0) return ERR;
        mkdir(rootdir, 0755);

        FILE* cfgfile = fopen(path, "w");
        fclose(cfgfile);
        return true;
    }

    struct json_object *j;
    if (json_object_object_get_ex(root, "port", &j) && json_object_is_type(j, json_type_int)) config.port = json_object_get_int(j);
    if (json_object_object_get_ex(root, "n_ctx", &j) && json_object_is_type(j, json_type_int)) config.n_ctx = json_object_get_int(j);
    if (json_object_object_get_ex(root, "reserve", &j) && json_object_is_type(j, json_type_int)) config.reserve = json_object_get_int(j);
    if (json_object_object_get_ex(root, "models_dir", &j) && json_object_is_type(j, json_type_string)) {
        if (resolve(json_object_get_string(j), config.models_dir, sizeof(config.models_dir)) != 0) return ERR;
        mkdir(config.models_dir, 0755);
    }
    if (json_object_object_get_ex(root, "chats_dir", &j) && json_object_is_type(j, json_type_string)) {
        if (resolve(json_object_get_string(j), config.chats_dir, sizeof(config.chats_dir)) != 0) return ERR;
        mkdir(config.chats_dir, 0755);
    }
    if (json_object_object_get_ex(root, "server_flags", &j) && json_object_is_type(j, json_type_array)) {
        int n = json_object_array_length(j);
        config.server_flags = calloc(n + 1, sizeof(char*));
        for (int i = 0; i < n; i++) {
            struct json_object *s = json_object_array_get_idx(j, i);
            if (json_object_is_type(s, json_type_string))
                config.server_flags[i] = strdup(json_object_get_string(s));
        }
        config.flags_count = n;
    }

    json_object_put(root);
    return true;
}

void cfgfree(cfg* c) {
    if (!c) return;

    if (c->server_flags != NULL) {
        for (int i = 0; i < c->flags_count; i++) {
            free(c->server_flags[i]);
        }
        free(c->server_flags);

        c->server_flags = NULL;
        c->flags_count = 0;
    }
}

