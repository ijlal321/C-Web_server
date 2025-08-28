#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <pthread.h>

#include "config.h"
#include "uthash.h"
#include "cJSON.h"

struct PublicIdEntry {
    int public_id;
    UT_hash_handle hh;
};

struct ChunkKey{
    int public_id;
    int file_id;
    int chunk_id;
};

struct FileChunk {
    struct ChunkKey chunk_key;
    int client_count; // nr of clients needing it
    // int * public_ids; // uthash for all public ids needing it
    struct PublicIdEntry *public_ids; // hash table of public ids of clients which needs this.
    int is_downloaded; // set when you successsfully fetch data from target.
    unsigned char * data; // dynamically allocated on heap. Must be freed. Size from Config.h.

    // last used ? // or something for cleanup, in case a client registered but never requested
    pthread_rwlock_t rw_lock;  

    UT_hash_handle hh; // makes ut hashable in uthash
};

struct ChunkManager{
    struct FileChunk * chunks;  // Hashtable for chunks based on ChunkKey(public_id + file_id + chunk id)
    int chunk_count;
    int max_pool_size;
    pthread_rwlock_t rw_lock;   
};

void chunk_manager_init(struct ChunkManager * chunk_mgr);

// if chunk available in memory, then use it. else create if space available
void chunk_client_register(); 

// // when chunk is ready, broadcast to all clients via websocket.
// void chunk_ready();

// // Store a fully downloaded chunk into memory and update its state.
// void chunk_store_downloaded();

// // send chunk to one of clients who resgistered with it.
// void chunk_upload();

// client requesting a chunk. if available, send ready message. else create and download it.
// ofc if space available in pool.
void chunk_request(struct AppContext * app_ctx , const cJSON * ws_data, struct mg_connection *conn);
void add_client_to_chunk(struct FileChunk * new_chunk, int sender_public_id);
struct FileChunk * chunk_create(int public_id, int file_id, int chunk_id);

#endif