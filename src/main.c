#include "server.h"
#include "web_socket.h"
#include "file_tracker.h"

#include "app_context.h"  // Global Context

int main() {
    struct mg_context *ctx = initialize_server();
    if (!ctx) {
        printf("Failed to start CivetWeb server\n");
        return 1;
    }

    init_handlers(ctx);

    printf("HTTP Server listening on http://localhost:%s/\n", PORT);

    struct AppContext app_ctx = {0};  // zero-initialize

    ws_manager_init(&app_ctx.ws_mgr);  //  TODO: Error Handling
    file_tracker_init(&app_ctx.file_tracker);  //  TODO: Error Handling

    ws_start(ctx, &app_ctx); // need ws_mgr and file_mgr for tracking files with ws
    printf("WebSocket listening on http://localhost:%s%s\n", PORT, WEB_SOCKET_PATH);

    getchar();
    mg_stop(ctx);
    return 0;
}
