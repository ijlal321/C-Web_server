#ifndef CLIENT_H
#define CLIENT_H

#include "uthash.h"

struct Client{
    int id;
    struct mg_connection * conn;
    int approved;
    pthread_rwlock_t rwlock;
    // files ?
    UT_hash_handle hh; // makes ut hashable in uthash
};

#endif