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

// ======= Helper functions Headers =====//

struct WsMsgHeader {
    int opcode;
    cJSON *data;
} ;

int ws_opcode_is_text(int con_opcode);
cJSON * parse_JSON_to_CJSON_root(char * data);
struct WsMsgHeader ws_get_msg_headers(cJSON *root);


// =============================================


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
    
    // check opCode - Accept Only Text Here
    if (ws_opcode_is_text(con_opcode) != 1){
        return 1;
    }
    
    // Parse json Message (So we can see its contents)
    cJSON *root = parse_JSON_to_CJSON_root(data);
    if (root == NULL) {
        return 1;
    }

    // Get Opcode and data fields from message
    struct WsMsgHeader ws_msg_header = ws_get_msg_headers(root);
    if (ws_msg_header.data == NULL){
        cJSON_Delete(root);
        return 1;
    }
    
    // take fields out of struct.
    const cJSON * ws_data = ws_msg_header.data;
    enum WsOPCodes op = (enum WsOPCodes)ws_msg_header.opcode;

    switch (op) {
        case CLIENT_REGISTER:
            struct Client * client = cm_add_client(connection_mgr, conn, ws_data);
            if (client == NULL){ // some error occured
                return 1;  // 0 means close connection. we are not rn.
            }
            cm_send_public_id_to_client(conn, client->public_id);
            cm_add_client_to_UI(&connection_mgr->server, client);
            break;
        case UI_REGISTER:
            cm_register_server(connection_mgr, conn);
            break;
        case UI_APPROVE_CLIENT:
            // TODO: alot of refactoring needed in this section. 2 times lock, finding, parsing public id...
            cm_approve_client(connection_mgr, ws_data);
            cm_notify_client_approved(connection_mgr, ws_data);
            printf("approve client called\n");
            break;
        case UI_DIS_APPROVE_CLIENT:
            cm_disapprove_client(connection_mgr, ws_data);
            cm_notify_client_disapproved(connection_mgr, ws_data);
            printf("UI_DIS_APPROVE_CLIENT client called\n");
            break;
        case CLIENT_ADD_FILES:
            // Handle Adding new files in client files
            cm_add_files(connection_mgr, ws_data);
            cm_send_files_to_UI(&connection_mgr->server, root);
            printf("Handling ADD_FILES\n");
            break;
        case CLIENT_REMOVE_FILE:
            // Handle removing files from client files
            cm_remove_files(connection_mgr, ws_data);
            cm_remove_files_from_UI(&connection_mgr->server, root);
            printf("Handling REMOVE_FILE\n");
            break;
        case UI_ADD_FILES:
            // Handle Adding new files in client files
            cm_server_add_files(connection_mgr, ws_data);
            cm_send_files_to_client(connection_mgr, root);
            printf("Handling ADD_FILES\n");
            break;
        case UI_REMOVE_FILE:
            // Handle removing files from client files
            cm_remove_server_files(connection_mgr, ws_data);
            cm_remove_files_from_clients(connection_mgr, root);
            printf("Handling REMOVE_FILE\n");
            break;
        case REQUEST_CHUNK:
            chunk_request(app_ctx , ws_data, conn);
            printf("Handling REQUEST CHUNK\n");
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

    cJSON_Delete(root);
    return 1;
    
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

void ws_start(struct mg_context * cw_ctx, struct AppContext * app_ctx){

    mg_set_websocket_handler(cw_ctx, WEB_SOCKET_PATH,
                            ws_connect,
                            ws_ready,
                            ws_data,
                            ws_close,
                            (void *)app_ctx);
}



//  ========== Helper Functions ========== //

int ws_opcode_is_text(int con_opcode){
    // Ignore anything that's not a text frame
    // last 4 bits are used to represent opcode.
    if ((con_opcode & 0x0F) != MG_WEBSOCKET_OPCODE_TEXT) {
        printf("Ignoring non-text frame (opcode %d)\n", con_opcode);
        return 0;
    }
    return 1;
}

cJSON * parse_JSON_to_CJSON_root(char * data){
    cJSON *root = cJSON_Parse(data);
    if (!root) {
        printf("Not In JSON. Ignore.\n");
        return NULL;
    }
    return root;
}

struct WsMsgHeader ws_get_msg_headers(cJSON *root){
    // get our Custom Opcode {from JSON}
    struct WsMsgHeader header = {0, NULL}; // Default/sentinel return
    const cJSON * opcode = cJSON_GetObjectItem(root, "opcode");
    const cJSON * ws_data = cJSON_GetObjectItem(root, "data");
    if (!cJSON_IsNumber(opcode) || !(cJSON_IsObject(ws_data) || cJSON_IsArray(ws_data)) ) {
        printf("Invalid Structure of Websocket Request.'\n");
        return header;
    }

    header.opcode = opcode->valueint;
    header.data = (cJSON *)ws_data; 

    return header;
}
