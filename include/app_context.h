// app_context.h
#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "web_socket.h"
#include "connection_manager.h"
#include "chunk_manager.h"

struct AppContext {
    struct ConnectionManager connection_mgr;
    struct ChunkManager chunk_mgr;
};

#endif
