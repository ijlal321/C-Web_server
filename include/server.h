#ifndef SERVER_H
#define SERVER_H

struct mg_context * initialize_server();

void init_handlers(struct mg_context * ctx);



#endif