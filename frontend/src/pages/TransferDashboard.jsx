import React, {useState, useEffect, useRef} from 'react';
import * as web_socket from "../modules/web_socket.js";
import * as utils from "../modules/utils.js";
import * as file_downloader from "../modules/file_downloader.js"

const TransferDashboard = ({self_client, set_self_client, remote_clients, set_remote_clients}) => {
    const fileIdCounter = useRef(1);  // for assigning ID to files.
    const [download_status, set_download_status] = useState({speed:"NA", parallel_chunks:"", chink_size:"", time_took:0});
    const temp_file_map = {};

    useEffect(()=>{
        file_downloader.registerOnDownloadStatusUpdate(set_download_status);
        file_downloader.register_get_file_blob(get_file_blob);
        web_socket.registerOnTransferIdReceived(onTransferIdReceived);
    });

    const onTransferIdReceived = (transfer_id, files) => {
        if (transfer_id == null){
            console.error("Cannot Add files. Invalid Or Incomplete transfer id ?");
            return;
        }
        const entry = temp_file_map[transfer_id];
        if (!entry) {
            // maybe already taken
            return;
        }

        console.log("temp_file: ", entry);
        console.log("self_client: ", self_client);

        // add files in self_client
        set_self_client(prev => ({
            ...prev,
            files: {
                ...prev.files,
                ...entry.files
            }
        }));

        // delete temp_file_map[transfer_id];
    }

    const handle_file_upload = (event) => {
        // Safe Check
        if (!event || !event.target || !event.target.files){
            return;
        }

        // Get files from Event
        const files = event.target.files;
        if (!files || files.length === 0) return;
        
        // remove duplicates (if any)
        const newFiles = utils.get_unique_files(self_client.files, files);
        if (newFiles.length == 0){
            return;
        }

        // convert new files to list with new ID
        const newFilesArr = newFiles.map(f => ({
            id: fileIdCounter.current++,
            name: f.name,
            size: f.size,
            type: f.type || '',
            file: f,
        }));
        // new files without blobs
        const new_files_without_blob = newFilesArr.map(({file, ...rest}) => rest)

        if (self_client.public_id == 0){
            const updatedFiles = { ...self_client.files };
            newFilesArr.forEach(file => {
                updatedFiles[file.id] = file;
            });
            set_self_client(prev => ({
                ...prev,
                files: updatedFiles
            }));
            const res = web_socket.send_files_to_server(new_files_without_blob, -1);
            return;
        }

        // saves new files blob to temp map
        const transfer_id = utils.generate_small_id();
        temp_file_map[transfer_id] = {time: Date.now(), files: {}}
        newFilesArr.forEach(file=>  temp_file_map[transfer_id].files[file.id] = file);

        // send new files (without file blob) to ws
        const res = web_socket.send_files_to_server(new_files_without_blob, transfer_id);
        if (res == false){
            // remove temp file with transfer id
            delete temp_file_map[transfer_id];
            console.error("error adding new file");
        }
        
    }


    const get_file_blob =(owner_public_id, file_id, start_pos, size) =>{
        const file = self_client.files[file_id];
        if (!file) {
            console.error('File not found for upload');
            return null;
        }
        const end = Math.min(start_pos + size, file.size);
        const blob = file.file.slice(start_pos, end);
        return blob;
    }


    const handle_file_remove = (file_id, file_index) => {
        if (!file_id == null || file_index == null) return;
        // ask websocket to remove fule
        const res = web_socket.remove_file_from_server(file_id);
        if (!res) return;
        // remove item from list
        // set_self_client(prev => {
        //     const newFiles = { ...prev.files };
        //     delete newFiles[file_id];
        //     return {
        //     ...prev,
        //     files: newFiles
        //     };
        // });
        return;
    }

    const handle_file_download = (owner_public_id, file) => {
        const int_owner_public_id = parseInt(owner_public_id);
        if (isNaN(int_owner_public_id)) {
            console.error("FIle Owner Public Id is not a int, which should not have happened");
            return;
        }
        file_downloader.start_download_file(int_owner_public_id, file);
    }


    return (
        <div style={{margin:"5% 10%"}}>
            <h1>Transfer Dashboard</h1>

            <div style={{border:"dotted black 2px", width:"40%", borderRadius:"10px", padding:"10px", margin:"20px"}}> {/* upload file section */}
                <h2>Upload Files</h2>
                <input type='file' multiple={true} onChange={handle_file_upload} />
            </div>

            <div style={{ border: "dotted black 2px", borderRadius: "10px", padding: "10px", margin: "20px" }}> {/* upload file section */}
                <h2>Uploaded Files</h2>
                {/* show our files */}
                {Object.entries(self_client.files).map(([file_id, f], idx) => (
                    <div key={idx}>
                        <p>
                            <b>{idx + 1}: {f.name}</b> <br /> Type: {f.type} <br /> Size: {f.size}  <br />
                            <button onClick={() => handle_file_remove(f.id, idx)}>Remove</button>
                        </p>
                    </div>
                ))}
            </div>
            
            <div style={{ border: "dotted black 2px", borderRadius: "10px", padding: "10px", margin: "20px" }}> {/* upload file section */}
                <h2>Available Files</h2>
                {/* show Public Available files */}
                {Object.entries(remote_clients).map(([remote_client_public_id, remote_client], idx) => (
                    // Show Available Public ID 
                    <div key={idx} style={{border:"solid black 1px", borderRadius:"5px", padding:"10px", margin:"15px"}}>
                        <b>Public:ID: {remote_client_public_id}</b> <br/>
                        {/* Show Files inside each Public ID */}
                        Files: {Object.entries(remote_client.files).map(([file_id, f], idx) => (
                            <div key={idx} style={{margin:"10px"}}>
                                <p><b>{idx+1}:
                                    Name: {f.name} </b><br />
                                    Size: {f.size} 
                                    <button style={{margin:"0 10px"}} onClick={()=>handle_file_download(remote_client_public_id, f)}>Download</button>
                                </p>
                            </div>
                        ))}
                    </div>
                ))}
                <button onClick={()=>console.log(self_client.files)}>Log Our Files</button>
                <button onClick={()=>console.log(remote_clients)}>Log other client Files</button>
            </div>
            
            <div>
                {/*download_status:  speed:"", parallel_chunks:"", chink_size:"", time_took */}
                Download_SPeed: {download_status.speed ? download_status.speed: "0 B/s"} <br/>
                parallel_chunks: {download_status.parallel_chunks ? download_status.parallel_chunks: "N/A"} <br/>
                chink_size: {download_status.chink_size ? download_status.chink_size: "N/A"} <br/>
                File Download Time: {download_status.time_took ? download_status.time_took: "N/A"} <br/>
            </div>
        </div>
    );
};

export default TransferDashboard;