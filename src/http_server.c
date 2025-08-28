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
    printf("found req\n");
    // Only accept POST
    if (strcmp(mg_get_request_info(conn)->request_method, "POST") != 0) {
        mg_send_http_error(conn, 405, "%s", "Method Not Allowed");
        return 405;
    }

    char public_id[32] = {0}, file_id[32] = {0}, chunk_id[32] = {0};
    const char *cl;
    long long content_len = 0;
    cl = mg_get_header(conn, "Content-Length");
    if (cl) content_len = atoll(cl);

    // Parse multipart form data
    // For simplicity, use CivetWeb's mg_parse_multipart
    char buf[1024];
    int n;
    FILE *out = NULL;
    int got_chunk = 0;
    while ((n = mg_read(conn, buf, sizeof(buf))) > 0) {
        // In a real implementation, parse multipart boundaries and fields.
        // Here, just write to a temp file for demonstration.
        if (!out) {
            out = fopen("received_chunk.tmp", "wb");
            if (!out) {
                mg_send_http_error(conn, 500, "%s", "Failed to open file");
                return 500;
            }
        }
        fwrite(buf, 1, n, out);
        got_chunk = 1;
    }
    if (out) fclose(out);

    if (got_chunk) {
        mg_send_http_ok(conn, "text/plain", 2);
        mg_write(conn, "OK", 2);
        printf("Chunk received and saved as received_chunk.tmp\n");
        return 200;
    } else {
        mg_send_http_error(conn, 400, "%s", "No chunk data received");
        return 400;
    }
}

void http_init_handlers(struct mg_context * cw_ctx, struct AppContext * app_ctx){
    mg_set_request_handler(cw_ctx, "/upload_chunk", UploadChunkHandler, 0);
    mg_set_request_handler(cw_ctx, "/*", RootHandler, 0); // route to this case in all cases
}