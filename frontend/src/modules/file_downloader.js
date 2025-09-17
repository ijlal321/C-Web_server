import * as web_socket from "./web_socket.js"

// ============= GLOBAL VARIABLES ================= //
let cur_download_speed = 0;  // tracks current download speed
let  bytes_loaded_this_sec = 0; // Used to track how much total bytes loaded this second. [For Tracking Speed]
let available_download_resource = {parallel_chunks:20, chunk_size: 1024*1024*20}; // 2, 1MB

const MAX_CONCURRENT_FILE_DOWNLOADS = 3;
let activeFileDownloads = 0; // unused for now // step 10
let queuedFileDownloads = []; // [(public_id, file_id), (...), ...]

let download_state = {  };
/*
Format of a download state for a file is
{
    public_id:{
        file_id1:{
            total_size,
            bytes_downloaded,
            activeChunkCount,
            next_chunk_download_position = 0, // increment when start a new chunk
            status: DownloadStatus: paused, downloading, downloaded, error
            chunks_by_start_pos:{
                start_pos: {size, data: blob, status: chunk_download_status},
                start_pos: {size, data: blob, status: chunk_download_status}
            }
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
    PENDING: 4,
    // WAITING: // Should I Use Waiting for like resources ? or like max n files download at a time.
});


// ========== Register callback functions for WebSocket events === //
// ========== Function naming format: register...(Callback) ============= //
let onDownloadSpeedUpdate;
export function registerOnDownloadSpeedUpdate(callback) {
    onDownloadSpeedUpdate = callback;
}



// ================== File DOWNLOAD FUNCTIONALITY =========== //


export function start_download_file(owner_public_id, file){
    // Guard:  callback fn exists
    if (!onDownloadSpeedUpdate){
        console.log("Client Logic Error: Callback Function Not Loaded Yet");
        return;
    }

    // Guard: check if file exists
    if (download_state[owner_public_id] && download_state[owner_public_id][file.id]){
        console.log("File ALready Downloading");
        return;
    }
    // Guard
    if (!download_state[owner_public_id]){
        download_state[owner_public_id] = {};
    }

    // Create Download State
    download_state[owner_public_id][file.id] = {
        total_size: file.size,
        bytes_downloaded: 0,
        next_chunk_download_position: 0,
        status: DownloadStatus.PENDING,
        activeChunkCount: 0,
        chunks_by_start_pos: {},
    }
    
    // request resources and send request.
    allocate_chunk_resource_smartly(owner_public_id, file.id);

}

function allocate_chunk_resource_smartly(owner_public_id, file_id){
    // if new file download start
    if (download_state[owner_public_id][file_id].status == DownloadStatus.PENDING){
        if (activeFileDownloads >= MAX_CONCURRENT_FILE_DOWNLOADS){ // if no space available
            console.log("No Space for New File. Adding to Queue");
            queuedFileDownloads.push([owner_public_id,file_id]);   // add to queue
            return;                                               // Now wait for space to be made.
        }else{   // if space is available
            activeFileDownloads += 1;   // 
            download_state[owner_public_id][file_id].status = DownloadStatus.DOWNLOADING;  // mark it downloading
        }
    }

    const downloading_file = download_state[owner_public_id][file_id];  // temp var for easier access
    
    // check if we can give it more chunk
    const max_available_chunks_per_file = Math.ceil(available_download_resource.parallel_chunks / activeFileDownloads); // Rounding above make sure it alteast gets started.
    if (downloading_file.activeChunkCount >= max_available_chunks_per_file){
        // no more chunks can be allocated. Just return.
        return;
    }

    // allocate 1 more chunk
    downloading_file.activeChunkCount += 1;
    // console.log("File Before sending to rquest chunk: ", downloading_file);
    request_next_chunk(owner_public_id, file_id);
    
}


/**
 * Given a file_id, it will auto send req for next chunk for that file.
 * No Checking if chunks resources are available. Thats for fn calling it.
 */
