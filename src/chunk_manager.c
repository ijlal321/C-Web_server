#include "app_context.h"
#include "json_to_data.h"


// ======= HELPER FUNCTIONS HEADER ======== //

static int parse_chunk_request(const cJSON *ws_data, int *sender_public_id, int *public_id, int *file_id, size_t *start_pos, size_t * size);
static struct mg_connection *find_target_socket(struct ConnectionManager *mgr, int public_id);
static void send_chunk_request(struct mg_connection *conn, int opcode, int public_id, int file_id, size_t start_pos, size_t size);


// ==================================  

// =============== FUNCTIONS =========== //

void chunk_manager_init(struct ChunkManager * chunk_mgr){
    pthread_rwlock_init(&chunk_mgr->rw_lock, NULL);
}


void chunk_request_manage(struct AppContext * app_ctx , const cJSON * ws_data, struct mg_connection *conn){
    struct ChunkManager * chunk_mgr = &app_ctx->chunk_mgr;
    struct ConnectionManager * connection_mgr = &app_ctx->connection_mgr;
    // {opcode: 12, data: {sender_public_id:_, file_public_id:_ , file_id: _, start_pos, size}}
    
    // Get All Fields
    int sender_public_id, owner_public_id, file_id;
    size_t start_pos, size;
    if (parse_chunk_request(ws_data, &sender_public_id, &owner_public_id, &file_id, &start_pos, &size) != 0) {
        printf("Invalid chunk request: missing fields.\n");
        return;
    }

    // create key to find chunk
    struct ChunkKey chunk_key = {0}; // very important zero out key. bcz uthash will cmp full struct to struct. may cause errors.
    chunk_key.public_id = owner_public_id;
    chunk_key.file_id = file_id;
    chunk_key.start_pos = start_pos;
    chunk_key.size = size;

    pthread_rwlock_wrlock(&chunk_mgr->rw_lock);

    // Find Target Chunk
    struct FileChunk * cur_chunk = NULL;
    HASH_FIND(hh, chunk_mgr->chunks, &chunk_key, sizeof(struct ChunkKey), cur_chunk);

    // if chunk dont exist
    if (!cur_chunk){
        
        // Get Owner Connection varaible
        struct mg_connection *target_socket = find_target_socket(connection_mgr, owner_public_id);
        if (!target_socket) goto end;

        // Create Chunk
        struct FileChunk * new_chunk = chunk_create(owner_public_id, file_id, start_pos, size);
        if (new_chunk == NULL) goto end;

        // add sender to chunk
        add_client_to_chunk(new_chunk, sender_public_id);
        
        // add chunk to chunks manager list.
        HASH_ADD(hh, chunk_mgr->chunks, chunk_key, sizeof(struct ChunkKey), new_chunk);
        
        // send message to owner to send data
        printf("calling fn to create chunk, opcode: %d \n", REQUEST_CHUNK_UPLOAD);
        send_chunk_request(target_socket, REQUEST_CHUNK_UPLOAD, owner_public_id, file_id, start_pos, size);
        goto end;
    
    }

    pthread_rwlock_wrlock(&cur_chunk->rw_lock);

    // add sender to chunk -> wait list(all clients who need this when downloaded).
    add_client_to_chunk(cur_chunk, sender_public_id);

    if (cur_chunk->is_downloaded){   // if chunk fully available then download, else wait.
        send_chunk_request(conn, CHUNK_READY, owner_public_id, file_id, start_pos, size);
    }

    pthread_rwlock_unlock(&cur_chunk->rw_lock);
    
end: 
    pthread_rwlock_unlock(&chunk_mgr->rw_lock);
}


