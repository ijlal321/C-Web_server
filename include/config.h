#ifndef CONFIG_H
#define CONFIG_H

#define PORT "8080"


#define DOCUMENT_ROOT "./Static/"
#define HTML_CLIENT_FILE "./Static/chat2.html"

// #define MAX_WEB_SOCKET_CLIENTS_DISCOVER 20 // max clients to discover
#define MAX_WEB_SOCKET_CLIENTS 20

#define WEB_SOCKET_PATH "/chat"

#define DEFAULT_CLIENT_MAX_FILES_SIZE 20

// =========== UI Configs ==========
#define UI_WINDOW_TITLE "Share APP"
#define UI_WINDOW_WIDTH 480
#define UI_WINDOW_HEIGHT 320

#define UI_HTML_PATH "./Static/ui.html"

// =====================================

#define PRIVATE_ID_SIZE 33  // 16 byte hex = 32 byte char + '\0;
#define NAME_STRING_SIZE 127

// ========== DATA TRANSFER =============

#define CHUNK_SIZE (1024 * 1024 * 512)  // 1MB    

#endif