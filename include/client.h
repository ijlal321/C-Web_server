#ifndef CLIENT_H
#define CLIENT_H

#include "uthash.h"
#include "config.h"

struct Client{
    char private_id[PRIVATE_ID_SIZE];  
    int public_id; 
    char public_name[NAME_STRING_SIZE];
    struct mg_connection * conn;
    int approved;
    pthread_rwlock_t rwlock;
    // files ?
    UT_hash_handle hh; // makes ut hashable in uthash
};

#endif