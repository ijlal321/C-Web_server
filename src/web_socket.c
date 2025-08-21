#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "web_socket.h"

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

void broadcast(const char *msg, size_t len, struct WsClientManager * ws_mgr) {
    for (int i = 0; i < ws_mgr->client_count; ++i) {
        mg_websocket_write(ws_mgr->clients[i], MG_WEBSOCKET_OPCODE_TEXT, msg, len);
    }
}


// ================= WS HANDLERS ===============

int ws_connect(const struct mg_connection *conn, void *cbdata) {
    struct WsClientManager * ws_mgr = (struct WsClientManager *)cbdata;
    if (ws_mgr->client_count < 10) {
        ws_mgr->clients[ws_mgr->client_count++] = (struct mg_connection *)conn;
        printf("Client connected. Total: %d\n", ws_mgr->client_count);
        return 0;  // CAREFUL!! ACCEPT ON 0. All else, accept on 1 and above.
    }
    return 1;  // Reject if full
}

void ws_ready(struct mg_connection *conn, void *cbdata) {
    const char *msg = "Connected to chat server!";
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, msg, strlen(msg));
}

int ws_data(struct mg_connection *conn, int opcode, char *data, size_t len, void *cbdata) {
    struct WsClientManager * ws_mgr = (struct WsClientManager *)cbdata;
    printf("Message from client: %.*s\n", (int)len, data);
    broadcast(data, len, ws_mgr);  // Send message to all clients
    return 1;  // keep connection open. 0 means close it.
}

void ws_close(const struct mg_connection *conn, void *cbdata) {
    struct WsClientManager * ws_mgr = (struct WsClientManager *)cbdata;
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

int ws_manager_init(struct WsClientManager * ws_mgr){
    memset(ws_mgr, 0, sizeof(struct WsClientManager));
    return 0;
}

int ws_manager_destroy(struct WsClientManager * ws_mgr){
    // will be needing this when mutexing connection manager. 
    return 0;
}

void ws_start(struct mg_context * ctx, struct WsClientManager * ws_mgr){
    mg_set_websocket_handler(ctx, WEB_SOCKET_PATH,
                            ws_connect,
                            ws_ready,
                            ws_data,
                            ws_close,
                            (void *)ws_mgr);
}