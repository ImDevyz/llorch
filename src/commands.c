#include <stdlib.h>
#include "commands.h"
#include "server.h"
#include "config.h"
#include "ui.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

extern char *ChatName;
extern char *ToolName;
extern char *ModelName;
extern cfg config;

sc mkchat(char* name, char* arg2) {
    (void)arg2;
    if (!name || !*name) {
        syserr("Invalid arguments. Type /help if you have troubles\n");
        return ERR;
    }

    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/%s.jsonl", config.chats_dir, name);

    FILE* file = fopen(file_path, "wx");
    if (file) {
        fclose(file);
        sysmsg("Succesfuly created\nUse /open to start chatting in it\n");
        return OK;
    } else if (errno == EEXIST) {
        sysmsg("Chat with the same name already exist\n");
        return OK;
    } else {
        syserr("Cannot create %s: %s\n", file_path, strerror(errno));
        return ERR;
    }
    sysmsg("Successfully created\nUse /open to start chatting in it\n");
    return OK;
}

sc rmchat(char *name, char* arg2) {
    (void)arg2;
    if (!name || !*name) {
        syserr("Invalid arguments. Type /help if you have troubles\n");
        return ERR;
    }

    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/%s.jsonl", config.chats_dir, name);

    if (remove(file_path) == 0) {
        sysmsg("Succesfuly removed\n");
        return OK;
    } else if (errno == ENOENT) {
        if (strcmp(name, "NewChat") != 0) {
            syserr("Chat with this name is not exist\n");
            return ERR;
        }
    } else {
        syserr("Cannot remove %s: %s\n", file_path, strerror(errno));
        return ERR;
    }
    sysmsg("Succesfuly removed\n");
    return OK;
}

sc chopen(char *name, char *arg2) {
    (void)arg2;
    if (!name || !*name) {
        syserr("Invalid arguments. Type /help if you have troubles\n");
        return ERR;
    }

    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/%s.jsonl", config.chats_dir, name);

    FILE *chat = fopen(file_path, "r");
    if (strcmp(name, "NewChat") == 0) {
        if (chat) {
            fclose(chat);
            remove(file_path);
        }
        chat = fopen(file_path, "w+");
        if (!chat) {
            syserr("Cannot configure anonymous chat %s: %s\n", file_path, strerror(errno));
            return ERR;
        }
        fclose(chat);
        chat = fopen(file_path, "r");
    } else if (!chat) {
        syserr("Chat with this name is not exist\n");
        return ERR;
    }

    if (messages) json_object_put(messages);
    messages = json_object_new_array();

    char line[8192];
    while (fgets(line, sizeof(line), chat)) {
        json_object *item = json_tokener_parse(line);
        if (item) json_object_array_add(messages, item);
    }
    fclose(chat);

    free(ChatName);
    ChatName = strdup(name);
    print_chat();
    return OK;
}

sc resolve(const char *path, char *out, int out_size) {
    if (path[0] == '~') {
        const char *home = getenv("HOME");
        if (!home) {
            syserr("$HOME not set\n");
            return ERR;
        }
        snprintf(out, out_size, "%s%s", home, path + 1);
    } else {
        snprintf(out, out_size, "%s", path);
    }
    return OK;
}

sc renchat(char* OldName, char* NewName) {
    if ((!OldName || !*OldName) || (!NewName || !*NewName)) {
        syserr("Invalid arguments. Type /help if you have troubles\n");
        return ERR;
    }

    char old_file[4096], new_file[4096];
    snprintf(old_file, sizeof(old_file), "%s/%s.jsonl", config.chats_dir, OldName);
    snprintf(new_file, sizeof(new_file), "%s/%s.jsonl", config.chats_dir, NewName);

    FILE* chat = fopen(old_file, "r");
    if (!chat && errno == ENOENT) {
        syserr("Chat with this name is not exist\n");
        return ERR;
    } else if (!chat) {
        syserr("Cannot rename %s: %s\n", old_file, strerror(errno));
        return ERR;
    }
    fclose(chat);

    if (rename(old_file, new_file) == 0) {
        sysmsg("Succesfuly renamed\n");
        return OK;
    } else {
        syserr("Cannot rename %s: %s\n", old_file, strerror(errno));
        return ERR;
    }
    sysmsg("Succesfuly renamed\n");
    return OK;
}

sc addmodel(char* gguf_path, char* name) {
    if ((!name || !*name) || (!gguf_path || !*gguf_path)) {
        syserr("Invalid arguments. Type /help if you have troubles\n");
        return ERR;
    }

    char modeldir[4096], link_path[4096];
    snprintf(link_path, sizeof(link_path), "%s/%s.gguf", modeldir, name);
    
    char abs[4096];
    if (!realpath(gguf_path, abs)) {
        syserr("Cannot resolve %s: %s\n", gguf_path, strerror(errno));
        return ERR;
    }
    if (symlink(abs, link_path) != 0) {
        syserr("Cannot save %s: %s\n", name, strerror(errno));
        return ERR;
    }
    sysmsg("Saved\n");
    return OK;
}

sc pexit(char* nil1, char* nil2) {
    (void)nil1; (void)nil2;
    server_stop();
    rmchat("NewChat", NULL);
    return QUIT;
}
