
// =============== Imports  =============//
import { WsOPCodes } from "./utils";

// ============= Global Vars - For Easier Access from other Fns  == //
let ws_our_public_id = -1; // in deployment, better to make a shared state for easier access. this works too.

// ========== Register callback functions for WebSocket events === //
// ========== Function naming format: register...(Callback) ============= //
let onPublicIdUpdate;
export function registerOnPublicIdUpdate(callback) {
    onPublicIdUpdate = callback;
}

let onApprovalStateChange;
export function registerOnApprovalStateChange(callback) {
    onApprovalStateChange = callback;
}

let onOurFilesUpdate;
export function registerOnOurFilesUpdate(callback) {
    onOurFilesUpdate = callback;
}

let onAvailableFilesUpdate;
export function registerOnAvailableFilesUpdate(callback) {
    onAvailableFilesUpdate = callback;
}




// ======================= WS Functionality / Event Registration ===================== //
let ws;
export function init(private_id){
    const ws_url = import.meta.env.VITE_WEB_SOCKET_URL;
    ws = new WebSocket(ws_url);
    // #  should we use window.location.hostname ?
    // http://example.com:3000/some/path?query=123
    // window.location.hostname → "example.com"



    ws.onopen = function() {
        ws.send(JSON.stringify({ opcode: WsOPCodes.CLIENT_REGISTER, data: { private_id } }));
    };

    ws.onclose = function(){
        console.log("websocket connection closed");
    }

    ws.onerror = function(event){
        console.error("Some error Happened in websocket: ", event);
    }

    ws.onmessage = function(event) {
        try {
            const msg = JSON.parse(event.data);
            if (msg.opcode && msg.data){
                handle_message(msg);
            }
        } catch (error) {
            console.log("Websocket Incomming Message Cannot be parsed: ", error);
        }
    }
    ws = ws;
}

// ================== HANDLING WS Out Going MESSAGES ======================== //
export function send_files_to_server(files){
    if (is_ws_ready_to_send_msg() == false) return false;
    if (!files || files.length === 0) return false;
    

    // Send new files to websocket
    ws.send(JSON.stringify({
        opcode: WsOPCodes.CLIENT_ADD_FILES,
        data: {
            public_id: ws_our_public_id,
            file_count: files.length,
            files,
        }
    }));
    return true;
}

export function remove_file_from_server(file_id){
    if (is_ws_ready_to_send_msg() == false) return false;

    // Remove from UI
    ws.send(JSON.stringify({
        opcode: WsOPCodes.CLIENT_REMOVE_FILE,
        data: {
            public_id: ws_our_public_id,
            file_id: file_id
        }
    }));
    return true;
}

// ================== HANDLING WS Incomming MESSAGES ======================== //

function handle_message(msg){
    const {opcode, data} = msg;
    console.log("WS Message received,  opcode: ", opcode, "  and data: ", data);

    switch (opcode){
        case WsOPCodes.PUBLIC_ID:
            ws_set_public_id(data);
            break;
        case WsOPCodes.SERVER_APPROVE_CLIENT:
            ws_set_approved_state(true);
            break;  
        case WsOPCodes.UI_ADD_FILES:
            ws_add_available_files(data);
            break;
        case WsOPCodes.SERVER_UPLOAD_CHUNK:
            break;
        case WsOPCodes.SERVER_CHUNK_READY:
            break;
        case WsOPCodes.UI_REMOVE_FILE:
            ws_remove_available_file(data);
            break;
        default:
            console.log("Unknown Opcode Send: ", opcode);
    }

}


function ws_set_public_id(data){
    if (!data.public_id){
        console.error("Server Should Have Send Some Public ID.");
        return;
    }
    if (onPublicIdUpdate == null){
        console.error("Client Logic Error: publicIdSetterCallback called before initialized");
        return;
    }
    ws_our_public_id = data.public_id;
    onPublicIdUpdate(data.public_id);
}

function ws_set_approved_state(new_state){
    if (onApprovalStateChange == null){
        console.error("Client Logic Error: approvedStateSetterCallback called before initialized");
        return;
    }
    onApprovalStateChange(new_state);
}

function ws_add_available_files(data){
    if (onAvailableFilesUpdate == null){
        console.error("Client Logic Error: onAvailableFilesUpdate called before initialized");
        return;
    }
    if (!data || typeof data.public_id == 'undefined' || !Array.isArray(data.files)) {
        console.log("Add Available FIles: Data from server Invalid or INcomplete");
        return;
    }
    onAvailableFilesUpdate(prev => {
        const updated = { ...prev };
        if (!updated[data.public_id]) {
            updated[data.public_id] = [];
        }
        updated[data.public_id] = updated[data.public_id].concat(data.files);
        return updated;
    });
}

function ws_remove_available_file(data){
    if (onAvailableFilesUpdate == null){
        console.error("Client Logic Error: onAvailableFilesUpdate called before initialized");
        return;
    }
    if (!data || typeof data.public_id == 'undefined' || typeof data.file_id == 'undefined') {
        console.log("Add Available FIles: Data from server Invalid or INcomplete");
        return;
    }
    onAvailableFilesUpdate(prev => {
        const updated = { ...prev };
        if (!updated[data.public_id]) {
            updated[data.public_id] = [];
        }
        // Remove the file with file_id from the array
        updated[data.public_id] = updated[data.public_id].filter(f => f.id !== data.file_id);
        return updated;
    });
}


// =====================  WS Helper FN ============ //

function is_ws_ready_to_send_msg(){
    if (!ws || ws.readyState != WebSocket.OPEN){
        console.log("Cannot Send Files. Websocket State is Not Open yet.");
        return false;
    }
    if (ws_our_public_id == -1){
        console.log("Cannot Send Files. Websocket State Not connected yet.");
        return false;
    }
    return true;
}