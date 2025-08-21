#include<stdio.h>
#include "civetweb.h"


#define CHUNK_SIZE  1 * 1024 * 1024 // 1 MB




static int handle_download_chunk(struct mg_connection *conn, void *cbdata) {

    const struct mg_request_info *ri = mg_get_request_info(conn);

    // Extract file_id and chunk_index from the URI
    const char *file_id_str = mg_get_request_info_param(ri, "file_id");
    const char *chunk_index_str = mg_get_request_info_param(ri, "chunk_index");

    if (!file_id_str || !chunk_index_str) {
        mg_send_http_error(conn, 400, "%s", "Missing file_id or chunk_index");
        return 400;  // just for clarity. mg_send_http_error alreadsreturns send 400
    }

    int chunk_index = atoi(chunk_index_str);

    // Open the file (assume path based on file_id)
    char path[256];
    snprintf(path, sizeof(path), "./uploads/%s.bin", file_id_str);

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        mg_send_http_error(conn, 404, "%s", "File Not Found");
        return 404; // just for clarity
    }

    // Calculate offset
    long offset = (long) chunk_index * CHUNK_SIZE;

    if (fseek(fp, offset, SEEK_SET) != 0) {
        fclose(fp);
        mg_send_http_error(conn, 416, "%s", "Range Not Satisfiable");
        return 416;
    }

    // Read chunk
    char buffer[CHUNK_SIZE];
    size_t read_bytes = fread(buffer, 1, CHUNK_SIZE, fp);
    fclose(fp);

    // Send response with chunk
    mg_send_http_ok(conn, "application/octet-stream", read_bytes);
    mg_write(conn, buffer, read_bytes);

    return 200;
}