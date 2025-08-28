#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "app_context.h"



struct mg_context * http_initialize_server();

void http_init_handlers(struct mg_context * cw_ctx, struct AppContext * app_ctx);



#endif