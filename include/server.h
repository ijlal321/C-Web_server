#ifndef SERVER_H
#define SERVER_H

#include "file_store.h"

struct Server{
    struct File * files;   
    int public_id; 
    char public_name[NAME_STRING_SIZE];
    struct mg_connection * conn;
    pthread_rwlock_t rwlock;
};

#endif