#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H

#include <pthread.h>

#include "config.h"
#include "civetweb.h"

// forward declaring - to avoid loop
struct AppContext;

enum WsOPCodes{
    CLIENT_REGISTER = 0,
    UI_REGISTER,
    PUBLIC_ID,  // Sending your public name
    ADD_CLIENT,
    UI_APPROVE_CLIENT,
    SERVER_APPROVE_CLIENT,
    UI_DIS_APPROVE_CLIENT,
    SERVER_DIS_APPROVE_CLIENT,
    CLIENT_ADD_FILES,  // HEY ADD THIS [LIST] OF FILES I HAVE TO YOUR DB
    CLIENT_REMOVE_FILE,  // REMOVE THIS SINGLE FILE
    UI_ADD_FILES,            // 10
    UI_REMOVE_FILE,

    REQUEST_CHUNK,  // SERVER WILL FIND THAT CHUNK AND LOAD INTO MEMORY
    SERVER_UPLOAD_CHUNK, // SERVER ASKING CHUNK OWNER TO UPLOAD THAT CHUNK TO SERVER
    SERVER_CHUNK_READY, // SERVER NOTIFYING PPL WHO WANT THAT CHUNK THAT IT IS READY

    UPLOAD_FILE, // ask client to upload this file.
    UPDATE_NAME, // ask other party to UPDATE SENDER NAME [HELPFUL IN PATRY MODE]
    REGISTER, // send a uiique uuid, and ask server to register it
    RECONNECT, // send previous UUID and try to reconnect.
    ASK_FILES,   // GIMME ALL FILES YOU GOT 
};

void ws_start( struct mg_context * cw_ctx, struct AppContext * app_ctx);

int ws_manager_destroys();


#endif