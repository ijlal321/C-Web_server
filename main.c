
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "external/civetweb/include/civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DOCUMENT_ROOT "."
#define PORT "8080"

// Handler for '/'
static int RootHandler(struct mg_connection *conn, void *cbdata) {
    const char *msg = "<html><body><h1>Welcome to the CivetWeb C server!</h1><p>Try <a href=\"/file\">/file</a> to see the file.html.</p></body></html>\n";
    size_t len = strlen(msg);
    mg_send_http_ok(conn, "text/html", len);
    mg_write(conn, msg, len);
    return 200;
}

// Handler for '/file'
static int FileHandler(struct mg_connection *conn, void *cbdata) {
    FILE *f = fopen("file.html", "rb");
    if (!f) {
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

int main() {
    struct mg_context *ctx;
    struct mg_callbacks callbacks;
    const char *options[] = {
        "document_root", DOCUMENT_ROOT,
        "listening_ports", PORT,
        0
    };

    memset(&callbacks, 0, sizeof(callbacks));
    ctx = mg_start(&callbacks, 0, options);
    if (!ctx) {
        printf("Failed to start CivetWeb server\n");
        return 1;
    }

    mg_set_request_handler(ctx, "/", RootHandler, 0);
    mg_set_request_handler(ctx, "/file", FileHandler, 0);

    printf("Server listening on http://localhost:%s/\n", PORT);

    // Run until interrupted
    while (1) {
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    mg_stop(ctx);
    return 0;
}