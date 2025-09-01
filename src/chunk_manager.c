#include "app_context.h"


void chunk_manager_init(struct ChunkManager * chunk_mgr){
    pthread_rwlock_init(&chunk_mgr->rw_lock, NULL);
}


void chunk_request_cheat(struct AppContext * app_ctx , const cJSON * ws_data, struct mg_connection *conn){
    struct ChunkManager * chunk_mgr = &app_ctx->chunk_mgr;
    struct ConnectionManager * connection_mgr = &app_ctx->connection_mgr;
    // {opcode: 12, data: {sender_public_id:_, public_id:_ , file_id: _, chunk_id}}
    int sender_public_id = cJSON_GetObjectItem(ws_data, "sender_public_id")->valueint;
    int public_id = cJSON_GetObjectItem(ws_data, "public_id")->valueint; // public id of target
    int file_id = cJSON_GetObjectItem(ws_data, "file_id")->valueint;
    int chunk_id = cJSON_GetObjectItem(ws_data, "chunk_id")->valueint;

    struct ChunkKey chunk_key = {0}; // very important zero out key. bcz uthash will cmp full struct to struct. may cause errors.
    chunk_key.public_id = public_id;
    chunk_key.file_id = file_id;
    chunk_key.chunk_id = chunk_id;

    pthread_rwlock_wrlock(&chunk_mgr->rw_lock);
    // hashfind from chunk_mgr->chunks on chunk_key
    struct FileChunk * cur_chunk = NULL;
    HASH_FIND(hh, chunk_mgr->chunks, &chunk_key, sizeof(struct ChunkKey), cur_chunk);

    // if chunk dont exist
    if (cur_chunk == NULL){
        struct FileChunk * new_chunk = chunk_create(public_id, file_id, chunk_id);
        add_client_to_chunk(new_chunk, sender_public_id);
        HASH_ADD(hh, chunk_mgr->chunks, chunk_key, sizeof(struct ChunkKey), new_chunk);
        // start fetching data ?
        struct mg_connection * target_socket = NULL;
        if (public_id == 0){ // menaing server;
            target_socket = connection_mgr->server.conn;
            printf("Target is Server\n");
        }else{
            struct Client * target_client = NULL;
            HASH_FIND_INT(connection_mgr->clients, &public_id, target_client);
            if (target_client != NULL){
                printf("Found target id:%d\n", public_id);
                target_socket = target_client->conn;
            }
        }
        if (target_socket == NULL){
            printf("No Target Socket ?\n");
        }
        char buffer[500];
        sprintf(buffer, "{\"opcode\":%d, \"data\":{\"public_id\":%d, \"file_id\":%d, \"chunk_id\":%d}}", SERVER_UPLOAD_CHUNK, public_id, file_id, chunk_id);
        int write_err = mg_websocket_write(target_socket, MG_WEBSOCKET_OPCODE_TEXT,  buffer, strlen(buffer));
        printf("Sedning error is: %d\n", write_err);
        printf("send req to get data\n %s \n", buffer);
        goto end;
    }

    pthread_rwlock_rdlock(&cur_chunk->rw_lock);
    // if chunk exist but not downloaded
    if (cur_chunk->is_downloaded == 0){
        // tell socket to wait
    }else{
        // if chunk fully ready
        // send msg ready
        char buffer[127];
        sprintf(buffer, "{\"opcode\":%d, \"data\": {\"public_id\":%d, \"file_id\":%d, \"chunk_id\":%d}}", SERVER_CHUNK_READY, public_id, file_id, chunk_id);
        mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));
    }
    pthread_rwlock_unlock(&cur_chunk->rw_lock);
end: 
    pthread_rwlock_unlock(&chunk_mgr->rw_lock);
}


