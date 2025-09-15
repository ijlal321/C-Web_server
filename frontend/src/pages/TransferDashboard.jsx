import React from 'react';
import * as web_socket from "../modules/web_socket.js";
import * as utils from "../modules/utils.js";

const TransferDashboard = ({our_files, set_our_files, available_files, set_available_files}) => {
    let fileIdCounter = 1; // for assigning ID to files.

    const handle_file_upload = (event) => {
        // Safe Check
        if (!event || !event.target || !event.target.files){
            return;
        }

        // Get files from Event
        const files = event.target.files;
        if (!files || files.length === 0) return;

        // remove duplicates (if any)
        const newFiles = utils.get_unique_files(our_files, files);

        // convert new files to list with new ID
        const newFilesArr = newFiles.map(f => ({
            id: fileIdCounter++,
            name: f.name,
            size: f.size,
            type: f.type || '',
            file: f,
        }));

        // send new files (without file blob) to ws
        const new_files_without_blob = newFilesArr.map(({file, ...rest}) => rest)
        const res = web_socket.send_files_to_server(new_files_without_blob);
        if (res == false){
            // data send to ws failed. no updating UI files
            return;
        }

        // add new files to UI
        set_our_files((prev) => {
            return [...prev, ...newFilesArr];
        });

    }

    const handle_file_remove = (file_id, file_index) => {
        if (!file_id == null || file_index == null) return;
        // ask websocket to remove fule
        const res = web_socket.remove_file_from_server(file_id);
        if (!res) return;
        // remove item from list
        set_our_files(prev => prev.filter((_, i) => i !== file_index));
        return;
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
                {our_files.map((f, idx) => (
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
                {Object.entries(available_files).map(([file_public_id, files], idx) => (
                    // Show Available Public ID 
                    <div key={idx} style={{border:"solid black 1px", borderRadius:"5px", padding:"10px", margin:"15px"}}>
                        <b>Public:ID: {file_public_id}</b> <br/>
                        {/* Show Files inside each Public ID */}
                        Files: {files.map((f, idx) => (
                            <div key={idx} style={{margin:"10px"}}>
                                <p><b>{idx+1}:
                                    Name: {f.name} </b><br />
                                    Size: {f.size} 
                                </p>
                            </div>
                        ))}
                    </div>
                ))}
                <button onClick={()=>console.log(available_files)}>click me</button>
            </div>
        </div>
    );
};

export default TransferDashboard;