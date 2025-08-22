#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H
#include <pthread.h>

#include "config.h"
#include "civetweb.h"

// forward declaring - to avoid loop
struct AppContext;


struct WsManager{
    struct mg_connection * clients[MAX_WEB_SOCKET_CLIENTS];
    int client_count;
    pthread_mutex_t lock;             
    // I dont think we will be needing condition variables here.
};

void ws_start(struct mg_context * ctx, struct AppContext * app_ctx);

int ws_manager_init(struct WsManager * ws_mgr);
int ws_manager_destroys(struct WsManager * ws_mgr);


#endif