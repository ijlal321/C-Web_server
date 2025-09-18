#include <stdio.h>
#include "ui.h"
#include "connection_manager.h"
#include "app_context.h"
#include <pthread.h>

int main(void) {
    struct AppContext app_ctx = {0};
    chunk_manager_init(&app_ctx.chunk_mgr);
    cm_init(&app_ctx.connection_mgr);

    
    // CONNECTIONS: start listening to connections on antoher thread
    pthread_t th;
    pthread_create(&th, NULL, start_connections, (void * )&app_ctx); 
    
    while(1){

    };

    // SCREEN : init screen_size, UI, etc 
    webview_t w = ui_init();
    if (w == NULL) {
        return 1;
    }

    // Run the webview
    webview_run(w); // Must run on main thread

    // CLEANUP
    webview_destroy(w);
    pthread_join(th, NULL);
    return 0;
}
