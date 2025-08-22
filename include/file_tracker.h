#ifndef FILE_TRACKER_H
#define FILE_TRACKER_H

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


struct ClientFiles{
    struct mg_connection *conn;        // WebSocket connection
    char *client_id;                   // Optional: user ID, token, etc.
    struct FileInfo files[DEFAULT_CLIENT_MAX_FILES_SIZE];    // We will double it if needed. 
    int file_count;               
    pthread_mutex_t lock;             
    // I dont think we will be needing condition variables here.
};

struct ServerFiles{
    char server_name[64];  // do i need it ? also should i use array ?
    struct FileInfo files[DEFAULT_CLIENT_MAX_FILES_SIZE];    // We will double it if needed. 
    int file_count;             
    pthread_mutex_t lock;             
    // I dont think we will be needing condition variables here.
};

struct FileTracker{
    struct ClientFiles clients[MAX_WEB_SOCKET_CLIENTS];
    int client_count;
    struct ServerFiles server;
    pthread_mutex_t lock;             
    // I dont think we will be needing condition variables here.
};

enum WsOPCodes{
    ADD_FILES = 0,  // HEY ADD THIS [LIST] OF FILES I HAVE TO YOUR DB
    REMOVE_FILE,  // REMOVE THIS SINGLE FILE
    ASK_FILES   // GIMME ALL FILES YOU GOT 
};

// ========= Functions ============

int file_tracker_init(struct FileTracker * file_tracker);


struct ClientFiles * search_client_in_accepted_clients(struct mg_connection *conn, struct FileTracker * file_tracker);
#endif