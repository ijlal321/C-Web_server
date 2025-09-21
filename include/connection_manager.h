#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <pthread.h>
#include "master_app.h"
#include "client.h"
#include "cJSON.h"


struct ConnectionManager{
    struct Client * clients;
    struct MasterApp master_app;
    pthread_rwlock_t rwlock;  // for managing clients.
};

void cm_init(struct ConnectionManager * connection_mgr);
void * start_connections(void * args);

struct Client * cm_add_client(struct ConnectionManager * connection_mgr, struct mg_connection *conn, const cJSON *data);

void cm_send_public_id_to_client(struct mg_connection * conn, int public_id);

void cm_registered_client_send_ack(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_add_client_to_UI(struct MasterApp * server, struct Client * client);

void cm_server_not_ready_handle(struct mg_connection *conn, cJSON * root);

int cm_register_master_app(struct ConnectionManager * connection_mgr, struct mg_connection *conn);

void cm_send_master_app_registered_ack(struct MasterApp * server, int res);

void cm_approve_client(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_notify_client_approved(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

int cm_set_client_approval(struct ConnectionManager * connection_mgr, const cJSON * ws_data, int approved);

void cm_broadcast_client_approval(struct ConnectionManager * connection_mgr, const cJSON * ws_data,  const cJSON * root);

int cm_add_files(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_send_files_to_UI(struct MasterApp * server, const cJSON * ws_data);

void cm_broadcast_new_file(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_remove_file_req_to_master_app(struct ConnectionManager * connection_mgr, const cJSON * root);

void cm_send_files_to_master_app(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_broadcast_remove_file(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_remove_files(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_remove_files_from_UI(struct MasterApp * server, const cJSON * ws_data);

void cm_server_add_files(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_send_files_to_client(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_remove_server_files(struct ConnectionManager * connection_mgr, const cJSON * ws_data);

void cm_remove_files_from_clients(struct ConnectionManager * connection_mgr, const cJSON * ws_data);




#endif