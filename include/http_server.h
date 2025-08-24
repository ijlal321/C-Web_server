#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

struct mg_context * http_initialize_server();

void http_init_handlers(struct mg_context * ctx);



#endif