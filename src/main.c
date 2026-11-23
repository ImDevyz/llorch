#include <string.h>
#include "ui.h"
#include "server.h"

char *ChatName  = NULL;
char *ToolName  = NULL;
char *ModelName = NULL;

int main(void) {
    ChatName  = strdup("None");
    ToolName  = strdup("System");
    ModelName = strdup("Model");

    ui_start();

    server_stop();
    return 0;
}
