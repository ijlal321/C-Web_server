#ifndef WS_HELPER_H
#define WS_HELPER_H

#include "web_socket.h"
#include "cJSON.h"

typedef struct {
    int opcode;
    cJSON *data;
} WsMsgHeader;

/**
 * @return 1 on success. 
*/
int ws_opcode_is_text(int con_opcode){
    // Ignore anything that's not a text frame
    // last 4 bits are used to represent opcode.
    if ((con_opcode & 0x0F) != MG_WEBSOCKET_OPCODE_TEXT) {
        printf("Ignoring non-text frame (opcode %d)\n", con_opcode);
        return 0;
    }
    return 1;
}

cJSON * parse_JSON_to_CJSON_root(char * data){
    cJSON *root = cJSON_Parse(data);
    if (!root) {
        printf("Not In JSON. Ignore.\n");
        return NULL;
    }
    return root;
}

WsMsgHeader get_ws_msg_header(cJSON *root){
    // get our Custom Opcode {from JSON}
    WsMsgHeader header = {0, NULL}; // Default/sentinel return
    const cJSON * opcode = cJSON_GetObjectItem(root, "opcode");
    const cJSON * ws_data = cJSON_GetObjectItem(root, "data");
    if (!cJSON_IsNumber(opcode) || !(cJSON_IsObject(ws_data) || cJSON_IsArray(ws_data)) ) {
        printf("Invalid Structure of Websocket Request.'\n");
        return header;
    }

    header.opcode = opcode->valueint;
    header.data = (cJSON *)ws_data; 

    return header;
}




#endif