//cheat one  // errors, need to be in same format as original
void chunk_request_cheat(struct AppContext * app_ctx , const cJSON * ws_data, struct mg_connection *conn){
    struct ChunkManager * chunk_mgr = &app_ctx->chunk_mgr;
    // struct ConnectionManager * connection_mgr = &app_ctx->connection_mgr;  // not using
    // {opcode: 12, data: {sender_public_id:_, public_id:_ , file_id: _, chunk_id}}
    int sender_public_id = cJSON_GetObjectItem(ws_data, "sender_public_id")->valueint;
    int public_id = cJSON_GetObjectItem(ws_data, "public_id")->valueint; // public id of target
    int file_id = cJSON_GetObjectItem(ws_data, "file_id")->valueint;
    int chunk_id = cJSON_GetObjectItem(ws_data, "chunk_id")->valueint;

    struct ChunkKey chunk_key = {0}; // very important zero out key. bcz uthash will cmp full struct to struct. may cause errors.
    chunk_key.public_id = public_id;
    chunk_key.file_id = file_id;
    // chunk_key.chunk_id = chunk_id;

    pthread_rwlock_wrlock(&chunk_mgr->rw_lock);
    // hashfind from chunk_mgr->chunks on chunk_key
    struct FileChunk * cur_chunk = NULL;
    HASH_FIND(hh, chunk_mgr->chunks, &chunk_key, sizeof(struct ChunkKey), cur_chunk);

    // if chunk dont exist
    if (cur_chunk == NULL){
        struct FileChunk * new_chunk = chunk_create(public_id, file_id, chunk_id, chunk_id);
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
        sprintf(buffer, "{\"opcode\":%d, \"data\": {\"public_id\":%d, \"file_id\":%d, \"chunk_id\":%d}}", CHUNK_READY, public_id, file_id, chunk_id);
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
        sprintf(buffer, "{\"opcode\":%d, \"data\": {\"public_id\":%d, \"file_id\":%d, \"chunk_id\":%d}}", CHUNK_READY, public_id, file_id, chunk_id);
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

struct FileChunk * chunk_create(int public_id, int file_id, size_t start_pos, size_t size){
    struct FileChunk * new_chunk = calloc(1, sizeof(struct FileChunk));
    if (new_chunk == NULL){
        printf("Failed to Allocate Space for new CHUNK\n");
        return NULL;
    }
    new_chunk->chunk_key.public_id = public_id;
    new_chunk->chunk_key.file_id = file_id;
    new_chunk->chunk_key.start_pos = start_pos;
    new_chunk->chunk_key.size = size;
    new_chunk->size = 0; // todo ? what to do here. should i leave it like this ?
    new_chunk->is_downloaded = 0;
    pthread_rwlock_init(&new_chunk->rw_lock, NULL);
    return new_chunk;
}



// =========== HELPER FUNCTIONS ============= //

static int parse_chunk_request(const cJSON *ws_data,
                               int *sender_public_id, int *owner_public_id,
                               int *file_id, size_t *start_pos, size_t *size)
{
    if (j2d_get_int(ws_data, "sender_public_id", sender_public_id) != 0 ||
        j2d_get_int(ws_data, "owner_public_id", owner_public_id) != 0 ||
        j2d_get_int(ws_data, "file_id", file_id) != 0 ||
        j2d_get_size_t(ws_data, "start_pos", start_pos) != 0 ||
        j2d_get_size_t(ws_data, "size", size) != 0)
    {
        return -1; // missing field(s)
    }
    return 0;
}

static struct mg_connection *find_target_socket(struct ConnectionManager *mgr, int public_id)
{
    if (public_id == 0) {
        return mgr->master_app.conn;  // special case: server
    }

    struct Client *target_client = NULL;
    HASH_FIND_INT(mgr->clients, &public_id, target_client);
    if (!target_client) {
        printf("Client with public_id %d not found.\n", public_id);
        return NULL;
    }

    return target_client->conn;
}

static void send_chunk_request(struct mg_connection *conn,
                               int opcode, int owner_public_id, int file_id, size_t start_pos, size_t size)
{
    char buffer[256];
    int n = snprintf(buffer, sizeof(buffer),
        "{\"opcode\":%d, \"data\":{\"owner_public_id\":%d, \"file_id\":%d, \"start_pos\":%zu , \"size\":%zu}}",
        opcode, owner_public_id, file_id, start_pos, size);

    if (n < 0 || n >= (int)sizeof(buffer)) {
        printf("Error: JSON message truncated.\n");
        return;
    }

    int write_err = mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, n);
    if (write_err < 0) {
        printf("Websocket send failed: %d\n", write_err);
    } else {
        printf("Sent: %s\n", buffer);
    }
}



