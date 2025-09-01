#ifndef CLIENT_H
#define CLIENT_H

#include "uthash.h"
#include "config.h"
#include <pthread.h>

struct Client{
    char private_id[PRIVATE_ID_SIZE];// PRIVATE_ID_SIZE = 33  
    int public_id; 
    char public_name[NAME_STRING_SIZE];
    struct mg_connection * conn;
    int approved;
    pthread_rwlock_t rwlock;
    struct File * files; 
    UT_hash_handle hh; // makes ut hashable in uthash
};

struct Client * client_create_new(const char * private_id, int public_id, const char * public_name, struct mg_connection * conn );

#endif