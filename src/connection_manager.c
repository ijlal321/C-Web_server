#include "http_server.h"

#include "app_context.h"  // Global Context

#include <cJSON.h>

void cm_init(struct ConnectionManager * connection_mgr){
    if (pthread_rwlock_init(&connection_mgr->rwlock, NULL) != 0){
        printf("Error Creating RW_lock\n");
        exit(0);
    }
}

void * start_connections(void * args){
    struct AppContext * app_ctx = (struct AppContext *)args;
    // ================== HTTP =====================//
    struct mg_context * cw_ctx = http_initialize_server();
    if (!cw_ctx) {
        printf("Failed to start CivetWeb server\n");
        exit(1);
    }

    http_init_handlers(cw_ctx, app_ctx);

    printf("HTTP Server listening on http://localhost:%s/\n", PORT);

    // ================= Websocket and FIle_tracker ============== 

    ws_start(cw_ctx, app_ctx); // need ws_mgr and file_mgr for tracking files with ws
    printf("WebSocket listening on http://localhost:%s%s\n", PORT, WEB_SOCKET_PATH);

    getchar();
    mg_stop(cw_ctx);
    return NULL;
}


int cm_add_client(struct ConnectionManager * connection_mgr, struct mg_connection *conn, cJSON *data){
    pthread_rwlock_wrlock(&connection_mgr->rwlock);

    const cJSON * private_key = cJSON_GetObjectItem(data, "private_key");
    if (!cJSON_IsString(private_key)){
        printf("Private Key not available\n");
        return 0; // meaning close connection with this one.
    }
    
    struct Client * new_client = malloc(sizeof(struct Client));
    
    strncpy(new_client->private_id, private_key->valuestring, PRIVATE_ID_SIZE-1);
    new_client->private_id[PRIVATE_ID_SIZE-1] = '\0';

    new_client->conn = conn;
    new_client->approved = 0;
    int public_id = HASH_COUNT(connection_mgr->clients) + 1;
    new_client->public_id = public_id;
    pthread_rwlock_init(&new_client->rwlock, NULL);

    HASH_ADD_INT(connection_mgr->clients, public_id, new_client);

    pthread_rwlock_unlock(&connection_mgr->rwlock);
    return public_id; // everything Okay. Keep Connection Open.
}
