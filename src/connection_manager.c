#include <cJSON.h>

#include "http_server.h"
#include "app_context.h"  // Global Context
#include "cJSON.h"

#include "json_to_data.h"

static void cm_broadcast_message(struct ConnectionManager * connection_mgr, char * payload, size_t payload_len);


void cm_init(struct ConnectionManager * connection_mgr){
    if (pthread_rwlock_init(&connection_mgr->rwlock, NULL) != 0){
        printf("Error Creating RW_lock\n");
        exit(0);
    }

    if (pthread_rwlock_init(&connection_mgr->server.rwlock, NULL) != 0){
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
    sprintf(buffer, "{\"opcode\":%d, \"data\":{\"public_id\":%d}}", PUBLIC_ID, public_id);
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));
}


void cm_add_client_to_UI(struct Server * server, struct Client * client){
    char buffer[256];
    sprintf(buffer, "{\"opcode\":%d, \"data\":{\"public_id\":%d, \"approved\":%d, \"public_name\":\"%s\"}}", ADD_CLIENT , client->public_id, client->approved, client->public_name);
    mg_websocket_write(server->conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));  
}


void cm_register_server(struct ConnectionManager * connection_mgr, struct mg_connection *conn){
    pthread_rwlock_wrlock(&connection_mgr->rwlock);
    
    struct Server * server = &connection_mgr->server;
    if (server->conn != NULL){
        printf("Connection Already exists ?\n");
        // TODO: ?
        // return;  // for testing, dont return. make it refresh.
    }

    server->conn =conn;
    server->public_id = 0;

    pthread_rwlock_unlock(&connection_mgr->rwlock);
    printf("App Connected Successfully\n");
    return;
}

void cm_set_client_approval(struct ConnectionManager * connection_mgr, const cJSON * ws_data, int approved){

    // Get public_id from data.
    int public_id;    
    if (j2d_get_int(ws_data, "public_id", &public_id) != 0){
        printf("Cannot APprove Client Because Public Id not found.\n");
        return;
    }

    // Lock the content manager to fund client
    printf("Public Id found is: %d \n", public_id);
    pthread_rwlock_rdlock(&connection_mgr->rwlock);
    

    // Find Client by Public_ID
    struct Client * client = client_find_by_public_id(connection_mgr->clients, public_id);
    if (client == NULL){
        printf("Cannot Approve Client because No CLient with Public_id: %d \n", public_id);
        goto end;
    }

    // Mark CLient Approved.
    pthread_rwlock_wrlock(&client->rwlock);
    // if (client->approved == 1){
    //     printf("Client ALready approved\n");
    // }
    client->approved = approved;
    pthread_rwlock_unlock(&client->rwlock);

    end:
    // close connections gracefully
    pthread_rwlock_unlock(&connection_mgr->rwlock);
    return;
}


void cm_notify_client_approval(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    
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
    char buffer[50];
    sprintf(buffer, "{\"opcode\":%d, \"data\":{}}", client->approved == 1 ? SERVER_APPROVE_CLIENT : SERVER_DIS_APPROVE_CLIENT);
    mg_websocket_write(client->conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));    
    pthread_rwlock_unlock(&client->rwlock);

    // gracefully exit lock.
end:
    pthread_rwlock_unlock(&connection_mgr->rwlock);
    return;
}

