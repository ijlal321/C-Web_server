#include <cJSON.h>

#include "http_server.h"
#include "app_context.h"  // Global Context
#include "cJSON.h"

#include "json_to_data.h"

static void cm_broadcast_message(struct ConnectionManager * connection_mgr, char * payload, size_t payload_len);
static void cm_broadcast_message_to_all_clients(struct ConnectionManager * connection_mgr, char * payload, size_t payload_len, int exception_id);

void cm_init(struct ConnectionManager * connection_mgr){
    if (pthread_rwlock_init(&connection_mgr->rwlock, NULL) != 0){
        printf("Error Creating RW_lock\n");
        exit(0);
    }

    if (pthread_rwlock_init(&connection_mgr->master_app.rwlock, NULL) != 0){
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


//  CLIENT REGISTRATION

struct Client * cm_add_client(struct ConnectionManager * connection_mgr, struct mg_connection *conn, const cJSON *data){
    pthread_rwlock_wrlock(&connection_mgr->rwlock);

    // get private ID from websocket data field.
    const char * private_id = j2d_get_string(data, "private_id");
    if (private_id == NULL){
        printf("Client did not sent a valid Private ID. Closing Connection.\n");
        pthread_rwlock_unlock(&connection_mgr->rwlock);
        return NULL;
    }
    
    // create new client
    int new_client_public_id = HASH_COUNT(connection_mgr->clients) + 1;
    struct Client * new_client = client_create_new(private_id, new_client_public_id, "Cline nam here", conn);
    if (new_client == NULL){
        pthread_rwlock_unlock(&connection_mgr->rwlock);
        return NULL;
    }

    // add client to list of clients.
    HASH_ADD_INT(connection_mgr->clients, public_id, new_client);

    pthread_rwlock_unlock(&connection_mgr->rwlock);
    return new_client; // everything Okay. Keep Connection Open.
}

void cm_send_public_id_to_client(struct mg_connection * conn, int public_id){
    char buffer[128];
    //                    10 +  4 +                      22  + 4 +2 = 42 byte total  
    sprintf(buffer, "{\"opcode\":%d, \"data\":{\"public_id\":%d}}", CLIENT_REGISTER_ACK, public_id);
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));
}
void cm_registered_client_send_ack(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    // get public_id of client who got registered
    int public_id;    
    if (j2d_get_int(ws_data, "public_id", &public_id) != 0){
        printf("Cannot APprove Client Because Public Id not found.\n");
        return;
    }
    pthread_rwlock_rdlock(&connection_mgr->rwlock);
    // find client which get registered by public id
    struct Client * client = client_find_by_public_id(connection_mgr->clients, public_id);
    if (client == NULL){
        printf("Cannot Approve Client because No CLient with Public_id: %d \n", public_id);
        pthread_rwlock_unlock(&connection_mgr->rwlock);
        goto end;
    }
    
    // notify newly registered client about his registration
    char buff[128];
    sprintf(buff, "{\"opcode\":%d, \"data\":{\"public_id\":%d }}", CLIENT_REGISTER_ACK, public_id);
    mg_websocket_write(client->conn, MG_WEBSOCKET_OPCODE_TEXT, buff, strlen(buff));
end:  
    pthread_rwlock_unlock(&connection_mgr->rwlock);
}

void cm_add_client_to_UI(struct MasterApp * master_app, struct Client * client){
    char buffer[256];
    sprintf(buffer, "{\"opcode\":%d, \"data\":{\"public_id\":%d, \"approved\":%d, \"public_name\":\"%s\"}}", CLIENT_REGISTER , client->public_id, client->approved, client->public_name);
    mg_websocket_write(master_app->conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));  
}

// MASTER APP NOT READY HANDLE
void cm_server_not_ready_handle(struct mg_connection *conn, cJSON * root){
    char * incoming_payload = cJSON_PrintUnformatted(root);
    char * payload = calloc(1, strlen(incoming_payload) + 64);
    sprintf(payload, "{\"opcode\": %d, \"data\": %s}", SERVER_NOT_READY, incoming_payload);
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, payload, strlen(payload));
    free(payload);
    free(incoming_payload);
}

//  MASTER APP REGISTRATION

int cm_register_master_app(struct ConnectionManager * connection_mgr, struct mg_connection *conn){
    pthread_rwlock_wrlock(&connection_mgr->rwlock);
    
    struct MasterApp * master_app = &connection_mgr->master_app;
    if (master_app->conn != NULL){
        printf("Connection Already exists ?\n");
        // TODO: ?
        // return;  // for testing, dont return. make it refresh.
    }

    master_app->conn =conn;
    master_app->public_id = 0;

    pthread_rwlock_unlock(&connection_mgr->rwlock);
    printf("App Connected Successfully\n");
    return 0;
}

void cm_send_master_app_registered_ack(struct MasterApp * server, int res){
    // use res to find if registered or not
    (void)res;  // for warning unsed vars

    char buffer[128];
    sprintf(buffer, "{\"opcode\":%d , \"data\":{\"public_id\": %d}}", MASTER_APP_REGISTER_ACK, 0);
    mg_websocket_write(server->conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));
}

//  CLIENT APPROVAL / DISAPPROVAL

/**
 * @return 0 on success. else error.
 */
