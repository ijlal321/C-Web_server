#include <stdio.h>

#include "config.h"
#include "ui.h"
#include "app_context.h"


webview_t ui_webview_window_init(){
    webview_t w = webview_create(0, NULL);
    if (w == NULL){
        printf("Cannot Initialize Webview for some reaosn\n");
        return NULL;
    }
    webview_error_t title_error = webview_set_title(w, UI_WINDOW_TITLE);
    if (title_error != WEBVIEW_ERROR_OK){
        printf("Cannot Set Title for window.\n");
        return NULL;    
    }
    webview_error_t size_error = webview_set_size(w, UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT, WEBVIEW_HINT_NONE);  
    if (size_error != WEBVIEW_ERROR_OK){
        printf("Cannot Resize Windows.\n");
        return NULL;    
    }
    return w;
}


int ui_set_html(webview_t w){
    webview_error_t error = webview_set_html(w,
    "<button onclick=\"window.say_hello('Hi from JS').then(alert)\">Call C from JS</button>"
    "<button onclick=\"window.external.invoke('run_eval')\">Call JS from C</button>"
    "<script>"
    "window.external = window.external || {};"
    "window.external.invoke = function(msg) {"
    "  if(msg === 'run_eval') {"
    "    alert('This alert is from JS, triggered by C!');"
    "  }"
    "};"
    "</script>"
  );
  if (error != WEBVIEW_ERROR_OK){
    printf("Cannot place WEB data in UI\n");
    return 1;
  }
  return 0;

}

int ui_bind_all_functions(struct AppContext * ctx){
    //   webview_bind(w, "say_hello", say_hello, w);
    return 0;
}

int ui_init(struct AppContext * ctx){
    // Initialize the webview Window
    ctx->w = ui_webview_window_init();
    if (ctx->w == NULL) {
        fprintf(stderr, "❌ Failed to create display window\n");
        return 1;
    }

    // Bind C functions to JavaScript
    if (ui_bind_all_functions(ctx) != 0) {
        fprintf(stderr, "❌ Failed to bind JavaScript functions\n");
        webview_destroy(ctx->w);
        return 1;
    }

    // Load HTML content
    if (ui_set_html(ctx->w) != 0) {
        fprintf(stderr, "❌ Failed to set HTML content\n");
        webview_destroy(ctx->w);
        return 1;
    }
    return 0;
}

