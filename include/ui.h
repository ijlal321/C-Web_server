#ifndef UI_C
#define UI_C

#include "webview/webview.h"
#include "app_context.h"


int ui_init(struct AppContext * ctx);

webview_t ui_webview_window_init();

int ui_set_html(webview_t w);

// int ui_bind_all_functions(struct AppContext * ctx);



#endif