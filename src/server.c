#include "server.h"
#include "commands.h"
#include "ui.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <errno.h>

pid_t server_pid = -1;
struct json_object *messages = NULL;

static volatile sig_atomic_t gen_abort = 0;

extern char *ModelName;
extern char *ChatName;
extern char *ToolName;
extern cfg config;

char* lh = "http://127.0.0.1:";

static void on_sigint(int s) {
    (void)s;
    gen_abort = 1;
}

static int cb_progress(void *u, curl_off_t a, curl_off_t b, curl_off_t c, curl_off_t d) {
    (void)u;(void)a;(void)b;(void)c;(void)d;
    return gen_abort ? 1 : 0;
}

struct Stream {
    char *line;
    size_t llen;
    size_t lcap;
    char *ans;
    size_t alen;
    size_t acap;
    int thinking;
};

static size_t cbstream(char *ptr, size_t size, size_t nmemb, void *ud) {
    if (gen_abort) return 0;
    struct Stream *s = ud;
    size_t total = size * nmemb;

    for (size_t i = 0; i < total; i++) {
        if (ptr[i] != '\n') {
            if (s->llen + 2 > s->lcap) {
                size_t nc = s->lcap ? s->lcap * 2 : 256;
                char *p = realloc(s->line, nc);
                if (!p) return 0;
                s->line = p; s->lcap = nc;
            }
            s->line[s->llen++] = ptr[i];
            s->line[s->llen] = '\0';
            continue;
        }

        if (s->llen && s->line[s->llen-1] == '\r') s->line[--s->llen] = '\0';
        char *ln = s->line ? s->line : "";

        if (strncmp(ln, "data:", 5) == 0) {
            char *p = ln + 5;
            if (*p == ' ') p++;
            struct json_object *j = NULL, *ch, *d, *t;
            if (strcmp(p, "[DONE]") != 0) j = json_tokener_parse(p);

            if (j && json_object_object_get_ex(j, "choices", &ch) &&
                json_object_array_length(ch) > 0 &&
                json_object_object_get_ex(json_object_array_get_idx(ch, 0), "delta", &d)) {

                if (json_object_object_get_ex(d, "reasoning_content", &t)) {
                    const char *v = json_object_get_string(t);
                    if (v && *v) {
                        if (!s->thinking) {
                            s->thinking = 1;
                        }
                        print(v, true);
                        fflush(stdout);
                    }
                }

                if (json_object_object_get_ex(d, "content", &t)) {
                    const char *v = json_object_get_string(t);
                    if (v && *v) {
                        if (s->thinking) {
                            print("\n\n", true);
                            s->thinking = 0;
                        }
                        print(v, false);
                        fflush(stdout);
                        size_t n = strlen(v);
                        if (s->alen + n + 1 > s->acap) {
                            size_t nc = s->acap ? s->acap : 1024;
                            while (nc < s->alen + n + 1) nc *= 2;
                            char *np = realloc(s->ans, nc);
                            if (!np) {
                                json_object_put(j);
                                return 0;
                            }
                            s->ans = np; s->acap = nc;
                        }
                        memcpy(s->ans + s->alen, v, n);
                        s->alen += n; s->ans[s->alen] = '\0';
                    }
                }
            }
            if (j) json_object_put(j);
        }
        s->llen = 0;
        if (s->line) s->line[0] = '\0';
    }
    return total;
}

void server_stop(void) {
    if (server_pid <= 0) return;
    int status;
    if (waitpid(server_pid, &status, WNOHANG) == 0) {
        sysmsg("There is an old server on PID %d. Killing...\n", server_pid);
        kill(server_pid, SIGTERM);
        waitpid(server_pid, NULL, 0);
    }
    server_pid = -1;
}