void cm_add_files(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    
    // Get public_id and file_count from data.
    int public_id, file_count;    
    if (j2d_get_int(ws_data, "public_id", &public_id) != 0 || j2d_get_int(ws_data, "file_count", &file_count) != 0){
        printf("Cannot add files - No public_id or file_count was found in message.\n");
        return;
    }
    

    // Lock the connection Manager to get Client.
    printf("public_id = %d, file_count = %d\n", public_id, file_count);
    pthread_rwlock_rdlock(&connection_mgr->rwlock);


    // Find Client by Public_ID
    struct Client * client = client_find_by_public_id(connection_mgr->clients, public_id);
    if (client == NULL){
        printf("Cannot Add Files - No CLient with Public_id: %d \n", public_id);
        goto end;
    }
    
    // Get array of Files Object
    const cJSON * files_obj = cJSON_GetObjectItem(ws_data, "files");
    if (files_obj == NULL){
        printf("Cannot Add Files - No files data found\n");
        goto end;   
    }

    for (int i = 0; i < file_count; i++) {
        printf("reached 1\n");
        // get current file
        cJSON *file = cJSON_GetArrayItem(files_obj, i);
        if (file == NULL){
            continue;
        }
printf("reached 2\n");
        // get file fields
        const char *name = j2d_get_string(file, "name");  
        const char *type = j2d_get_string(file, "type");  
        int id;
        size_t size;
printf("reached 3\n");
        // Fields CHecking. IMP: Name can be empty.
        if (j2d_get_int(file, "id", &id) != 0 || j2d_get_size_t(file, "size", &size) != 0 || type == NULL){
            printf("Canonot Add Filed - Incomplete Fields.\n");
            continue;
        }
printf("reached 4\n");
        // create new file
        struct File * new_file = (struct File *)calloc(1, sizeof(struct File));
        strncpy(new_file->name, name, sizeof(new_file->name)); // copy name
        new_file->name[sizeof(new_file->name)-1] = '\0';        // safe file name null terminator
        strncpy(new_file->type, type, sizeof(new_file->type));  // copy type
        new_file->type[sizeof(new_file->type)-1] = '\0';  // safe null check
        new_file->size = size;  // copy size
        new_file->id = id;  // copy id
printf("reached 5\n");
        new_file->is_transfering = 0; 
        pthread_rwlock_init(&new_file->rw_lock, NULL);
printf("reached 6\n");
        // add to hashmap
        pthread_rwlock_wrlock(&client->rwlock);
        printf("clinet-> files: %d, id: %d, new_file: %d", client->files == NULL ? 0 : 1, id, new_file == NULL ? 0 : 1);
        HASH_ADD_INT(client->files, id, new_file);
        pthread_rwlock_unlock(&client->rwlock);
    }
printf("reached 7\n");
end:
    pthread_rwlock_unlock(&connection_mgr->rwlock);
    return;
}


void cm_send_files_to_UI(struct Server * server, const cJSON * ws_data){
    char * string_to_send = cJSON_PrintUnformatted(ws_data);
    printf("data being sent to ui is: %s \n", string_to_send); 
    mg_websocket_write(server->conn, MG_WEBSOCKET_OPCODE_TEXT, string_to_send, strlen(string_to_send)); 
    free(string_to_send);   
    return;
}

void cm_broadcast_new_file(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    char * string_to_send = cJSON_PrintUnformatted(ws_data);
    cm_broadcast_message(connection_mgr, string_to_send, strlen(string_to_send));
    free(string_to_send);   
    return;
}


void cm_remove_files(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    // Get public_id and file_id from data.
    int public_id, file_id;    
    if (j2d_get_int(ws_data, "public_id", &public_id) != 0 || j2d_get_int(ws_data, "file_id", &file_id) != 0){
        printf("Cannot add files - No public_id or file_id was found in message.\n");
        return;
    }
    

    // Lock the connection Manager to get Client.
    printf("public_id = %d, file_id = %d\n", public_id, file_id);
    pthread_rwlock_rdlock(&connection_mgr->rwlock);

    // Find Client by Public_ID
    struct Client * client = client_find_by_public_id(connection_mgr->clients, public_id);
    if (client == NULL){
        printf("Cannot Add Files - No CLient with Public_id: %d \n", public_id);
        goto end;
    }

    pthread_rwlock_wrlock(&client->rwlock);
    struct File * cur_file = NULL;
    HASH_FIND_INT(client->files, &file_id, cur_file);
    HASH_DEL(client->files, cur_file);
    free(cur_file);
    printf("file removed\n");
    pthread_rwlock_unlock(&client->rwlock);

end:
    pthread_rwlock_unlock(&connection_mgr->rwlock);
    return;
}


