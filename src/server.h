#pragma once
#include <json-c/json.h>
#include <sys/types.h>
#include "ui.h"

extern pid_t server_pid;
extern struct json_object* messages;

sc load_model(char* path, char* arg2);
void server_stop();
sc hchat(char* text);
void chat_append(const char* role, const char* content);
