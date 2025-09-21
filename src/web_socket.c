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
    (void)conn;  // for warning unsed vars
    (void)cbdata;

    // accept all connections
    return 0;

    return 1;  // Reject connection
}

void ws_ready(struct mg_connection *conn, void *cbdata) {
    (void)conn;  // for warning unsed vars
    (void)cbdata;
    
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

    // TODO: SERIOUS: HANDLE MASTER_APP LOCK
    pthread_rwlock_rdlock(&connection_mgr->master_app.rwlock);
    if (connection_mgr->master_app.conn == NULL && op != MASTER_APP_REGISTER){
        // if some client sent data when master app not registered, ask to relay
        cm_server_not_ready_handle(conn, root);
        pthread_rwlock_unlock(&connection_mgr->master_app.rwlock);
        goto end;
    }
    pthread_rwlock_unlock(&connection_mgr->master_app.rwlock);


    int res; // temp var for holding data below
    switch (op) {
        case MASTER_APP_REGISTER:
            res = cm_register_master_app(connection_mgr, conn);
            // TODO: Send client the list of clients waiting for approval or if some connect before it. use cm_send_public_id_to_client();
            cm_send_master_app_registered_ack(&connection_mgr->master_app, res);
            break;
        case CLIENT_REGISTER:
            struct Client * client = cm_add_client(connection_mgr, conn, ws_data);
            if (client == NULL){ // some error occured
                return 1;  // 0 means close connection. we are not rn.
            }
            // TODO: Big : Name send.
            // cm_send_public_id_to_client(conn, client->public_id);
            cm_add_client_to_UI(&connection_mgr->master_app, client);
            break;
        case CLIENT_REGISTER_ACK:
            cm_registered_client_send_ack(connection_mgr, ws_data);
            break;
        case CLIENT_APPROVED:
            res = cm_set_client_approval(connection_mgr, ws_data, 1); // set client to approved
            if (res != -1){

                cm_broadcast_client_approval(connection_mgr, ws_data, root); // tell Every Client about a client approval
                printf("approve client called\n");
            }
            break;
        case CLIENT_DIS_APPROVED:
            res = cm_set_client_approval(connection_mgr, ws_data, 0); // set client to disaproved
            if (res != -1){
                cm_broadcast_client_approval(connection_mgr, ws_data, root); // notify client
                printf("UI_DIS_APPROVE_CLIENT client called\n");
            }
            break;
        case ADD_FILES:
            /// Note: Duplicate FIle ID will be Ignored. Client is expected to do this.
            // cm_broadcast_new_file(connection_mgr, ws_data);
            cm_send_files_to_master_app(connection_mgr, ws_data);
            printf("Handling ADD_FILES\n");
            break;
        case FILES_ADDED:
        // use above broadcast file
        cm_broadcast_new_file(connection_mgr, ws_data);
            break;
        case REMOVE_FILE:
            cm_remove_file_req_to_master_app(connection_mgr, root);
            // cm_broadcast_remove_file(connection_mgr, ws_data);
            printf("Handling REMOVE_FILE\n");
            break;
        case FILE_REMOVED:
        // use above broadcast file
        cm_broadcast_remove_file(connection_mgr, root);
            break;
        case REQUEST_CHUNK:
            chunk_request_manage(app_ctx , ws_data, conn);
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

end:
    cJSON_Delete(root);
    return 1;
    
}

void ws_close(const struct mg_connection *conn, void *cbdata) {
    (void)conn;  // for warning unsed vars
    
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
