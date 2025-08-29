#include <stdio.h>

#include "config.h"
#include "ui.h"
#include "app_context.h"


/*
    Call C from JS
    webview_bind(w, "approve_client", approve_client, app_ctx);

    void approve_client(const char *id, const char *arg, void * user_data){
        cJSON *root = cJSON_Parse(arg);
        const cJSON * public_id_obj = cJSON_GetObjectItem(root, "public_id");

        if (!cJSON_IsNumber(public_id_obj)){
        }

        int public_id = public_id_obj->valueint;
                
        // 0 means resolve. Not 0 means reject.
        webview_return(w, id, 0, "true");  //  window.myFn().then(success => { if (success) {
        webview_return(w, id, 0, "false");  // } else {
        webview_return(w, id, 1, "\"Something went wrong\"");  // .catch(err => {

    }

*/
void approve_client(const char *id, const char *arg, void * user_data);


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


int ui_set_html(webview_t w) {
    FILE *file = fopen(UI_HTML_PATH, "rb");
    if (!file) {
        printf("Cannot open HTML file: %s\n", UI_HTML_PATH);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    char *html = (char *)malloc(filesize + 1);
    if (!html) {
        printf("Memory allocation failed for HTML content\n");
        fclose(file);
        return 1;
    }

    size_t read_size = fread(html, 1, filesize, file);
    html[read_size] = '\0';
    fclose(file);

    webview_error_t error = webview_set_html(w, html);
    free(html);

    if (error != WEBVIEW_ERROR_OK) {
        printf("Cannot place WEB data in UI\n");
        return 1;
    }
    return 0;
}

webview_t ui_init(){
    // Initialize the webview Window
    webview_t w = ui_webview_window_init();
    if (w == NULL) {
        fprintf(stderr, "❌ Failed to create display window\n");
        return NULL;
    }

    // Bind C functions to JavaScript
    // if (ui_bind_all_functions(ctx) != 0) {
    //     fprintf(stderr, "❌ Failed to bind JavaScript functions\n");
    //     webview_destroy(w);
    //     return NULL;
    // }

    // Load HTML content
    if (ui_set_html(w) != 0) {
        fprintf(stderr, "❌ Failed to set HTML content\n");
        webview_destroy(w);
        return NULL;
    }
    return w;
}


// int ui_bind_all_functions(struct AppContext * app_ctx){
//     webview_t w = app_ctx->w;
//     //   webview_bind(w, "say_hello", say_hello, w);
//     webview_bind(w, "approve_client", approve_client, app_ctx);
//     return 0;
// }


// // =========== UI BINDER FUNCTIONS ==========

// void approve_client(const char *id, const char *arg, void * user_data){
//     struct AppContext * app_ctx = (struct AppContext *) user_data;
//     struct ConnectionManager * con_mgr = &app_ctx->connection_mgr; 
//     webview_t w = app_ctx->w;

//     int is_error = 0;

//     cJSON *root = cJSON_Parse(arg);
//     const cJSON * public_id_obj = cJSON_GetObjectItem(root, "public_id");

//     if (!cJSON_IsNumber(public_id_obj)){
//         printf("Cannot Approve Connection. Public Id not available\n");
//         is_error = 1;
//         goto finish;
//     }

//     int public_id = public_id_obj->valueint;
//     pthread_rwlock_rdlock(&con_mgr->rwlock);

//     // TODO: Do we need to ?
//     // Handle when threadA removed it, while thread b was waiting to access it.

//     // find desired client
//     struct Client * cur_client = NULL;
//     HASH_FIND_INT(con_mgr->clients, &public_id, cur_client);
//     if (cur_client == NULL){
//         printf("Cannot find selected Client due to unknown public ID\n");
//         is_error = 1;
//         goto finish;
//     }
    
//     pthread_rwlock_wrlock(&cur_client->rwlock);
//     if (cur_client->approved == 0){
//         cur_client->approved = 1;
//         // 0 means resolve. Not 0 means reject.
//         webview_return(w, id, 0, "true");  //  window.myFn().then(success => { if (success) {
//         // webview_return(w, id, 0, "false");  // } else {
//         // webview_return(w, id, 1, "\"Something went wrong\"");  // .catch(err => {
//         }else{
//             printf("Client ALready Approved");
//             is_error = 1;
//         }

// finish:

//     pthread_rwlock_wrlock(&cur_client->rwlock);
    
//     pthread_rwlock_unlock(&con_mgr->rwlock);
    
//     if (is_error == 0){
//         webview_return(w, id, 0, "true");  
//     }else{
//         webview_return(w, id, 0, "false");  
//     }
// }

// void ui_add_client(webview_t w, struct Client * client){
//     // TODO: handling files completely remains
//     char buffer[500];
//     snprintf(buffer, sizeof(buffer), 
//         "window.AddClient({\"public_id\":%d, \"private_id\":\"%s\", \"approved\":%d, \"public_name\":\"%s\"})", 
//         client->public_id, client->private_id, client->approved, client->public_name);
//     webview_eval(w, buffer);
// }

