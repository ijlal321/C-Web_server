#ifndef MASTER_APP_H
#define MASTER_APP_H

// SERVER_H
struct MasterApp{
    int public_id; 
    char public_name[NAME_STRING_SIZE];
    struct mg_connection * conn;
    pthread_rwlock_t rwlock;
};

#endif