void cm_remove_files_from_UI(struct Server * server, const cJSON * ws_data){
    char * string_to_send = cJSON_PrintUnformatted(ws_data);
    printf("data being sent to ui is: %s \n", string_to_send); 
    mg_websocket_write(server->conn, MG_WEBSOCKET_OPCODE_TEXT, string_to_send, strlen(string_to_send)); 
    free(string_to_send);   
    return;
}


void cm_server_add_files(struct ConnectionManager * connection_mgr, const cJSON * ws_data){

    int file_count = cJSON_GetObjectItem(ws_data, "file_count")->valueint;
    printf("file_count = %d\n", file_count);

    struct Server * server = &connection_mgr->server;

    const cJSON * files_obj = cJSON_GetObjectItem(ws_data, "files");

    for (int i = 0; i < file_count; i++) {
        struct File * new_file = (struct File *)calloc(1, sizeof(struct File));
        cJSON *file = cJSON_GetArrayItem(files_obj, i);
        
        const char *name = cJSON_GetObjectItem(file, "name")->valuestring;
        int size = cJSON_GetObjectItem(file, "size")->valueint;
        const char *type = cJSON_GetObjectItem(file, "type")->valuestring;
        int id = cJSON_GetObjectItem(file, "id")->valueint;
        
        printf("File %d:\n", i + 1);
        strncpy(new_file->name, name, sizeof(new_file->name));
        printf("  Name: %s\n", name);
        new_file->size = size;
        printf("  Size: %d\n", size);
        strncpy(new_file->type, type, sizeof(new_file->type));
        printf("  Type: %s\n", type);
        new_file->id = id;
        printf("  ID:   %d\n", id);

        new_file->is_transfering = 0;
        pthread_rwlock_init(&new_file->rw_lock, NULL);
        HASH_ADD_INT(server->files, id, new_file);
    }

    return;
}

void cm_send_files_to_client(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    {
        pthread_rwlock_rdlock(&connection_mgr->rwlock);
        char * string_to_send = cJSON_PrintUnformatted(ws_data);
        printf("data being sent to client is: %s \n", string_to_send); 
        struct Client *cur, *temp;  
        HASH_ITER(hh, connection_mgr->clients, cur, temp ){
            mg_websocket_write(cur->conn, MG_WEBSOCKET_OPCODE_TEXT, string_to_send, strlen(string_to_send));
        }
        free(string_to_send);   
        pthread_rwlock_unlock(&connection_mgr->rwlock);
        return;
    }
 
}




void cm_remove_server_files(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    pthread_rwlock_rdlock(&connection_mgr->server.rwlock);
    int public_id = cJSON_GetObjectItem(ws_data, "public_id")->valueint;
    int file_id = cJSON_GetObjectItem(ws_data, "file_id")->valueint;
    printf("public_id = %d, file_count = %d\n", public_id, file_id);


    struct File * cur_file = NULL;
    HASH_FIND_INT(connection_mgr->server.files, &file_id, cur_file);
    HASH_DEL(connection_mgr->server.files, cur_file);
    free(cur_file);
    printf("file removed\n");

    pthread_rwlock_unlock(&connection_mgr->server.rwlock);
    return;
}


void cm_remove_files_from_clients(struct ConnectionManager * connection_mgr, const cJSON * ws_data){
    char * string_to_send = cJSON_PrintUnformatted(ws_data);
    printf("data being sent to ui is: %s \n", string_to_send);
    struct Client *cur, *temp;   
    HASH_ITER(hh, connection_mgr->clients, cur, temp ){
        mg_websocket_write(cur->conn, MG_WEBSOCKET_OPCODE_TEXT, string_to_send, strlen(string_to_send));
        // free(cur); 
    }
    free(string_to_send);   
    return;
}





// ===========  CM HELPER FUNS ============= //

static void cm_broadcast_message(struct ConnectionManager * connection_mgr, char * payload, size_t payload_len){
    // lock connection manager, bcz we need to loop over 
    pthread_rwlock_wrlock(&connection_mgr->rwlock);
    
    // send payload to server
    mg_websocket_write(connection_mgr->server.conn, MG_WEBSOCKET_OPCODE_TEXT, payload, payload_len);

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

