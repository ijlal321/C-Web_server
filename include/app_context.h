// app_context.h
#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "web_socket.h"
#include "file_tracker.h"

struct AppContext {
    struct WsManager ws_mgr;
    struct FileTracker file_tracker;
};

#endif
