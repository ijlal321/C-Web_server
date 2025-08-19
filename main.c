#include "external/civetweb/include/civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



static int
handler(struct mg_connection *conn, void *ignored)
{
	const char *msg = "Hello world";
	unsigned long len = (unsigned long)strlen(msg);

	mg_send_http_ok(conn, "text/plain", len);

	mg_write(conn, msg, len);

	return 200; /* HTTP state 200 = OK */
}


int main()
{
    struct mg_context *ctx;

    mg_init_library(0);

    const char *options[] = {
        "listening_ports", "8080",
        NULL
    };

    ctx = mg_start(NULL, 0, options);

    if (!ctx) {
        printf("Failed to start CivetWeb server\n");
        return 1;
    }

    // Register handler for all URIs
    mg_set_request_handler(ctx, "/", handler, NULL);

    printf("Server listening on http://localhost:8080/\n");
    getchar();
    mg_stop(ctx);
    mg_exit_library();
    return 0;
}