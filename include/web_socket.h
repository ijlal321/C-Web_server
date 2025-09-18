#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H

#include <pthread.h>

#include "config.h"
#include "civetweb.h"

// forward declaring - to avoid loop
struct AppContext;

enum WsOPCodes{
    CLIENT_REGISTER = 0,  // client asking server to register
    CLIENT_REGISTER_ACK, // PUBLIC_ID    // tell client if registered or not + public id sent
    NEW_CLIENT_REGISTERED,  // tell master app a new client registered

    MASTER_APP_REGISTER, // master app asking to register
    MASTER_APP_REGISTER_ACK,    // send ack to master app telling if auth successful

    APPROVE_CLIENT, // MASTER APP tell server to approve this client
    DIS_APPROVE_CLIENT,  // .. disapprove this client
    CLIENT_APPROVED,    // server tell everyone this client is approved
    CLIENT_DIS_APPROVED, //  ... dissapproved

    ADD_FILES,   // Client: HEY ADD THIS [LIST] OF FILES I HAVE TO YOUR DB
    REMOVE_FILE,    // client telling server to remove this file (singular)
    FILES_ADDED,    // server broadcast new file list
    FILE_REMOVED,   // server broadcast removed file (singlular)


    REQUEST_CHUNK,  // CLIENT ASK SERVER TO READY THIS CHUNK
    UPLOAD_CHUNK_REQUEST, // SERVER ASKING CHUNK OWNER TO UPLOAD THAT CHUNK TO SERVER
    CHUNK_READY, // SERVER NOTIFYING PPL WHO WANT THAT CHUNK THAT IT IS READY

    UPLOAD_FILE, // ask client to upload this file.
    UPDATE_NAME, // ask other party to UPDATE SENDER NAME [HELPFUL IN PATRY MODE]
    REGISTER, // send a uiique uuid, and ask server to register it
    RECONNECT, // send previous UUID and try to reconnect.
    ASK_FILES,   // GIMME ALL FILES YOU GOT 
};

void ws_start( struct mg_context * cw_ctx, struct AppContext * app_ctx);

int ws_manager_destroys();


#endif