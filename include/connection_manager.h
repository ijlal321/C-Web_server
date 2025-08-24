#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <pthread.h>
#include "server.h"
#include "client.h"


struct ConnectionManager{
    struct Client * clients;
    struct Server server;
    pthread_rwlock_t rwlock;
    int id;
};

void cm_init(struct ConnectionManager * connection_mgr);
void * start_connections(void * args);

struct Client * cm_add_client(struct ConnectionManager * connection_mgr, struct mg_connection *conn);

// add Client
// find Client
// approve Client
// remove Client
// cm_destroy

// websocket takes connection manager as input.


#endif