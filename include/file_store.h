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


#endif