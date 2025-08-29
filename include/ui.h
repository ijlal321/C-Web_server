#ifndef UI_C
#define UI_C

#include "webview/webview.h"
#include "app_context.h"


webview_t ui_init();

webview_t ui_webview_window_init();

int ui_set_html(webview_t w);

// int ui_bind_all_functions(struct AppContext * ctx);



#endif