#include "http_server.h"
#include "config.h"
#include <memory.h>
#include <stdio.h>
#include "civetweb.h"

/**
 * @brief Initializes and starts the CivetWeb server.
 *
 * This function sets up the server callbacks and configuration options,
 * then starts the CivetWeb server. The returned context pointer must be
 * freed by calling mg_stop() when the server is no longer needed.
 *
 * @return Pointer to the initialized mg_context on success, or NULL on failure.
 */
struct mg_context * http_initialize_server() {
    struct mg_callbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));

    const char *options[] = {
        "document_root", DOCUMENT_ROOT,
        "listening_ports", PORT,
        0
    };

    struct mg_context *ctx = mg_start(&callbacks, 0, options);
    if (!ctx) {
        printf("Failed to start CivetWeb server\n");
        return NULL;
    }
    // ctx is on HEAP due to start function. Only freed when called mg_stop(). 
    // Thus we can return pointer, no issue.
    return ctx;
}


// Handler for '/'
// Serves the contents of "static.html" as the response
static int RootHandler(struct mg_connection *conn, void *cbdata) {
    FILE *f = fopen(HTML_CLIENT_FILE, "rb");
    if (!f) {
        printf("file not found\n");
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

// Handler for '/upload_chunk' POST
static int UploadChunkHandler(struct mg_connection *conn, void *cbdata) {
    // Only accept POST
    if (strcmp(mg_get_request_info(conn)->request_method, "POST") != 0) {
        mg_send_http_error(conn, 405, "%s", "Method Not Allowed");
        return 405;
    }
    struct AppContext * app_ctx = (struct AppContext *)cbdata;
    struct ChunkManager * chunk_mgr = &app_ctx->chunk_mgr;


    char *chunk_data = NULL;
    char buf[128];
    int owner_public_id = -1, file_id = -1;
    size_t start_pos = 0, size = 0;

    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *query = ri->query_string;

    // Extract query parameters: owner_public_id, file_id, start_pos, size
    if (query) {
        if (mg_get_var(query, strlen(query), "owner_public_id", buf, sizeof(buf)) > 0) {
            owner_public_id = atoi(buf);
        }
        if (mg_get_var(query, strlen(query), "file_id", buf, sizeof(buf)) > 0) {
            file_id = atoi(buf);
        }
        if (mg_get_var(query, strlen(query), "start_pos", buf, sizeof(buf)) > 0) {
            start_pos = (size_t) strtoull(buf, NULL, 10);
        }
        if (mg_get_var(query, strlen(query), "size", buf, sizeof(buf)) > 0) {
            size = (size_t) strtoull(buf, NULL, 10);
        }
    }
    if (owner_public_id < 0 || file_id < 0){
        printf("invalid download query parameters \n");
        mg_send_http_error(conn, 400, "%s", "Missing or invalid query parameters");
        return 400;
    }
    

    // Allocate buffer for chunk data
    long content_length = ri->content_length;
    if (content_length != size) {
        printf("invalid chunk size %ld\n", content_length);
        mg_send_http_error(conn, 400, "%s", "Invalid content length");
        return 400;
    }

    chunk_data = malloc((size_t)content_length);
    if (!chunk_data) {
        printf("cannot malloc \n");
        mg_send_http_error(conn, 500, "%s", "Failed to allocate memory for chunk");
        return 500;
    }


    long bytes_read = 0;
    while (bytes_read < content_length) {
        int r = mg_read(conn, chunk_data + bytes_read, (size_t)(content_length - bytes_read));
        if (r <= 0) {
            free(chunk_data);
            printf("failed to read chunk \n");
            mg_send_http_error(conn, 400, "%s", "Failed to read chunk data");
            return 400;
        }
        bytes_read += r;
    }

    pthread_rwlock_wrlock(&chunk_mgr->rw_lock);

    // Lookup chunk in memory
    struct ChunkKey chunk_key = {0}; // very important zero out key. bcz uthash will cmp full struct to struct. may cause errors.
    chunk_key.public_id = owner_public_id;
    chunk_key.file_id = file_id;
    chunk_key.start_pos = start_pos;
    chunk_key.size = size;
    
    // Store chunk in memory and notify clients (reuse previous logic)
    struct FileChunk * cur_chunk = NULL;
    HASH_FIND(hh, chunk_mgr->chunks, &chunk_key, sizeof(struct ChunkKey), cur_chunk);
    if (!cur_chunk) {
        free(chunk_data);
        printf("chunk not found \n");
        mg_send_http_error(conn, 404, "%s", "Chunk not found");
        pthread_rwlock_unlock(&chunk_mgr->rw_lock);
        return 404;
    }
    pthread_rwlock_wrlock(&cur_chunk->rw_lock);
    cur_chunk->is_downloaded = 1;
    cur_chunk->data = (unsigned char *)chunk_data;
    cur_chunk->size = bytes_read;
    struct PublicIdEntry *entry, *tmp;
    HASH_ITER(hh, cur_chunk->public_ids, entry, tmp) {
        struct mg_connection * target_conn = NULL;
        if (entry->public_id == 0){
            target_conn = app_ctx->connection_mgr.server.conn;
        }else{
            struct Client * cur_client = NULL;
            HASH_FIND(hh, app_ctx->connection_mgr.clients, &entry->public_id, sizeof(int), cur_client);
            target_conn = cur_client->conn;
        }
        if (target_conn) {
            char buffer[300];
            sprintf(buffer, "{\"opcode\":%d, \"data\":{\"owner_public_id\":%d, \"file_id\":%d, \"start_pos\":%zu, \"size\":%zu}}", CHUNK_READY, owner_public_id, file_id, start_pos, size);
            int write_res = mg_websocket_write(target_conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer));
            printf("Ready Signal Sent to Client %d with res: %d\n", entry->public_id, write_res);
        }
    }
    pthread_rwlock_unlock(&cur_chunk->rw_lock);
    pthread_rwlock_unlock(&chunk_mgr->rw_lock);
    mg_send_http_ok(conn, "text/plain", 2);
    mg_write(conn, "OK", 2);
    printf("Chunk received and stored in memory (owner_public_id=%d, file_id=%d, start_pos=%d, size:%zu, bytes_read=%ld)\n", owner_public_id, file_id, start_pos, size, bytes_read);
    return 200;

}

static int DownloadChunkHandler(struct mg_connection *conn, void *cbdata) {
    struct AppContext * app_ctx = (struct AppContext *)cbdata;
    struct ChunkManager * chunk_mgr = &app_ctx->chunk_mgr;

    char buf[128];
    int owner_public_id = -1, file_id = -1;
    size_t start_pos = 0, size = 0;

    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *query = ri->query_string;

    // Extract query parameters: owner_public_id, file_id, start_pos, size
    if (query) {
        if (mg_get_var(query, strlen(query), "owner_public_id", buf, sizeof(buf)) > 0) {
            owner_public_id = atoi(buf);
        }
        if (mg_get_var(query, strlen(query), "file_id", buf, sizeof(buf)) > 0) {
            file_id = atoi(buf);
        }
        if (mg_get_var(query, strlen(query), "start_pos", buf, sizeof(buf)) > 0) {
            start_pos = (size_t) strtoull(buf, NULL, 10);
        }
        if (mg_get_var(query, strlen(query), "size", buf, sizeof(buf)) > 0) {
            size = (size_t) strtoull(buf, NULL, 10);
        }
    }
    if (owner_public_id < 0 || file_id < 0){
        printf("invalid download query parameters \n");
        mg_send_http_error(conn, 400, "%s", "Missing or invalid query parameters");
        return 400;
    }

    // Lookup chunk in memory
    struct ChunkKey chunk_key = {0}; // very important zero out key. bcz uthash will cmp full struct to struct. may cause errors.
    chunk_key.public_id = owner_public_id;
    chunk_key.file_id = file_id;
    chunk_key.start_pos = start_pos;
    chunk_key.size = size;
    
    struct FileChunk * cur_chunk = NULL;
    HASH_FIND(hh, chunk_mgr->chunks, &chunk_key, sizeof(struct ChunkKey), cur_chunk);
    if (!cur_chunk || !cur_chunk->is_downloaded || !cur_chunk->data) {
        printf("chunk not found or not uploaded yet\n");
        mg_send_http_error(conn, 404, "%s", "Chunk not found");
        return 404;
    }

    // Send chunk as raw binary
    pthread_rwlock_rdlock(&cur_chunk->rw_lock);
    mg_send_http_ok(conn, "application/octet-stream", cur_chunk->size);
    mg_write(conn, cur_chunk->data, cur_chunk->size);
    pthread_rwlock_unlock(&cur_chunk->rw_lock);
    printf("Chunk served (owner_public_id=%d, file_id=%d, start_pos=%zu, size=%zu)\n", owner_public_id, file_id, start_pos, size);
    return 200;
}


void http_init_handlers(struct mg_context * cw_ctx, struct AppContext * app_ctx){
    mg_set_request_handler(cw_ctx, "/upload_chunk", UploadChunkHandler, app_ctx);
    mg_set_request_handler(cw_ctx, "/download_chunk", DownloadChunkHandler, app_ctx);
    // mg_set_request_handler(cw_ctx, "**", RootHandler, 0); // router all paths to default one. In case user put something else in navbar.
}