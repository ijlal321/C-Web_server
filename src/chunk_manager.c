#include "chunk_manager.h"
#include "app_context.h"


void chunk_manager_init(struct ChunkManager * chunk_mgr){
    pthread_rwlock_init(&chunk_mgr->rw_lock, NULL);
}


void chunk_request(struct AppContext * app_ctx , const cJSON * ws_data, struct mg_connection *conn){
    struct ChunkManager * chunk_mgr = &app_ctx->chunk_mgr;
    struct ConnectionManager * connection_mgr = &app_ctx->connection_mgr;

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
        goto end;
    }

    pthread_rwlock_rdlock(&cur_chunk->rw_lock);
    // if chunk exist but not downloaded
    if (cur_chunk->is_downloaded == 0){
        // tell socket to wait
    }else{
        // if chunk fully ready
        // send msg ready
        const buffer[127];
        sprintf(buffer, "{\"opcode\":%d, \"data\": {\"public_id\":%d, \"file_id\":%d, \"chunk_id\":%d}}", SERVER_CHUNK_READY, public_id, file_id, chunk_id);
        mg_write(conn, buffer, sizeof(buffer));
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
    new_chunk->is_downloaded = 0;
    pthread_rwlock_init(&new_chunk->rw_lock, NULL);
    return &new_chunk;
}