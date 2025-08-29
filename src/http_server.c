#include "http_server.h"
#include "config.h"
#include <memory.h>
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


    char *body = NULL;
    long long body_len = 0;
    int got_chunk = 0;
    int public_id = -1, file_id = -1, chunk_id = -1;
    size_t chunk_size = 0;
    unsigned char *chunk_mem = NULL;
    const char *cl = mg_get_header(conn, "Content-Length");
    if (cl) body_len = atoll(cl);
    if (body_len <= 0 || body_len > 10485760) { // 10MB max
        mg_send_http_error(conn, 400, "%s", "Invalid or missing Content-Length");
        return 400;
    }
    body = malloc((size_t)body_len + 1);
    if (!body) {
        mg_send_http_error(conn, 500, "%s", "Failed to allocate memory for body");
        return 500;
    }
    long long read = 0;
    while (read < body_len) {
        int r = mg_read(conn, body + read, (size_t)(body_len - read));
        if (r <= 0) break;
        read += r;
    }
    body[body_len] = '\0';
    // Get boundary from Content-Type
    const char *ct = mg_get_header(conn, "Content-Type");
    if (!ct || strncmp(ct, "multipart/form-data; boundary=", 30) != 0) {
        free(body);
        mg_send_http_error(conn, 400, "%s", "Invalid Content-Type");
        return 400;
    }
    const char *boundary = ct + 30;
    char boundary_str[256];
    snprintf(boundary_str, sizeof(boundary_str), "--%s", boundary);
    size_t boundary_len = strlen(boundary_str);
    char *p = body;
    char *end = body + body_len;
    while (p < end) {
        // Find next boundary
        char *b = strstr(p, boundary_str);
        if (!b) break;
        p = b + boundary_len;
        if (p >= end) break;
        if (*p == '\r' && *(p+1) == '\n') p += 2;
        else if (*p == '\n') p += 1;
        // Check for end boundary
        if (p < end && strncmp(p, "--", 2) == 0) break;
        // Parse headers
        char *header_end = strstr(p, "\r\n\r\n");
        if (!header_end) header_end = strstr(p, "\n\n");
        if (!header_end) break;
        size_t header_len = header_end - p;
        char header[512] = {0};
        size_t copy_len = header_len < sizeof(header)-1 ? header_len : sizeof(header)-1;
        memcpy(header, p, copy_len);
        header[copy_len] = '\0';
        // Find Content-Disposition and name/filename
        char name[128] = {0}, filename[256] = {0};
        char *cd = strstr(header, "Content-Disposition:");
        if (cd) {
            char *nptr = strstr(cd, "name=");
            if (nptr) {
                nptr += 5;
                if (*nptr == '"') nptr++;
                char *nend = strchr(nptr, '"');
                if (!nend) nend = strchr(nptr, ';');
                if (!nend) nend = nptr + strcspn(nptr, "\r\n;");
                size_t nlen = nend ? (size_t)(nend - nptr) : strlen(nptr);
                if (nlen < sizeof(name)) memcpy(name, nptr, nlen);
                name[nlen < sizeof(name) ? nlen : sizeof(name)-1] = '\0';
            }
            char *fptr = strstr(cd, "filename=");
            if (fptr) {
                fptr += 9;
                if (*fptr == '"') fptr++;
                char *fend = strchr(fptr, '"');
                if (!fend) fend = strchr(fptr, ';');
                if (!fend) fend = fptr + strcspn(fptr, "\r\n;");
                size_t flen = fend ? (size_t)(fend - fptr) : strlen(fptr);
                if (flen < sizeof(filename)) memcpy(filename, fptr, flen);
                filename[flen < sizeof(filename) ? flen : sizeof(filename)-1] = '\0';
            }
        }
        // Data start
        char *data = header_end;
        if (strncmp(data, "\r\n\r\n", 4) == 0) data += 4;
        else if (strncmp(data, "\n\n", 2) == 0) data += 2;
        // Data end: next boundary
        char *next_b = strstr(data, boundary_str);
        size_t data_len = next_b ? (size_t)(next_b - data) : (size_t)(end - data);
        // Remove trailing CRLF
        while (data_len > 0 && (data[data_len-1] == '\r' || data[data_len-1] == '\n')) data_len--;
        // Assign fields
        if (strcmp(name, "public_id") == 0) {
            char tmp[32] = {0};
            size_t cpy = data_len < sizeof(tmp)-1 ? data_len : sizeof(tmp)-1;
            memcpy(tmp, data, cpy);
            public_id = atoi(tmp);
        } else if (strcmp(name, "file_id") == 0) {
            char tmp[32] = {0};
            size_t cpy = data_len < sizeof(tmp)-1 ? data_len : sizeof(tmp)-1;
            memcpy(tmp, data, cpy);
            file_id = atoi(tmp);
        } else if (strcmp(name, "chunk_id") == 0) {
            char tmp[32] = {0};
            size_t cpy = data_len < sizeof(tmp)-1 ? data_len : sizeof(tmp)-1;
            memcpy(tmp, data, cpy);
            chunk_id = atoi(tmp);
        } else if (strcmp(name, "chunk") == 0) {
            chunk_mem = malloc(data_len);
            if (!chunk_mem) {
                free(body);
                mg_send_http_error(conn, 500, "%s", "Failed to allocate memory for chunk");
                return 500;
            }
            memcpy(chunk_mem, data, data_len);
            chunk_size = data_len;
            got_chunk = 1;
        }
        p = next_b ? next_b : end;
    }
    free(body);

    if (got_chunk && public_id != -1 && file_id != -1 && chunk_id != -1) {
        struct ChunkKey chunk_key = {public_id, file_id, chunk_id};
        struct FileChunk * cur_chunk = NULL;
        HASH_FIND(hh, chunk_mgr->chunks, &chunk_key, sizeof(struct ChunkKey), cur_chunk);
        pthread_rwlock_wrlock(&cur_chunk->rw_lock);
        cur_chunk->is_downloaded = 1;
        cur_chunk->data = chunk_mem;
        struct PublicIdEntry *entry, *tmp;
        HASH_ITER(hh, cur_chunk->public_ids, entry, tmp) {
            struct Client * cur_client = NULL;
            HASH_FIND(hh, app_ctx->connection_mgr.clients, &entry->public_id, sizeof(int), cur_client);
            char buffer[300];
            sprintf(buffer, "{\"opcode\":%d, \"data\":{\"public_id\":%d, \"file_id\":%d, \"chunk_id\":%d}}", SERVER_CHUNK_READY, public_id, file_id, chunk_id);
            mg_websocket_write(cur_client->conn, MG_WEBSOCKET_OPCODE_TEXT, buffer, strlen(buffer) ); 
            printf("Ready Signnal Sent to Client %d\n", entry->public_id);
        }

        pthread_rwlock_unlock(&cur_chunk->rw_lock);
        mg_send_http_ok(conn, "text/plain", 2);
        mg_write(conn, "OK", 2);
        printf("Chunk received and stored in memory (public_id=%d, file_id=%d, chunk_id=%d, size=%zu)\n", public_id, file_id, chunk_id, chunk_size);
        return 200;
    } else {
        if (chunk_mem) free(chunk_mem);
        mg_send_http_error(conn, 400, "%s", "No chunk data received or missing fields");
        return 400;
    }
}

void http_init_handlers(struct mg_context * cw_ctx, struct AppContext * app_ctx){
    mg_set_request_handler(cw_ctx, "/upload_chunk", UploadChunkHandler, app_ctx);
    mg_set_request_handler(cw_ctx, "/*", RootHandler, 0); // route to this case in all cases
}