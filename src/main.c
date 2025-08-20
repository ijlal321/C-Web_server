
#include <windows.h>

#include "civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "server.h"
#include "web_socket.h"


int main() {
    struct mg_context *ctx = initialize_server();
    if (!ctx) {
        printf("Failed to start CivetWeb server\n");
        return 1;
    }

    init_handlers(ctx); // point to web file for different routes

    printf("Server listening on http://localhost:%s/\n", PORT);

    struct ws_manager ws_mgr;
    ws_manager_init(&ws_mgr);

    ws_start(ctx, &ws_mgr);


    getchar();
    mg_stop(ctx);
    return 0;
}