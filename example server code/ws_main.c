// chat_server.c

#include "external/civetweb/include/civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>


#define PORT "8080"

struct mg_context *ctx;
struct mg_connection *clients[10];
int client_count = 0;

void broadcast(const char *msg, size_t len) {
    for (int i = 0; i < client_count; ++i) {
        mg_websocket_write(clients[i], MG_WEBSOCKET_OPCODE_TEXT, msg, len);
    }
}

int ws_connect(const struct mg_connection *conn, void *cbdata) {
    if (client_count < 10) {
        clients[client_count++] = (struct mg_connection *)conn;
        printf("Client connected. Total: %d\n", client_count);
        return 0;  // CAREFUL!! ACCEPT ON 0. All else, accept on 1 and above.
    }
    return 1;  // Reject if full
}
/* Callback types for websocket handlers in C/C++.

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
       Is called, when the connection is closed.*/

void ws_ready(struct mg_connection *conn, void *cbdata) {
    const char *msg = "Connected to chat server!";
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, msg, strlen(msg));
}

int ws_data(struct mg_connection *conn, int opcode, char *data, size_t len, void *cbdata) {
    printf("Message from client: %.*s\n", (int)len, data);
    broadcast(data, len);  // Send message to all clients
    return 1;  // keep connection open. 0 means close it.
}

void ws_close(const struct mg_connection *conn, void *cbdata) {
    // Remove the connection from the client list
    for (int i = 0; i < client_count; ++i) {
        if (clients[i] == conn) {
            for (int j = i; j < client_count - 1; ++j) {
                clients[j] = clients[j + 1];
            }
            --client_count;
            printf("Client disconnected. Total: %d\n", client_count);
            break;
        }
    }
}

static int ChatPageHandler(struct mg_connection *conn, void *cbdata) {
    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *upgrade = mg_get_header(conn, "Upgrade");
    if (upgrade && !mg_strcasecmp(upgrade, "websocket")) {
        // Let CivetWeb handle the WebSocket handshake
        return 0;
    }
    printf("Page handler called\n");
    FILE *f = fopen("chat.html", "rb");
    if (!f) {
        mg_send_http_error(conn, 404, "%s", "File not found");
        return 404;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    mg_send_http_ok(conn, "text/html", (size_t)len);
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        mg_write(conn, buf, n);
    }
    fclose(f);
    return 200;
}

int main() {
    const char *options[] = {
        "listening_ports", PORT,
        "document_root", ".",  // Serve HTML from current directory
        0
    };

    struct mg_callbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));

    ctx = mg_start(&callbacks, 0, options);

    if (!ctx) {
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }
    // mg_set_request_handler(ctx, "/chat", ChatPageHandler, 0);

    mg_set_websocket_handler(ctx, "/chat",
                             ws_connect,
                             ws_ready,
                             ws_data,
                             ws_close,
                             0);

    printf("Chat server running at http://localhost:%s\n", PORT);
    while (1) {
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    mg_stop(ctx);
    return 0;
}
