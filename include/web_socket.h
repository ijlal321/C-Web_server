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
    PUBLIC_NAME,  // Sending your public name
    ADD_CLIENT,
    UI_APPROVE_CLIENT,
    SERVER_APPROVE_CLIENT,
    UI_DIS_APPROVE_CLIENT,
    SERVER_DIS_APPROVE_CLIENT,
    ADD_FILES,  // HEY ADD THIS [LIST] OF FILES I HAVE TO YOUR DB
    REMOVE_FILE,  // REMOVE THIS SINGLE FILE
    ASK_FILES,   // GIMME ALL FILES YOU GOT 
    UPLOAD_FILE, // ask client to upload this file.
    UPDATE_NAME, // ask other party to UPDATE SENDER NAME [HELPFUL IN PATRY MODE]
    REGISTER, // send a uiique uuid, and ask server to register it
    RECONNECT, // send previous UUID and try to reconnect.
};

void ws_start( struct mg_context * cw_ctx, struct AppContext * app_ctx);

int ws_manager_destroys();


#endif