#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H

#include "config.h"
#include "civetweb.h"
// #include <pthread.h> // intellisense may give error on Windows if windows phthread lib not included properly.

struct ws_manager{
    struct mg_connection *clients[MAX_WEB_SOCKET_CLIENTS];
    int client_count;
};

void ws_start(struct mg_context * ctx, struct ws_manager * ws_mgr);

int ws_manager_init(struct ws_manager * ws_mgr);
int ws_manager_destroys(struct ws_manager * ws_mgr);


#endif