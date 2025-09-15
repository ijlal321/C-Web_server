
// =============== Imports  =============//
import { WsOPCodes } from "./utils";




// ========== Register callback functions for WebSocket events === //
// ========== Function naming format: register...Callback ============= //
let publicIdSetterCallback;
export function registerPublicIdSetterCallback(callback) {
    publicIdSetterCallback = callback;
}




// ======================= WS Functionality ===================== //
// let ws; // no need for global


export function init(private_id){
    const ws_url = import.meta.env.VITE_WEB_SOCKET_URL;
    const ws = new WebSocket(ws_url);
    // #  should we use window.location.hostname ?
    // http://example.com:3000/some/path?query=123
    // window.location.hostname → "example.com"



    ws.onopen = function() {
        ws.send(JSON.stringify({ opcode: WsOPCodes.CLIENT_REGISTER, data: { private_id } }));
    };

    ws.onclose = function(){
        console.log("websocket connection closed");
    }

    // NOT A FUNCTION ERROR ?
    // ws.onerror() = function(event){
    //     console.error("Some error Happened in websocket: ", event);
    // }

    ws.onmessage = function(event) {
        try {
            const msg = JSON.parse(event.data);
            if (msg.opcode && msg.data){
                handle_message(msg);
            }
        } catch (error) {
            
        }
    }
}


// ================== HANDLING WS MESSAGES ======================== //

function handle_message(msg){
    const {opcode, data} = msg;
    console.log("WS Message received,  opcode: ", opcode, "  and data: ", data);

    switch (opcode){
        case WsOPCodes.PUBLIC_NAME:
            ws_set_public_name(data);
            break;
        case WsOPCodes.UI_ADD_FILES:
            break;
        case WsOPCodes.SERVER_UPLOAD_CHUNK:
            break;
        case WsOPCodes.SERVER_CHUNK_READY:
            break;
        default:
            console.log("Unknown Opcode Send");
    }

}


function ws_set_public_name(data){
    if (!data.public_id){
        console.error("Server Should Have Send Some Public ID.");
        return;
    }
    if (publicIdSetterCallback == null){
        console.error("Client Logic Error: publicIdSetterCallback called before initialized");
        return;
    }

    publicIdSetterCallback(data.public_id);
}