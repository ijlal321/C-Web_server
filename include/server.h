#ifndef SERVER_H
#define SERVER_H

struct Server{
    int public_id; 
    char public_name[NAME_STRING_SIZE];
    struct mg_connection * conn;
    pthread_rwlock_t rwlock;
};

#endif