#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H



struct FileInfo{
    char * name;  // The file name (without path)
    size_t size;    // File size in bytes
    char * type;  // MIME type (e.g. "image/png", "application/pdf")
    struct mg_connection *conn;  // we can get name etc from here.
    int is_transfering; // keep track if a file is being transfered.
};

struct AvailableFiles{
    FileInfo on_server[100]; // array holding files on server
    FileInfo on_clients[100];  // array holding files on clients

    int server_count; // nr files available on server
    int client_count; // (collective) nr files available on all clients
};


#endif