//cheat one
void chunk_request(struct AppContext * app_ctx , const cJSON * ws_data, struct mg_connection *conn){
    struct ChunkManager * chunk_mgr = &app_ctx->chunk_mgr;
    struct ConnectionManager * connection_mgr = &app_ctx->connection_mgr;
    // {opcode: 12, data: {sender_public_id:_, public_id:_ , file_id: _, chunk_id}}
    int sender_public_id = cJSON_GetObjectItem(ws_data, "sender_public_id")->valueint;
    int public_id = cJSON_GetObjectItem(ws_data, "public_id")->valueint; // public id of target
    int file_id = cJSON_GetObjectItem(ws_data, "file_id")->valueint;
    int chunk_id = cJSON_GetObjectItem(ws_data, "chunk_id")->valueint;

    struct ChunkKey chunk_key = {0}; // very important zero out key. bcz uthash will cmp full struct to struct. may cause errors.
    chunk_key.public_id = public_id;
    chunk_key.file_id = file_id;
    chunk_key.chunk_id = chunk_id;

    pthread_rwlock_wrlock(&chunk_mgr->rw_lock);
    // hashfind from chunk_mgr->chunks on chunk_key
    struct FileChunk * cur_chunk = NULL;
    HASH_FIND(hh, chunk_mgr->chunks, &chunk_key, sizeof(struct ChunkKey), cur_chunk);

    // if chunk dont exist
    if (cur_chunk == NULL){
        struct FileChunk * new_chunk = chunk_create(public_id, file_id, chunk_id);
        add_client_to_chunk(new_chunk, sender_public_id);
        // For testing: open file, read chunk, malloc and assign to new_chunk->data
        const char *test_path = "D:/downloads/rar_file.rar";
        FILE *fp = fopen(test_path, "rb");
        if (fp) {
            size_t chunk_size = CHUNK_SIZE;
            size_t offset = chunk_id * chunk_size;
            fseek(fp, 0, SEEK_END);
            size_t file_size = ftell(fp);
            if (offset < file_size) {
                size_t to_read = chunk_size;
                printf("data to read is %zu \n", to_read);
                if (offset + chunk_size > file_size) to_read = file_size - offset;
                fseek(fp, offset, SEEK_SET);
                void *buf = malloc(to_read);
                size_t got = fread(buf, 1, to_read, fp);
                printf("original chunk size: %zu , but got %zu \n", to_read, got);
                if (buf) {
                    new_chunk->data = buf;
                    new_chunk->size = to_read;
                    new_chunk->is_downloaded = 1;
                    printf("Loaded chunk %d (%zu bytes) from test file.\n", chunk_id, to_read);
                } else {
                    if (buf) free(buf);
                    printf("Failed to read chunk from file.\n");
                }
            } else {
                printf("Chunk offset beyond file size.\n");
            }
            fclose(fp);
        } else {
            printf("Failed to open test file: %s\n", test_path);
        }
        HASH_ADD(hh, chunk_mgr->chunks, chunk_key, sizeof(struct ChunkKey), new_chunk);
        // Immediately mark as ready for download
        char buffer[127];
        sprintf(buffer, "{\"opcode\":%d, \"data\": {\"public_id\":%d, \"file_id\":%d, \"chunk_id\":%d}}", SERVER_CHUNK_READY, public_id, file_id, chunk_id);
        mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));
        goto end;
    }

    pthread_rwlock_rdlock(&cur_chunk->rw_lock);
    // if chunk exist but not downloaded
    if (cur_chunk->is_downloaded == 0){
        // tell socket to wait
    }else{
        // if chunk fully ready
        // send msg ready
        char buffer[127];
        sprintf(buffer, "{\"opcode\":%d, \"data\": {\"public_id\":%d, \"file_id\":%d, \"chunk_id\":%d}}", SERVER_CHUNK_READY, public_id, file_id, chunk_id);
        mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));
    }
    pthread_rwlock_unlock(&cur_chunk->rw_lock);
end: 
    pthread_rwlock_unlock(&chunk_mgr->rw_lock);
}

void add_client_to_chunk(struct FileChunk * new_chunk, int sender_public_id){
    struct PublicIdEntry * public_id_entry = (struct PublicIdEntry *)calloc(1, sizeof(struct PublicIdEntry));
    public_id_entry->public_id = sender_public_id;
    HASH_ADD_INT(new_chunk->public_ids, public_id, public_id_entry);
    new_chunk->client_count ++;
}

struct FileChunk * chunk_create(int public_id, int file_id, int chunk_id){
    struct FileChunk * new_chunk = calloc(1, sizeof(struct FileChunk));
    new_chunk->chunk_key.public_id = public_id;
    new_chunk->chunk_key.file_id = file_id;
    new_chunk->chunk_key.chunk_id = chunk_id;
    new_chunk->size = 0; // todo ? what to do here. should i leave it like this ?
    new_chunk->is_downloaded = 0;
    pthread_rwlock_init(&new_chunk->rw_lock, NULL);
    return new_chunk;
}