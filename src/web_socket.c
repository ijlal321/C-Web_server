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

    // accept all connections
    return 0;

    return 1;  // Reject connection
}

void ws_ready(struct mg_connection *conn, void *cbdata) {

    // send a new clientId to client
    // mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, msg, strlen(msg));
    
    // printf("Connected to client. Tota; Clients: %d\n", HASH_COUNT(connection_mgr->clients));
}

int ws_data(struct mg_connection *conn, int con_opcode, char *data, size_t len, void *cbdata) {
    
    struct AppContext * app_ctx = (struct AppContext *)cbdata;
    struct ConnectionManager * connection_mgr = &app_ctx->connection_mgr;

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

    // get our Custom Opcode {from JSON}
    const cJSON * opcode = cJSON_GetObjectItem(root, "opcode");
    const cJSON * ws_data = cJSON_GetObjectItem(root, "data");
    if (!cJSON_IsNumber(opcode) || !cJSON_IsObject(ws_data) ) {
        printf("Invalid Structure of Websocket Request.'\n");
        cJSON_Delete(root);
        return 1;
    }

    enum WsOPCodes op = (enum WsOPCodes)opcode->valueint;
    // check if opcode is register 
    // Special Case: 
    // 1. where we dont need to find client instance
    // 2. special need for write lock.
    if (op == REGISTER){
        int public_id = cm_add_client(connection_mgr, conn, ws_data);
        if (public_id == 0){ // some error occured

            return 0;  // close connection.
        }
        char buffer[500];
        sprintf(buffer, "{\"opcode\":%d, \"data\":{\"public_id\":%d}}", PUBLIC_NAME, public_id);
        mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));
        cJSON_Delete(root);
        return 1;
    }

    // SCENERIOS: We need READ LOCK on CONNECTION MANAGER
    pthread_rwlock_rdlock(&connection_mgr->rwlock);

    switch (op) {
        case ADD_FILES:
            // Handle Adding new files in client files
            printf("Handling ADD_FILES\n");
            break;
        case REMOVE_FILE:
            // Handle removing files from client files
            printf("Handling REMOVE_FILE\n");
            break;
        case ASK_FILES:
            // Client Asking all Server Files
            printf("Handling ASK_FILES\n");
            break;
        case UPDATE_NAME:
            // Handle asking server ti update client Public Name
            printf("Handling ASK_FILES\n");
            break;
        case RECONNECT:
            printf("Handling ASK_FILES\n");
            break;
        // more later
        default:
            printf("Unknown opcode: %d\n", op);
            break;
    }

    pthread_rwlock_unlock(&connection_mgr->rwlock);

    cJSON_Delete(root);
    return 1;


    printf("Message from client: %.*s\n", (int)len, data);
    return 1;  // keep connection open. 0 means close it.
    
}

void ws_close(const struct mg_connection *conn, void *cbdata) {
    struct AppContext *app_ctx = (struct AppContext *)cbdata;
    struct ConnectionManager * connection_mgr = &app_ctx->connection_mgr;

    printf("Client disconnected. Total: %d\n", HASH_COUNT(connection_mgr->clients));
}
 

// =============================

int ws_manager_destroy(){
    // will be needing this when mutexing connection manager. 
    // pthread_mutex_destroy(&ws_mgr->lock);
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