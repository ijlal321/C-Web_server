import * as web_socket from "./web_socket.js"

// ============= GLOBAL VARIABLES ================= //
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
            name,
            start_time,
            end_time,
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
    1: {parallel_chunks:2, chunk_size: 1024*1024*0.5},
    4: {parallel_chunks:3, chunk_size: 1024*1024*2},
    5: {parallel_chunks:5, chunk_size: 1024*1024*5},
    10: {parallel_chunks:7, chunk_size: 1024*1024*7},
    25: {parallel_chunks:10, chunk_size: 1024*1024*10},
    35: {parallel_chunks:12, chunk_size: 1024*1024*13},
    50: {parallel_chunks:15, chunk_size: 1024*1024*15},
    75: {parallel_chunks:17, chunk_size: 1024*1024*18},
    100: {parallel_chunks:20, chunk_size: 1024*1024*20},
    200: {parallel_chunks:40, chunk_size: 1024*1024*40},
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
let onDownloadStatusUpdate;
export function registerOnDownloadStatusUpdate(callback) {
    onDownloadStatusUpdate = callback;
}

let get_file_blob;
export function register_get_file_blob(callback) {
    get_file_blob = callback;
}


// ================== File DOWNLOAD FUNCTIONALITY =========== //


export function start_download_file(owner_public_id, file){
    // Guard:  callback fn exists
    if (!onDownloadStatusUpdate){
        console.log("Client Logic Error: Callback Function Not Loaded Yet");
        return;
    }

    // Guard: check if file exists
    // if (download_state[owner_public_id] && download_state[owner_public_id][file.id]){
    //     console.log("File ALready Downloading");
    //     return;
    // }
    // Guard
    if (!download_state[owner_public_id]){
        download_state[owner_public_id] = {};
    }

    // Create Download State
    download_state[owner_public_id][file.id] = {
        total_size: file.size,
        name: file.name,
        start_time: Date.now(),
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
    while (downloading_file.activeChunkCount < max_available_chunks_per_file){        
        // allocate 1 more chunk
        downloading_file.activeChunkCount += 1;
        // console.log("File Before sending to rquest chunk: ", downloading_file);
        request_next_chunk(owner_public_id, file_id);
    }
    // no more chunks can be allocated. Just return.
    return;
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
        // Check if any piece remains, else done
        // Todo
        // console.error("TODO: SOme CHunks Not Got Downloaded, all else done. Reached -1 start pos");
        return;
    }

    // find size of chunk
    const size = Math.min(available_download_resource.chunk_size, downloading_file.total_size - start_pos);
    if (downloading_file.total_size <= start_pos + size){
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

export function download_chunk(owner_public_id, file_id, start_pos, size){
    const url = `/download_chunk?owner_public_id=${encodeURIComponent(owner_public_id)}&file_id=${encodeURIComponent(file_id)}&start_pos=${encodeURIComponent(start_pos)}&size=${encodeURIComponent(size)}`;
    const xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.responseType = "arraybuffer";

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
            download_state[owner_public_id][file_id].chunks_by_start_pos[start_pos].status = DownloadStatus.ERROR;
        }
    };

    xhr.onerror = function() {
        download_state[owner_public_id][file_id].chunks_by_start_pos[start_pos].status = DownloadStatus.ERROR;
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

    // console.log("Chunk Downloaded. State After Downloading: ", downloading_file);

    downloading_file.end_time = Date.now();
    const time_in_sec = ((downloading_file.end_time - downloading_file.start_time) / 1000).toFixed(2);
    onDownloadStatusUpdate((prev) => ({ ...prev, time_took: time_in_sec }));

    if (downloading_file.bytes_downloaded == downloading_file.total_size){
        // full file has been downloaded
        console.log("FULL FILE DOWNLOADED. File: ", downloading_file);
        console.log("total chunks: ", Object.keys(downloading_file.chunks_by_start_pos).length);
        // joinChunksToBlob(downloading_file); // NO UNNECESSARY DOWNLOADING
        if (queuedFileDownloads.length != 0){
            const next_file_to_download = queuedFileDownloads.shift();
            allocate_chunk_resource_smartly(next_file_to_download[0], next_file_to_download[1]);
        }
        return;
    }
    // send req to download next chunk
    allocate_chunk_resource_smartly(owner_public_id, file_id);
}

function joinChunksToBlob(downloading_file) {
    const chunkMap = downloading_file.chunks_by_start_pos;

    // Get all start positions and sort them numerically
    const sortedStartPositions = Object.keys(chunkMap)
        .map(Number)
        .sort((a, b) => a - b);

    // Collect chunk .data in order
    const blobParts = sortedStartPositions.map(start => {
        return chunkMap[start].data;
    });

    // Combine all chunks into one Blob
    const finalBlob = new Blob(blobParts, { type: 'application/octet-stream' });

    // Trigger browser download
    const url = URL.createObjectURL(finalBlob);
    const a = document.createElement('a');
    a.href = url;
    a.download = downloading_file.name; // You can set a better name if available
    document.body.appendChild(a);
    a.click();  
    setTimeout(() => {
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }, 1000);
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


// ========= FILE UPLOAD FUNCTIONALITY ============ //

export function upload_chunk(owner_public_id, file_id, start_pos, size){        
    if (!get_file_blob){
        console.error("Client Logic Error: get_file_blob not set");
        return;
    }
    const blob = get_file_blob(owner_public_id, file_id, start_pos, size);
    if (!blob){
        return;
    }
    const url = `/upload_chunk?owner_public_id=${encodeURIComponent(owner_public_id)}&file_id=${encodeURIComponent(file_id)}&start_pos=${encodeURIComponent(start_pos)}&size=${encodeURIComponent(size)}`;
    fetch(url, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/octet-stream'
        },
        body: blob
    }).then(resp => {
        if (!resp.ok) {
            console.error('Chunk Upload Error. Response from http failed');
            return;
        };
        console.log('Chunk uploaded Successully');
    }).catch(err => {
        console.error('Chunk upload error: ');
    });
}

