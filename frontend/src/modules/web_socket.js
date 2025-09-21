
// =============== Imports  =============//
import { WsOPCodes } from "./utils.js";
import * as file_downloader from "./file_downloader.js";

// ========== Register callback functions for WebSocket events === //
// ========== Function naming format: register...(Callback) ============= //
// self_client
let ws_our_public_id = -1; // a copy, for better keeping track
let set_self_client, set_remote_clients;  // use getter here
let get_all_files;
export function register_all_clients(_set_self_client, _set_remote_clients, _get_all_files){
    set_self_client = _set_self_client;
    set_remote_clients = _set_remote_clients;
    get_all_files = _get_all_files;
}

let onTransferIdReceived;
export function registerOnTransferIdReceived(callback) {
    onTransferIdReceived = callback;
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
            if (typeof msg.opcode !== 'undefined' && msg.data){
                handle_message(msg);
            }
        } catch (error) {
            console.log("Websocket Incomming Message Cannot be parsed: ", error);
        }
    }
}

// ================== HANDLING WS Out Going MESSAGES ======================== //
export function send_files_to_server(files, transfer_id){
    if (is_ws_ready_to_send_msg() == false) return false;
    if (!files || files.length === 0) return false;
    

    if (ws_our_public_id == 0){  // dont send ws message then
        // send files added message to server
        ws.send(JSON.stringify({
            opcode: WsOPCodes.FILES_ADDED,
            data: {
                public_id: 0,
                files,
            },
        }))
        return;
    }

    // Send new files to websocket
    ws.send(JSON.stringify({
        opcode: WsOPCodes.ADD_FILES,
        data: {
            public_id: ws_our_public_id,
            file_count: files.length,
            transfer_id,
            files,
        }
    }));

    // TODO: update self client local var one here ? 
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

export function change_client_approval_state(client_public_id, client_public_name, approval_state){
    if (client_public_id == null || approval_state == null){
        console.error("Cannot APprove/DisApprove Client: Invalid or incomplete information");
        return;
    }
    if (approval_state == 0){ // dissapprove - everyone who receives it will cut ties from it.
        ws.send(JSON.stringify({
            opcode: WsOPCodes.CLIENT_DIS_APPROVED,
            data: {public_id: client_public_id, public_name: client_public_name}
        }))
    }else if (approval_state == 1){ // approve
        // send all client data to that client. Like a fresh baby
        const all_files = get_all_files();
        console.log("all files got are: ", all_files);
        ws.send(JSON.stringify({
            opcode: WsOPCodes.CLIENT_APPROVED,
            data: {public_id: client_public_id, public_name: client_public_name, all_files}
        }))
    }

}

// ================== HANDLING WS Incomming MESSAGES ======================== //

function handle_message(msg){
    const {opcode, data} = msg;
    console.log("WS Message received,  opcode: ", opcode, "  and data: ", data);

    switch (opcode){
        case WsOPCodes.SERVER_NOT_READY: // try again after x second
            handle_server_not_ready(data);
            break;
        case WsOPCodes.MASTER_APP_REGISTER_ACK:  // master app registered
            handle_master_app_registered_ack(data);
            break;
        case WsOPCodes.CLIENT_REGISTER:  // tell master_app that a client wants to register - 
            if (!only_master_app()) return;
            ws_register_client(data);
            break;
        case WsOPCodes.CLIENT_REGISTER_ACK:
            if (!only_clients()) return;
            ws_set_public_id(data);
            break;
        // case WsOPCodes.NEW_CLIENT_REGISTERED:
        //     ws_add_client(data);
        //     break;
        case WsOPCodes.CLIENT_APPROVED:  // no need to reach master app
            if (!only_clients()) return;
            ws_set_approved_state(data, true);
            break;  
        case WsOPCodes.CLIENT_DIS_APPROVED: // no need to reach master app
            if (!only_clients()) return;
            ws_set_approved_state(data, false);
            break;  
        case WsOPCodes.ADD_FILES:
            // only Master App
            ws_master_app_approve_new_files(data);
            break;
        case WsOPCodes.REMOVE_FILE:
            // only Master App
            ws_master_app_remove_file(data);
            break;
        case WsOPCodes.FILES_ADDED:
            ws_new_files_add(data);
            break;
        case WsOPCodes.FILE_REMOVED:  
            ws_remove_client_file(data);   // Trick right here. need proper opcodes here.
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

function ws_register_client(data){
    if (data == null || data.public_id == null || data.public_name == null || data.approved == null )
    {
        console.error("M_APPCannot register client. incomplete data");
        return;
    }
    data.files = {}; // short cut, there must be proper client - master app communications and things set of what would be send
    // save client
    add_or_update_client(data);
    // const all_files = get_all_files();

    // notify client that its registered with its public id
    ws.send(JSON.stringify({
        opcode: WsOPCodes.CLIENT_REGISTER_ACK,
        data: {
            public_id: data.public_id,
        },
    }));
    // ws.send(JSON.stringify({
    //     opcode: NEW_CLIENT_REGISTERED,
    //     data: {
    //         public_id: data.public_id,
    //         all_files
    //     },
    // }));
}

function handle_server_not_ready(data){
    if (!data){
        console.error("Payload not sent back properly");
        return;
    }
    console.log("sending data back after 3 sec");
    setTimeout(() => {
        ws.send(JSON.stringify(data));
    }, 3000);
}

function handle_master_app_registered_ack(data){
    if (!set_self_client){
        console.error("Client Logic Error: set_self_client called before initialized");
        return;
    }
    if (data.public_id != null && data.public_id == 0){
        set_self_client(prev=> ({...prev, public_id: data.public_id})) // mark as Master_APP
        ws_our_public_id = data.public_id;  // marking temp var
    }else{
        console.error("Server Does Not Accept Us as Master App");
    }
}

function ws_set_public_id(data){
    if (!data.public_id){
        console.error("Server Should Have Send Some Public ID.");
        return;
    }
    if (set_self_client == null){
        console.error("Client Logic Error: publicIdSetterCallback called before initialized");
        return;
    }
    if (ws_our_public_id != -1){
        console.error("Set Public ID: Hmm, it already has a valid public id. WHy send him ?");
        return;     
    }
    console.log("public id updated: ", data.public_id);
    set_self_client((prev)=> ({...prev, public_id: data.public_id}))
    ws_our_public_id = data.public_id;
}

function ws_set_approved_state(data, new_state){
    if (data ==null || data.public_id == null || data.all_files == null){
        console.error("Change Approve State: Data from server Invalid or INcomplete");
        return;
    }
    if (set_self_client == null || set_remote_clients == null){
        console.error("Client Logic Error: self_client or set_remote_clients() called before initialized");
        return;
    }
    // if our state is being changed
    if (data.public_id == ws_our_public_id){ 
        set_self_client(prev => ({...prev, approved: new_state}));
        if (new_state == true){
            // add latest files from clients
            const all_incoming_files = data.all_files;
            delete all_incoming_files[ws_our_public_id];
            set_remote_clients(all_incoming_files);
            console.log("remote clients set after approval to : ", all_incoming_files);
        }else{
            // TODO: remove one self and clear all ties
        }
        return;
    }else{  // if soneome else if approved or disapproved
        if (new_state == true) { // if being approved, add to remote_clients
            set_remote_clients(prev => ({
                ...prev,
                [data.public_id]: {public_id: data.public_id, public_name: data.public_name, approved: new_state, files: {}}
            }));
        } else {
            set_remote_clients(prev => {
                const updated = { ...prev };
                delete updated[data.public_id];
                return updated;
            });
        }
    }

    // check if the one being approved is us
    if (data.public_id == ws_our_public_id){
        set_self_client((prev=>({...prev, approved: new_state})));
        return;
    }else{
        // add or update new client
        add_or_update_client(data);
        console.log("other client updated");
    }
}

function ws_master_app_approve_new_files(data){
    if (!data || typeof data.public_id == 'undefined' || !Array.isArray(data.files)) {
        console.log("Add Available FIles: Data from server Invalid or INcomplete");
        return;
    }
    set_remote_clients(prev=>{
        const updated = {...prev};
        if (!updated[data.public_id]){
            console.error("Add files for a client not added yet");
            return updated;
        }
        if(updated[data.public_id]){
            console.error("client which is asking to approve add files is not in data list");
            return;
        }
        // Ensure files object exists
        updated[data.public_id].files = { ...updated[data.public_id].files };
        // Add new files
        data.files.forEach(file => {
            updated[data.public_id].files[file.id] = file;
        });
    });

    // send files added message to server
    ws.send(JSON.stringify({
        opcode: WsOPCodes.FILES_ADDED,
        data,
    }))
}

function ws_master_app_remove_file(data){
    if (!data || typeof data.public_id == 'undefined' || typeof data.file_id == 'undefined') {
        console.log("Remove Available Files: Data from server Invalid or INcomplete");
        return;
    }


    set_remote_clients(prev => {
        const updated = {...prev};
        if (!updated[data.public_id]) {
            console.error("Remove files for a client not added yet");
            return updated;
        }
        if (!updated[data.public_id].files) {
            console.error("File remove Erorr: client metadata on sevrer should not be empty. what would i remove ?");
            return updated;
        }
        delete updated[data.public_id].files[data.file_id];
        return updated;
    })

    // notify server about removed files if needed
    ws.send(JSON.stringify({
        opcode: WsOPCodes.FILE_REMOVED,
        data,
    }));

    console.log("notification send for FILES_REMOVED")
}

function ws_new_files_add(data){
    if (ws_our_public_id == 0){
        return;  // master app has nothing to do  // also i dont think he will be called here ever.
    }
    if (set_remote_clients == null){
        console.error("Client Logic Error: set_remote_clients called before initialized");
        return;
    }
    if (!data || typeof data.public_id == 'undefined' || !Array.isArray(data.files)) {
        console.log("Add Available FIles: Data from server Invalid or INcomplete");
        return;
    }
    // if our files got added
    if (data.public_id == ws_our_public_id){
        onTransferIdReceived(data.transfer_id, data.files)
        return;
    }
    // todo: make sure init is soo good, it never comes to this
    // if a remote client added some files
    // if (!get_remote_clients[data.public_id]){
    //     if (data.public_id == 0){   // TODO: do i hvae to make servre here if not alrwady there ?
    //      // remote_clients[0] = {public_id: 0, public_name:"master_app someone", approved: 1, files: {}}; // issue, use proper updated syntax
    //     }else{
    //         console.error("Add files for a client not added yet");
    //         return;
    //     }
    // }

    set_remote_clients(prev => {
        const updated = {...prev};
        data.files.forEach(file => {
            updated[data.public_id].files[file.id] = file; // file dont have blob. We already know
        });
        return updated;
    });
}

function ws_remove_client_file(data){
    if (!data || typeof data.public_id == 'undefined' 
        // || !Array.isArray(data.files)  // need if later we convert it to array
        ){
        console.log("Add Available FIles: Data from server Invalid or INcomplete");
        return;
    }
    if (data.public_id == ws_our_public_id) {
        // Remove files from self_client
        set_self_client(prev => {
            const newFiles = { ...prev.files };
            delete newFiles[data.file_id];
            return {
            ...prev,
            files: newFiles
            };
        });
        return;
    }
    // Remove files from remote_clients
    set_remote_clients(prev => {
        const updated = {...pre};
        delete updated[data.public_id].files[data.file_id];
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

// ============== Data related helper funxtions

function add_or_update_client(client){
    set_remote_clients(prev => {
        const updated = {...prev};
        const existing_client = updated[client.public_id];
        if (!existing_client){
            updated[client.public_id] = {...client, files:{}};
            console.log("new client added successfully");
        }else{
            updated[client.public_id] = {...existing_client, ...client};
        }
        return updated;
    })
}

// function get_all_files(){
//     const all_files = {...get_remote_clients(), [ws_our_public_id]: get_self_client()};
//     return all_files;
// }

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

function only_master_app(){
    if (ws_our_public_id == 0){
        return true;
    }
    console.log("returned false");
    return false;
}   

function only_clients(){
    if (ws_our_public_id != 0){
        return true;
    }
    return false;
}   