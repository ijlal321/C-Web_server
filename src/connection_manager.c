#include "http_server.h"

#include "app_context.h"  // Global Context

void cm_init(struct ConnectionManager * connection_mgr){
    if (pthread_rwlock_init(&connection_mgr->rwlock, NULL) != 0){
        printf("Error Creating RW_lock\n");
        exit(0);
    }
}

void * start_connections(void * args){
    struct AppContext * app_ctx = (struct AppContext *)args;

    // ================== HTTP =====================//
    struct mg_context *ctx = http_initialize_server();
    if (!ctx) {
        printf("Failed to start CivetWeb server\n");
        exit(1);
    }

    http_init_handlers(ctx);

    printf("HTTP Server listening on http://localhost:%s/\n", PORT);

    // ================= Websocket and FIle_tracker ============== 

    ws_start(ctx, app_ctx); // need ws_mgr and file_mgr for tracking files with ws
    printf("WebSocket listening on http://localhost:%s%s\n", PORT, WEB_SOCKET_PATH);

    getchar();
    mg_stop(ctx);
    return NULL;
}


struct Client * cm_add_client(struct ConnectionManager * connection_mgr, struct mg_connection *conn){
    pthread_rwlock_wrlock(&connection_mgr->rwlock);
    struct Client * new_client = malloc(sizeof(struct Client)); 
    new_client->id = connection_mgr->id;
    new_client->approved = 0;
    new_client->conn = conn;
    pthread_rwlock_init(&new_client->rwlock, NULL);
    HASH_ADD_
}