// ======= SPEED CALCULATOR (sEND TO uTILS lATER) ============ //

const sortedKeys = Object.keys(download_speed_vs_resource_map)
    .map(Number)
    .sort((a, b) => a - b);  // done once. Need for finding best resources given speed.
    
setInterval(() => {
    if (!onDownloadStatusUpdate) return; 
    // Calculate speed in bytes/sec, show as KB/s or MB/s
    let speed = bytes_loaded_this_sec / 1;
    bytes_loaded_this_sec = 0;
    let speedStr;
    if (speed > 1024 * 1024) {
        speedStr = (speed / (1024 * 1024)).toFixed(2) + ' MB/s';
    } else if (speed > 1024) {
        speedStr = (speed / 1024).toFixed(2) + ' KB/s';
    } else {
        speedStr = speed + ' B/s';
    }
    const cur_download_speed = speedStr;
    if (speed != 0){
        onDownloadStatusUpdate((prev)=> ({...prev, speed: cur_download_speed}));
        const speedMBps = Math.ceil(speed / (1024*1024));
        const new_resource = getResourceForSpeed(speedMBps);
        console.log(new_resource);
        console.log(`Speed: ${speed / (1024 * 1024)}, parallel_chunks: ${new_resource.parallel_chunks}, chunk size: ${new_resource.chunk_size}`);
        // available_download_resource = new_resource;
    }

}, 1000);

function getResourceForSpeed(speedMBps) {
    for (let key of sortedKeys) {
        if (speedMBps <= key) {
            return download_speed_vs_resource_map[key];
        }
    }

    // If speed is higher than all keys, return the max key's config
    const maxKey = sortedKeys[sortedKeys.length - 1];
    return download_speed_vs_resource_map[maxKey];
}