function request_next_chunk(owner_public_id, file_id){
    if (request_chunk_guard(owner_public_id, file_id) == false) return;



    // set new chunk variables
    const downloading_file = download_state[owner_public_id][file_id];
    const start_pos = downloading_file.next_chunk_download_position;

    if (start_pos == -1){
        // meaning all chunks downloaded sequentially, but there was some in between that was not downloaded. find and download that one.
        // Todo
        console.error("TODO: SOme CHunks Not Got Downloaded, all else done. Reached -1 start pos");
        return;
    }

    // find size of chunk
    const size = Math.min(available_download_resource.chunk_size, downloading_file.total_size - start_pos);
    if (downloading_file.total_size < start_pos + size){
        downloading_file.next_chunk_download_position = -1;
    }else{
        downloading_file.next_chunk_download_position += size;
    }

    // push data to download state
    downloading_file.chunks_by_start_pos[start_pos] =
        {size, data: null, status: DownloadStatus.DOWNLOADING};

    // req websocket to request message
    web_socket.request_chunk(owner_public_id, file_id, start_pos, size);

}

function request_chunk_guard(owner_public_id, file_id){
    // Check If we have resources to download a new chunk
    if (!download_state[owner_public_id] || !download_state[owner_public_id][file_id]){
        console.error("CHunk Download: File Not INItalized Properly");
        return false;
    }

    if (download_state[owner_public_id][file_id].status != DownloadStatus.DOWNLOADING){
        console.log("CHunk req not sent. Bcz file status not downloading");
        return false;
    }

    if (download_state[owner_public_id][file_id].next_chunk_download_position > download_state[owner_public_id][file_id].size){
        console.log("Invalid Next Chunk Download Poistion: Chunk Not Downloaded bcz outside size");
        return false;
    }
    return true;
}

export function download_chunk(owner_public_id, file_id, start_pos, size){
    const url = `/download_chunk?owner_public_id=${encodeURIComponent(owner_public_id)}&file_id=${encodeURIComponent(file_id)}&start_pos=${encodeURIComponent(start_pos)}&size=${encodeURIComponent(size)}`;
    const xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.responseType = 'blob';

    // Progress callback (fires repeatedly as data arrives)
    let lastLoaded = 0; // Track previous loaded value. Used to find how much data load this event.
    xhr.onprogress = function (event) {
        if (event.lengthComputable) {
            const delta = event.loaded - lastLoaded;
            bytes_loaded_this_sec += delta;
            lastLoaded = event.loaded;
        }
    };

    xhr.onload = function() {
        if (xhr.status === 200) {
            const blob = xhr.response;
            store_downloaded_chunk_blob(owner_public_id, file_id, start_pos, size, blob);
            // console.log("Chunk downloaded:", { start_pos, size, blob });
        } else {
            console.error(`Error downloading chunk: HTTP status ${xhr.status}`);
            download_state[public_id][file_id].chunks_by_start_pos[start_pos].status = DownloadStatus.ERROR;
        }
    };

    xhr.onerror = function() {
        download_state[public_id][file_id].chunks_by_start_pos[start_pos].status = DownloadStatus.ERROR;
        console.error("Network error while downloading chunk.");
    };

    xhr.send();
}

function store_downloaded_chunk_blob(owner_public_id, file_id, start_pos, size, blob){
    const downloading_file = download_state[owner_public_id][file_id];  // temp var for easier access
    downloading_file.bytes_downloaded += size;
    downloading_file.activeChunkCount -= 1; 
    downloading_file.chunks_by_start_pos[start_pos].data = blob;
    downloading_file.chunks_by_start_pos[start_pos].status = DownloadStatus.DOWNLOADED;

    console.log("Chunk Downloaded. State After Downloading: ", downloading_file);

    if (downloading_file.bytes_downloaded == downloading_file.total_size){
        // full file has been downloaded
        console.log("FULL FILE DOWNLOADED");
        if (queuedFileDownloads.length != 0){
            const next_file_to_download = queuedFileDownloads.shift();
            allocate_chunk_resource_smartly(next_file_to_download[0], next_file_to_download[1]);
        }
        return;
    }
    // send req to download next chunk
    allocate_chunk_resource_smartly(owner_public_id, file_id);
}