sc load_model(char *path, char* nil) {
    (void)nil;
    server_stop();
    if (!path || !*path) {
        syserr("Invalid arguments. Type /help if you have troubles\n");
        return ERR;
    }

    char rpath[4096];
    if (path[0] == '~') {
        if (resolve(path, rpath, sizeof(rpath)) != 0) return ERR;
    } else if (path[0] == '/') {
        snprintf(rpath, sizeof(rpath), "%s", path);
    } else {
        char modeldir[4096];
        if (resolve("~/.local/share/llorch/Models", modeldir, sizeof(modeldir)) != 0) return ERR;
        snprintf(rpath, sizeof(rpath), "%s/%s.gguf", modeldir, path);
    }

    sysmsg("Loading model from %s...\n", rpath);
    free(ModelName);
    ModelName = strdup(path);

    if (access(rpath, R_OK) != 0) {
        syserr("Cannot reach %s: %s\n", rpath, strerror(errno));
        return ERR;
    }
    pid_t pid = fork();
    if (pid < 0) return ERR;
    if (pid == 0) {
        freopen("/tmp/llorch-server-out.log", "w", stdout);
        freopen("/tmp/llorch-server-err.log", "w", stderr);

        char port_str[16], ctx_str[16];
        snprintf(port_str, sizeof(port_str), "%d", config.port);
        snprintf(ctx_str, sizeof(ctx_str), "%d", config.n_ctx);

        char *args[64];
        int n = 0;
        args[n++] = "llama-server";
        args[n++] = "-m";     args[n++] = rpath;
        args[n++] = "--port"; args[n++] = port_str;
        args[n++] = "-c";     args[n++] = ctx_str;

        for (int i = 0; i < config.flags_count && n < 62; i++)
            if (config.server_flags[i]) args[n++] = config.server_flags[i];

        args[n] = NULL;
        execvp("llama-server", args);
        _exit(1);
    }

    server_pid = pid;
    for (int i = 0; i < 60; i++) {
        sleep(1);
        CURL *curl = curl_easy_init();
        if (!curl) {
            syserr("Error while making curl request: %s\n", strerror(errno));
            return ERR;
        }
        char hch[256];
        snprintf(hch, sizeof(hch), "%s%d/health", lh, config.port);
        curl_easy_setopt(curl, CURLOPT_URL, hch);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        int code = 0;
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        else if (res == CURLE_COULDNT_CONNECT) code = 503;
        curl_easy_cleanup(curl);

        if (code == 200) {
            sysmsg("Server load succesfully on PID %d\n", server_pid);
            return OK;
        }
        if (code != 503) {
            syserr("Undefined status code while starting server: %d\n", code);
            server_stop();
            return ERR;
        }
    }
    server_stop();
    syserr("Server is not responding\n");
    return ERR;
}

sc hchat(char *text) {
    if (server_pid <= 0) {
        sysmsg("Server isn't running. Use /load <ModelName>\n");
        return ERR;
    }

    if (messages == NULL) {
        chopen("NewChat", NULL);
        if (messages == NULL) messages = json_object_new_array();
    }

    chat_append("user", text);

    struct json_object *root = json_object_new_object();
    json_object_object_add(root, "messages", json_object_get(messages));
    json_object_object_add(root, "model", json_object_new_string(ModelName));
    json_object_object_add(root, "stream", json_object_new_boolean(1));

    const char *json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);

    CURL *curl = curl_easy_init();
    if (!curl) { 
        syserr("Error while making curl request: %s\n", strerror(errno));
        json_object_put(root);
        return ERR;
    }

    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
    struct Stream state = {0};

    modelans();

    char api[256];
    snprintf(api, sizeof(api), "%s%d/v1/chat/completions", lh, config.port);
    curl_easy_setopt(curl, CURLOPT_URL, api);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cbstream);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cb_progress);

    struct termios tio_old, tio_new;
    int tio_ok = (tcgetattr(STDIN_FILENO, &tio_old) == 0);
    if (tio_ok) {
        tio_new = tio_old;
        tio_new.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &tio_new);
    }

    gen_abort = 0;
    struct sigaction sa = {0}, old;
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, &old);

    CURLcode res = curl_easy_perform(curl);

    sigaction(SIGINT, &old, NULL);
    if (tio_ok) tcsetattr(STDIN_FILENO, TCSANOW, &tio_old);
    tcflush(STDIN_FILENO, TCIFLUSH);

    if (res != CURLE_OK && !gen_abort) {
        syserr("Error while performing curl request to model: %s\n", curl_easy_strerror(res));
        free(state.line);
        free(state.ans);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        json_object_put(root);
        return ERR;
    }
    if (state.ans && state.alen > 0) chat_append("assistant", state.ans);

    free(state.line);
    free(state.ans);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    json_object_put(root);
    return OK;
}

void chat_append(const char *role, const char *content) {
    json_object *msg = json_object_new_object();
    json_object_object_add(msg, "role", json_object_new_string(role));
    json_object_object_add(msg, "content", json_object_new_string(content));
    json_object_array_add(messages, json_object_get(msg));

    char chatdir[4096];
    if (resolve("~/.local/share/llorch/Chats", chatdir, sizeof(chatdir)) == 0) {
        char file_path[4096];
        snprintf(file_path, sizeof(file_path), "%s/%s.jsonl", chatdir, ChatName);
        FILE *f = fopen(file_path, "a");
        if (f) {
            fprintf(f, "%s\n", json_object_to_json_string_ext(msg, JSON_C_TO_STRING_PLAIN));
            fclose(f);
        }
    }
    json_object_put(msg);
}
