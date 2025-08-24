#ifndef FILE_STORE_H
#define FILE_STORE_H

#include "uthash.h"

struct File{
    char name[128];
    size_t size;
    // char * type; ? ` // MIME type (e.g. "image/png", "application/pdf")
    int is_transfering;
    UT_hash_handle hh;
};

enum WsOPCodes{
    ADD_FILES = 0,  // HEY ADD THIS [LIST] OF FILES I HAVE TO YOUR DB
    REMOVE_FILE,  // REMOVE THIS SINGLE FILE
    ASK_FILES,   // GIMME ALL FILES YOU GOT 
    UPLOAD_FILE, // ask client to upload this file.
    UPDATE_NAME, // ask other party to UPDATE SENDER NAME [HELPFUL IN PATRY MODE]
};


#endif