int cm_set_client_approval(struct ConnectionManager * connection_mgr, const cJSON * ws_data, int approved){

    // Get public_id from data.
    int public_id;    
    if (j2d_get_int(ws_data, "public_id", &public_id) != 0){
        printf("Cannot APprove Client Because Public Id not found.\n");
        return 1;
    }

    // Lock the content manager to fund client
    printf("Public Id found is: %d \n", public_id);
    pthread_rwlock_rdlock(&connection_mgr->rwlock);
    

    // Find Client by Public_ID
    struct Client * client = client_find_by_public_id(connection_mgr->clients, public_id);
    if (client == NULL){
        printf("Cannot Approve Client because No CLient with Public_id: %d \n", public_id);
        pthread_rwlock_unlock(&connection_mgr->rwlock);
        return 1;
    }

    // Mark CLient Approved.
    pthread_rwlock_wrlock(&client->rwlock);
    // if (client->approved == 1){
    //     printf("Client ALready approved\n");
    // }
    client->approved = approved;
    pthread_rwlock_unlock(&client->rwlock);

    // close connections gracefully
    pthread_rwlock_unlock(&connection_mgr->rwlock);
    return 0;
}


void cm_broadcast_client_approval(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    
    // Get public_id from data.
    int public_id;    
    if (j2d_get_int(ws_data, "public_id", &public_id) != 0){
        printf("Cannot APprove Client Because Public Id not found.\n");
        return;
    }


    pthread_rwlock_rdlock(&connection_mgr->rwlock);
    
    // Find Client by Public_ID
    struct Client * client = client_find_by_public_id(connection_mgr->clients, public_id);
    if (client == NULL){
        printf("Cannot Approve Client because No CLient with Public_id: %d \n", public_id);
        goto end;
    }
    
    // Lock Client for readinf
    pthread_rwlock_rdlock(&client->rwlock);
    char buffer[sizeof(struct Client) + 100]; 
    sprintf(buffer, "{\"opcode\":%d, \"data\":{\"public_id\":%d, \"public_name\":\"%s\", \"approved\":%d}}", client->approved == 1 ? CLIENT_APPROVED : CLIENT_DIS_APPROVED, public_id, client->public_name, client->approved);
    cm_broadcast_message_to_all_clients(connection_mgr, buffer, strlen(buffer), client->public_id);
    pthread_rwlock_unlock(&client->rwlock);

    // gracefully exit lock.
end:
    pthread_rwlock_unlock(&connection_mgr->rwlock);
    return;
}


//   FILE MANAGEMENT


void cm_broadcast_new_file(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    // ws_data contains full data without opcodes
    char * data_string = cJSON_PrintUnformatted(ws_data);
    char * string_to_send = calloc(1, strlen(data_string) + 128);
    sprintf(string_to_send, "{\"opcode\":%d, \"data\":%s}", FILES_ADDED, data_string); 
    // char * string_to_send = cJSON_PrintUnformatted(ws_data);
    cm_broadcast_message_to_all_clients(connection_mgr, string_to_send, strlen(string_to_send), -1);
    free(string_to_send);
    free(data_string);
    return;
}

void cm_remove_file_req_to_master_app(struct ConnectionManager * connection_mgr, const cJSON * root){
    // lock master_app
    pthread_rwlock_rdlock(&connection_mgr->master_app.rwlock);

    // ws_data contains full data without opcodes
    char * data_string = cJSON_PrintUnformatted(root);

    mg_websocket_write(connection_mgr->master_app.conn, MG_WEBSOCKET_OPCODE_TEXT, data_string, strlen(data_string));

    free(data_string);

    pthread_rwlock_unlock(&connection_mgr->master_app.rwlock);
    return;
}

void cm_send_files_to_master_app(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    // lock master_app
    pthread_rwlock_rdlock(&connection_mgr->master_app.rwlock);

    // ws_data contains full data without opcodes
    char * data_string = cJSON_PrintUnformatted(ws_data);
    char * string_to_send = calloc(1, strlen(data_string) + 128);
    sprintf(string_to_send, "{\"opcode\":%d, \"data\":%s}", ADD_FILES, data_string);

    mg_websocket_write(connection_mgr->master_app.conn, MG_WEBSOCKET_OPCODE_TEXT, string_to_send, strlen(string_to_send));

    free(string_to_send);
    free(data_string);

    pthread_rwlock_unlock(&connection_mgr->master_app.rwlock);
    return;
}

void cm_broadcast_remove_file(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    // ws_data contains full data without opcodes
    char * data_string = cJSON_PrintUnformatted(ws_data);
    cm_broadcast_message_to_all_clients(connection_mgr, data_string, strlen(data_string), -1);
    free(data_string);
    return;
}



// ===========  CM HELPER FUNS ============= //

static void cm_broadcast_message(struct ConnectionManager * connection_mgr, char * payload, size_t payload_len){
    // lock connection manager, bcz we need to loop over 
    pthread_rwlock_wrlock(&connection_mgr->rwlock);
    
    // send payload to server
    mg_websocket_write(connection_mgr->master_app.conn, MG_WEBSOCKET_OPCODE_TEXT, payload, payload_len);

    // send payload to all clients
    // loop over conn of all clients and send if its approved
    struct Client *cur, *tmp;
    HASH_ITER(hh, connection_mgr->clients, cur, tmp) {
        if (cur->approved){
            mg_websocket_write(cur->conn, MG_WEBSOCKET_OPCODE_TEXT, payload, payload_len);
        }
    }
    pthread_rwlock_unlock(&connection_mgr->rwlock);
}

/**
 * @attention It assumes lock on connection manager is already done
 */
static void cm_broadcast_message_to_all_clients(struct ConnectionManager * connection_mgr, char * payload, size_t payload_len, int exception_id){

    // send payload to all clients
    // loop over conn of all clients and send if its approved
    struct Client *cur, *tmp;
    HASH_ITER(hh, connection_mgr->clients, cur, tmp) {
        if (cur->public_id == exception_id || cur->approved){
            mg_websocket_write(cur->conn, MG_WEBSOCKET_OPCODE_TEXT, payload, payload_len);
        }
    }
}

