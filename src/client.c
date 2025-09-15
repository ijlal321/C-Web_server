#include <stdio.h>
#include "client.h"

struct Client * client_create_new(const char * private_id, int public_id, const char * public_name, struct mg_connection * conn ){
    
    struct Client * new_client = malloc(sizeof(struct Client));
    
    // strlen donot include \0. PRIVATE_ID_SIZE includes space for \0
    //  32  !=  33 - 1
    if (strlen(private_id) != PRIVATE_ID_SIZE-1){
        printf("Private Id size not correct.\n");
        free(new_client);
        return NULL;
    }

    // even though private_key is already null teminated by cJSON (32 bytes + \0), we still do manual check. Habbit and clarity I guess.
    strncpy(new_client->private_id, private_id, PRIVATE_ID_SIZE-1); // copy 32 bytes
    new_client->private_id[PRIVATE_ID_SIZE-1] = '\0'; // put null terminator in last.

    // for public name, we just take what it fit.
    strncpy(new_client->public_name, public_name, NAME_STRING_SIZE-1);
    new_client->public_name[NAME_STRING_SIZE-1] = '\0';

    new_client->conn = conn;
    new_client->approved = 0;
    new_client->public_id = public_id;
    new_client->files = NULL;
    int res = pthread_rwlock_init(&new_client->rwlock, NULL);
    if (res != 0){
        printf("Failed to Initiate rw_lock for new client. Closing Connection.\n");
        free(new_client);
        return NULL;
    }
    return new_client;
}

struct Client * client_find_by_public_id(struct Client * clients, int public_id){
    struct Client * client = NULL;
    HASH_FIND_INT(clients, &public_id, client);
    if (client == NULL){
        printf("Client not exist for public id\n");
        return NULL;
    }
    return client;
}