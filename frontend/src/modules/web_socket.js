
// =============== Imports  =============//
import { WsOPCodes } from "./utils";
import * as file_downloader from "./file_downloader.js";
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

// Client Approval / Removal Fns
let onClientAddUpdate;
export function registerOnClientAddUpdate(callback) {
    onClientAddUpdate = callback;
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
        if (private_id == null){
            ws.send(JSON.stringify({ opcode: WsOPCodes.MASTER_APP_REGISTER, data: {} }));
        }else{
            ws.send(JSON.stringify({ opcode: WsOPCodes.CLIENT_REGISTER, data: { private_id } }))
        }
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
}

// ================== HANDLING WS Out Going MESSAGES ======================== //
export function send_files_to_server(files){
    if (is_ws_ready_to_send_msg() == false) return false;
    if (!files || files.length === 0) return false;
    

    // Send new files to websocket
    ws.send(JSON.stringify({
        opcode: WsOPCodes.ADD_FILES,
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
        opcode: WsOPCodes.REMOVE_FILE,
        data: {
            public_id: ws_our_public_id,
            file_id: file_id
        }
    }));
    return true;
}

export function request_chunk(owner_public_id, file_id, start_pos, size) {
    if (is_ws_ready_to_send_msg() == false || ws_our_public_id == -1) {
        console.error("websocket not ready to send messages, fix");
        return;
    }
    ws.send(JSON.stringify({
        opcode: WsOPCodes.REQUEST_CHUNK,
        data: {
            sender_public_id: ws_our_public_id,
            owner_public_id: owner_public_id,
            file_id: file_id,
            start_pos,
            size,
        }
    }));
}

export function change_client_approval_state(client_public_id, approval_state){
    if (client_public_id == null || approval_state == null){
        console.error("Cannot APprove/DisApprove Client: Invalid or incomplete information");
        return;
    }
    if (approval_state == 0){ // dissapprove
        ws.send(JSON.stringify({
            opcode: WsOPCodes.DIS_APPROVE_CLIENT,
            data: {public_id: client_public_id}
        }))
    }else if (approval_state == 1){ // approve
        ws.send(JSON.stringify({
            opcode: WsOPCodes.APPROVE_CLIENT,
            data: {public_id: client_public_id}
        }))
    }

}

// ================== HANDLING WS Incomming MESSAGES ======================== //

function handle_message(msg){
    const {opcode, data} = msg;
    console.log("WS Message received,  opcode: ", opcode, "  and data: ", data);

    switch (opcode){
        case WsOPCodes.MASTER_APP_REGISTER_ACK:
            handle_master_app_registered_ack(data);
            break;
        case WsOPCodes.CLIENT_REGISTER_ACK:
            ws_set_public_id(data);
            break;
        case WsOPCodes.NEW_CLIENT_REGISTERED:
            ws_add_client(data);
            break;
        case WsOPCodes.CLIENT_APPROVED:
            ws_set_approved_state(data, true);
            break;  
        case WsOPCodes.CLIENT_DIS_APPROVED:
            ws_set_approved_state(data, false);
            break;  
        case WsOPCodes.FILES_ADDED:
            ws_add_available_files(data);
            break;
        case WsOPCodes.FILE_REMOVED:
            ws_remove_available_files(data);   // Trick right here. need proper opcodes here.
            break;
        case WsOPCodes.REQUEST_CHUNK_UPLOAD:
            ws_upload_chunk(data)
            break;
        case WsOPCodes.CHUNK_READY:
            ws_download_chunk(data);
            break;
        // case WsOPCodes.UI_REMOVE_FILE:
        //     ws_remove_available_file(data);
        //     break;
        // case WsOPCodes.ADD_CLIENT:  // unused i think
        //     ws_add_client(data);
        //     break;
        default:
            console.warn("Unknown Opcode Received: ", opcode);
    }

}

function handle_master_app_registered_ack(data){
    if (!onPublicIdUpdate){
        console.error("Client Logic Error: publicIdSetterCallback called before initialized");
        return;
    }
    if (data.public_id != null && data.public_id == 0){
        onPublicIdUpdate(data.public_id); // mark as Master_APP
        ws_our_public_id = data.public_id;
    }else{
        console.error("Server Does Not Accept Us as Master App");
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

function ws_set_approved_state(client, new_state){
    if (client ==null || client.public_id == null || client.public_name == null || client.approved == null){
        console.error("Change Approve State: Data from server Invalid or INcomplete");
        return;
    }
    if (onApprovalStateChange == null){
        console.error("Client Logic Error: approvedStateSetterCallback called before initialized");
        return;
    }

    // check if the one being approved is us
    if (client.public_id == ws_our_public_id){
        onApprovalStateChange(new_state);
        return;
    }else{  // else update data of available clients
        onClientAddUpdate({...client, approved: new_state});
    }
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
    if (data.public_id == ws_our_public_id){
        return; // not for us.
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

function ws_remove_available_files(data){
    if (onAvailableFilesUpdate == null){
        console.error("Client Logic Error: onAvailableFilesUpdate called before initialized");
        return;
    }
    if (!data || typeof data.public_id == 'undefined' 
        // || !Array.isArray(data.files)  // need if later we convert it to array
        ){
        console.log("Add Available FIles: Data from server Invalid or INcomplete");
        return;
    }
    if (data.public_id == ws_our_public_id){
        return; // not for us.
    }
    onAvailableFilesUpdate(prev => {
        const updated = { ...prev };
        if (updated[data.public_id]) {
            updated[data.public_id] = updated[data.public_id].filter(file => file.id != data.file_id);
        }
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

function ws_download_chunk(data){
    const requiredKeys = ['owner_public_id', 'file_id', 'start_pos', 'size'];
    if (!data || !requiredKeys.every(key => typeof data[key] !== 'undefined')) {
        console.log("WS Download Chunk Ready: Incomplete or Invalid Fields");
        return;
    }
    file_downloader.download_chunk(
        data.owner_public_id,
        data.file_id,
        data.start_pos,
        data.size
    );
}

function ws_upload_chunk(data){
    console.log("Server is Asking to Download CHunk. Functionality We Have Not Implemented Yet");
    const {owner_public_id, file_id, start_pos, size} = data;
    if (!owner_public_id == null || file_id == null || start_pos == null || size == null){
        console.error("Chunk Upload Error: Invalid Or Inccomplete Data");
        return;
    }

    file_downloader.upload_chunk(owner_public_id, file_id, start_pos, size);
}

function ws_add_client(data){
    if (data.public_id== null || data.public_name == null || data.approved == null){
        console.error("Cannot Add Client: Invalid or incomplete information");
        return;
    }
    onClientAddUpdate(data);

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

function only_server(){
    if (ws_our_public_id == 0){
        return true;
    }
    return false;
}   