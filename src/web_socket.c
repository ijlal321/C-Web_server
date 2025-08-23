#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <pthread.h>
#include <cJSON.h> 

#include "web_socket.h"
#include "app_context.h"

/*
        SOME IMPORTANT INFO ON WS FUNCTIONS

    Callback types for websocket handlers in C/C++.

   mg_websocket_connect_handler
       Is called when the client intends to establish a websocket connection,
       before websocket handshake.
       Return value:
         0: civetweb proceeds with websocket handshake.
         1: connection is closed immediately.

   mg_websocket_ready_handler
       Is called when websocket handshake is successfully completed, and
       connection is ready for data exchange.

   mg_websocket_data_handler
       Is called when a data frame has been received from the client.
       Parameters:
         bits: first byte of the websocket frame, see websocket RFC at
               http://tools.ietf.org/html/rfc6455, section 5.2
         data, data_len: payload, with mask (if any) already applied.
       Return value:
         1: keep this websocket connection open.
         0: close this websocket connection.

   mg_connection_close_handler
       Is called, when the connection is closed.

*/


// ================= WS HANDLERS ===============

int ws_connect(const struct mg_connection *conn, void *cbdata) {
    struct AppContext *app_ctx = (struct AppContext *)cbdata;
    struct WsManager *ws_mgr = &app_ctx->ws_mgr;

    // Check if space is available — but don't store yet
    if (ws_mgr->client_count < MAX_WEB_SOCKET_CLIENTS) {
        return 0;  // Accept connection
    }

    return 1;  // Reject connection
}

void ws_ready(struct mg_connection *conn, void *cbdata) {
    struct AppContext *app_ctx = (struct AppContext *)cbdata;
    struct WsManager *ws_mgr = &app_ctx->ws_mgr;

    ws_mgr->clients[ws_mgr->client_count++] = conn;
    printf("Connected to client. Tota; Clients: %d\n", ws_mgr->client_count);
    // const char *msg = "Connected to !";
    // mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, msg, strlen(msg));
}

int ws_data(struct mg_connection *conn, int con_opcode, char *data, size_t len, void *cbdata) {
    return 1;
    struct AppContext * app_ctx = (struct AppContext *)cbdata;
    struct WsManager * ws_mgr = &app_ctx->ws_mgr;
    struct FileTracker * file_tracker = &app_ctx->file_tracker;
    printf("Some client sent data: %.*s\n", (int)len, data);
    printf("Connection Opcode is: %d\n", con_opcode);
    
    // Ignore anything that's not a text frame
    // last 4 bits are used to represent opcode.
    if ((con_opcode & 0x0F) != MG_WEBSOCKET_OPCODE_TEXT) {
        printf("Ignoring non-text frame (opcode %d)\n", con_opcode);
        return 1;
    }
    
    // convert data to json
    cJSON *root = cJSON_Parse(data);
    if (!root) {
        printf("Not In JSON. Ignore.\n");
        return -1;
    }

    // get Opcode
    const cJSON * opcode = cJSON_GetObjectItem(root, "opcode");
    if (!cJSON_IsNumber(opcode)) {
        printf("Missing or invalid 'opcode'\n");
        cJSON_Delete(root);
        return 1;
    }

    enum WsOPCodes op = (enum WsOPCodes)opcode->valueint;


    // find client who is sending data in accepted clients
    // and lock it.
    pthread_mutex_lock(&file_tracker->lock);
    struct ClientFiles * cur_client = search_client_in_accepted_clients(conn, file_tracker);
    if (cur_client == NULL){
        printf("Non Connected Client is trying to talk, hmmm,,,,\n");
        pthread_mutex_unlock(&file_tracker->lock);
        return;
    }
    pthread_mutex_lock(&cur_client->lock);  // lock client before unlocking manager.
    pthread_mutex_unlock(&file_tracker->lock);

    switch (op) {
        case ADD_FILES:
            // Handle adding files
            printf("Handling ADD_FILES\n");
            // TODO: Add your logic here
            break;
        case REMOVE_FILE:
            // Handle removing a file
            printf("Handling REMOVE_FILE\n");
            // TODO: Add your logic here
            break;
        case ASK_FILES:
            // Handle ASKING SERVER for all his files
            printf("Handling ASK_FILES\n");
            // TODO: Add your logic here
            break;
        // more later
        default:
            printf("Unknown opcode: %d\n", op);
            break;
    }

    // unlock lock on Client
    pthread_mutex_unlock(&cur_client->lock); 
    cJSON_Delete(root);
    return 1;


    printf("Message from client: %.*s\n", (int)len, data);
    return 1;  // keep connection open. 0 means close it.
}

void ws_close(const struct mg_connection *conn, void *cbdata) {
    struct AppContext * app_ctx = (struct AppContext *)cbdata;
    struct WsManager * ws_mgr = &app_ctx->ws_mgr;
    // Remove the connection from the client list
    for (int i = 0; i < ws_mgr->client_count; ++i) {
        if (ws_mgr->clients[i] == conn) {
            for (int j = i; j < ws_mgr->client_count - 1; ++j) {
                ws_mgr->clients[j] = ws_mgr->clients[j + 1];
            }
            --ws_mgr->client_count;
            printf("Client disconnected. Total: %d\n", ws_mgr->client_count);
            break;
        }
    }
}
 

// =============================

int ws_manager_init(struct WsManager * ws_mgr){
    memset(ws_mgr, 0, sizeof(struct WsManager));
    if (pthread_mutex_init(&ws_mgr->lock, NULL) != 0){
        return -1;
    }
    return 0;
}

int ws_manager_destroy(struct WsManager * ws_mgr){
    // will be needing this when mutexing connection manager. 
    pthread_mutex_destroy(&ws_mgr->lock);
    return 0;
}

void ws_start(struct mg_context * ctx, struct AppContext * app_ctx){
    mg_set_websocket_handler(ctx, WEB_SOCKET_PATH,
                            ws_connect,
                            ws_ready,
                            ws_data,
                            ws_close,
                            (void *)app_ctx);
}