#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <pthread.h>
#include "config.h"

/*
    Aim of this file is to use WebSocket communiation to 
    keep track of all files available on server or clients.
*/

struct FileInfo{
    char * name;  // The file name (without path)
    size_t size;    // File size in bytes
    char * type;  // MIME type (e.g. "image/png", "application/pdf")
    struct mg_connection *conn;  // we can get name etc from here.
    int is_transfering; // keep track if a file is being transfered.
};


struct Ws_Client{
    struct mg_connection *conn;        // WebSocket connection
    char *client_id;                   // Optional: user ID, token, etc.
    struct FileInfo files[DEFAULT_CLIENT_MAX_FILES_SIZE];    // We will double it if needed. 
    int nr_files;               
    pthread_mutex_t lock;             
    // I dont think we will be needing condition variables here.
};

struct Ws_Server{
    char * server_name;  // do i need it ? also should i use array ?
    struct FileInfo files[DEFAULT_CLIENT_MAX_FILES_SIZE];    // We will double it if needed. 
    int nr_files;             
    pthread_mutex_t lock;             
    // I dont think we will be needing condition variables here.
};

struct WsFileManager{
    struct Ws_Client clients[MAX_WEB_SOCKET_CLIENTS];
    int client_count;
    struct Ws_Server Server;
    pthread_mutex_t lock;             
    // I dont think we will be needing condition variables here.
};


#endif