#include <stdio.h>
#include "ui.h"
#include "connection_manager.h"
#include "app_context.h"
#include <pthread.h>

int main(void) {
    struct AppContext app_ctx = {0};
    
    // CONNECTIONS: start listening to connections on antoher thread
    pthread_t th;
    pthread_create(&th, NULL, start_connections, (void * )&app_ctx); 
    
    // SCREEN : init screen_size, UI, etc 
    if (ui_init(&app_ctx) != 0){    
        return 1;
    }

    cm_init(&app_ctx.connection_mgr);


    // Run the webview
    webview_run(app_ctx.w); // Must run on main thread

    // CLEANUP
    webview_destroy(app_ctx.w);
    pthread_join(th, NULL);
    return 0;
}
