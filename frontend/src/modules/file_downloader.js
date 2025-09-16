import * as web_socket from "./web_socket.js"

// ============= GLOBAL VARIABLES ================= //
let cur_download_speed = 0;  // tracks current download speed
let available_download_resource = {parallel_chunks:2, chunk_size: 1024*1024*1}; // 2, 1MB

// total download status. can be found from just download_state, but quick access here.
let nr_chunks_being_used = 0;
let nr_files_downloading = 0; // unused for now // step 10
let total_files_to_download = 0;  // unused for now // step 10

let download_state = {  };
/*
Format of a download state for a file is
{
    public_id:{
        file_id1:{
            total_size,
            bytes_downloaded
            next_chunk_download_position = 0, // increment when start a new chunk
            status: DownloadStatus: paused, downloading, downloaded, error
            files:[
                {start_pos, size, data: blob, status: chunk_download_status},
                {start_pos, size, data: blob, status: chunk_download_status}
            ]
        }, 
        file_id2:{...}
    }
}
*/

// round cur_speed to upper MB. Then compare with map
const download_speed_vs_resource_map = {
    0.5: {parallel_chunks:2, chunk_size: 1024*1024*0.5},
    1: {parallel_chunks:3, chunk_size: 1024*1024*2},
    3: {parallel_chunks:4, chunk_size: 1024*1024*4},
    5: {parallel_chunks:5, chunk_size: 1024*1024*5},
    10: {parallel_chunks:7, chunk_size: 1024*1024*7},
    25: {parallel_chunks:10, chunk_size: 1024*1024*10},
    50: {parallel_chunks:15, chunk_size: 1024*1024*15},
    100: {parallel_chunks:20, chunk_size: 1024*1024*20},
};

const DownloadStatus = Object.freeze({
    PAUSED: 0,
    DOWNLOADING: 1,
    DOWNLOADED: 2,
    ERROR: 3,
    // WAITING: // Should I Use Waiting for like resources ? or like max n files download at a time.
});


// ========== Register callback functions for WebSocket events === //
// ========== Function naming format: register...(Callback) ============= //
let onDownloadSpeedUpdate;
export function registerOnDownloadSpeedUpdate(callback) {
    onDownloadSpeedUpdate = callback;
}



// ================== File DOWNLOAD FUNCTIONALITY =========== //


export function start_download_file(public_id, file){
    // Guard:  callback fn exists
    if (!onDownloadSpeedUpdate){
        console.log("Client Logic Error: Callback Function Not Loaded Yet");
        return;
    }

    // check if file exists
    if (download_state[public_id] && download_state[public_id][file.id]){
        console.log("File ALready Downloading");
        return;
    }

    if (!download_state[public_id]){
        download_state[public_id] = {};
    }

    download_state[public_id][file.id] = {
        total_size: file.size,
        bytes_downloaded: 0,
        next_chunk_download_position: 0,
        status: DownloadStatus.DOWNLOADING,
        files: [],
    }

    for(let i = 0 ; i < available_download_resource.parallel_chunks; i++ ){
        request_next_chunk(public_id, file.id);
    }
}

function request_next_chunk(public_id, file_id){
    if (request_chunk_guard(public_id, file_id) == false) return;

    // set new chunk variables
    const downloading_file = download_state[public_id][file_id];
    const start_pos = downloading_file.next_chunk_download_position;
    const size = Math.min(available_download_resource.chunk_size, downloading_file.total_size - start_pos + 1);
    if (downloading_file.total_size < start_pos + size){
        downloading_file.next_chunk_download_position = -1;
    }else{
        downloading_file.next_chunk_download_position += size;
    }

    // push data to download state
    downloading_file.files.push(
        {start_pos, size, data: null, status: DownloadStatus.DOWNLOADING},
    )

    // req websocket to request message
    web_socket.request_chunk(public_id, file_id, start_pos, size);

}

function request_chunk_guard(public_id, file_id){
    // Check If we have resources to download a new chunk
    if (!download_state[public_id] || !download_state[public_id][file_id]){
        console.error("CHunk Download: File Not INItalized Properly");
        return false;
    }

    if (download_state[public_id][file_id].status != DownloadStatus.DOWNLOADING){
        console.log("CHunk req not sent. Bcz file status not downloading");
        return false;
    }

    if (nr_chunks_being_used > available_download_resource.parallel_chunks){
        console.log("Request Chunk: Not Requestes. Max Chunks Reached");
        return false;
    }
    const downloading_file = download_state[public_id][file_id];
    if (downloading_file.next_chunk_download_position == -1 || downloading_file.next_chunk_download_position > downloading_file.size){
        console.log("Remove this Later: Chunk Not Downloaded bcz outside size");
        return false;
    }
    return true;
}

