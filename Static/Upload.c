
#include <stdio.h>
#include <string.h>
#include "../external/civetweb/include/civetweb.h"

// Handler for /upload/init
static int handle_upload_init(struct mg_connection *conn, void *cbdata) {
	const char *resp = "{\"uploadId\": \"dummy123\"}";
	mg_send_http_ok(conn, "application/json", strlen(resp));
	mg_write(conn, resp, strlen(resp));
	return 200;
}

// Handler for /upload/{uploadId}/{index}
static int handle_upload_chunk(struct mg_connection *conn, void *cbdata) {

    // ========= Verify Request Type ================
    if (strcmp(mg_get_request_info(conn)->request_method, "PUT") != 0) {
    return 405; // Method Not Allowed
    }  // check for put sv post. later decide what we want here.


    // ========= get field id and chunk id ================
    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *uri = ri->local_uri ? ri->local_uri : ri->request_uri;
    // uri is like "/upload/UPLOADID/CHUNKID"
    char upload_id[128] = {0};
    char chunk_id[32] = {0};
    sscanf(uri, "/upload/%127[^/]/%31[^/]", upload_id, chunk_id);
    // TODO: verification


	char buf[4096];  // TODO: Use global Buffer Size
	int n;
	while ((n = mg_read(conn, buf, sizeof(buf))) > 0) {
		// do somtrhing with data
	}
	mg_send_http_ok(conn, "text/plain", 0);
	return 200;
}

// Handler for /upload/{uploadId}/complete
static int handle_upload_complete(struct mg_connection *conn, void *cbdata) {
	char buf[4096];
	int n;
	while ((n = mg_read(conn, buf, sizeof(buf))) > 0) {
		// Discard
	}
	mg_send_http_ok(conn, "text/plain", 0);
	return 200;
}

// Register handlers (call this from your main server setup)
void register_upload_handlers(struct mg_context *ctx) {
	mg_set_request_handler(ctx, "/upload/init", handle_upload_init, 0);
	mg_set_request_handler(ctx, "/upload/*/[0-9]+", handle_upload_chunk, 0); // crude pattern
	mg_set_request_handler(ctx, "/upload/*/complete", handle_upload_complete, 0);
}
