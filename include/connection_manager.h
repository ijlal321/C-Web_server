#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <pthread.h>
#include "server.h"
#include "client.h"
#include "cJSON.h"


struct ConnectionManager{
    struct Client * clients;
    struct Server server;
    pthread_rwlock_t rwlock;
};

void cm_init(struct ConnectionManager * connection_mgr);
void * start_connections(void * args);

struct Client * cm_add_client(struct ConnectionManager * connection_mgr, struct mg_connection *conn, const cJSON *data);

void cm_send_public_id_to_client(struct mg_connection * conn, int public_id);

void cm_add_client_to_UI(struct Server * server, struct Client * client);

void cm_register_server(struct ConnectionManager * connection_mgr, struct mg_connection *conn);


// add Client
// find Client
// approve Client
// remove Client
// cm_destroy

// websocket takes connection manager as input.


#endif