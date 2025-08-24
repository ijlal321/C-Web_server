#include <stdio.h>
#include <stdlib.h>
#include "webview/webview.h"

// Include Windows threading library
#include <windows.h>

void dispatch_eval(webview_t w, void *arg) {
    webview_eval(w, "window.external.invoke('run_eval')");
}

// Thread function to call webview_eval
DWORD WINAPI call_eval_fn(LPVOID args){
    webview_t w = (webview_t)args;
    // Sleep for 2 seconds before calling eval
    Sleep(2000);
  webview_dispatch(w, dispatch_eval, NULL);
    return 0;
}

// Correct signature for the callback
void say_hello(const char *id, const char *req, void *arg) {
  printf("JS called C with: %s\n", req);
  // Respond to JS (id, status, result)
  webview_return((webview_t)arg, id, 0, "\"Hello from C!\"");
}

int main(void) {
  webview_t w = webview_create(0, NULL);
  webview_set_title(w, "Webview Bind & Eval Example");
  webview_set_size(w, 480, 320, WEBVIEW_HINT_NONE);

  // Pass 'w' as the user data (void*) to the callback
  webview_bind(w, "say_hello", say_hello, w);

  webview_set_html(w,
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

  webview_eval(w, "console.log('Hello from C via eval!');");
  printf("1\n");

  // Create a thread to call call_eval_fn while main thread runs webview_run
  HANDLE hThread;
  hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)call_eval_fn, w, 0, NULL);
  if (hThread == NULL) {
    fprintf(stderr, "Failed to create thread\n");
    return 1;
  }
  webview_run(w);

  printf("2\n");
  webview_destroy(w);
  return 0;
}

