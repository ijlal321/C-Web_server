import React, { useEffect, useState } from 'react';
import * as web_socket from "../modules/web_socket.js";

const ClientApproval = ({ our_files, set_our_files, available_files, set_available_files }) => {
    const [clients, set_clients] = useState([]);

    useEffect(() => {
        web_socket.registerOnClientAddUpdate(add_update_client);
    }, []);

    const add_update_client = (client) => {
        const existing_client = clients.find(c => c.public_id === client.public_id);
        if (!existing_client) {
            set_clients(prev => [...prev, client]);
            return;
        }
        // update client
        set_clients(prev =>
            prev.map(c =>
                c.public_id === client.public_id ? { ...c, ...client } : c
            )
        );
    }

    const set_client_approve_status = (client_public_id, new_approval_state) => {
        // find client
        const cur_client = clients.find(c => c.public_id == client_public_id);
        // Guard
        if (!cur_client) {
            return;
        }
        // Guard
        if (cur_client.approved == new_approval_state) {
            return;
        }
        // change state
        set_clients(prev =>
            prev.map(c => c.public_id === client_public_id ? { ...c, approved: new_approval_state } : c)
        );
        // send req to web socket 
        /// TODO: [should be done before changing state]
        web_socket.change_client_approval_state(client_public_id, new_approval_state)
    }

    return (
        <div style={{ margin: "5% 10%" }}>
            <h2>Approve Incomming Clients</h2>
            <div style={{ border: "dotted black 2px", width: "40%", borderRadius: "10px", padding: "10px", margin: "20px" }}> {/* upload file section */}
                {/* Approve Clients Here */}
                <h3>DisApproved Clients</h3>
                {clients.map((client, idx) => {
                    if (client.approved == 0)
                        return <div key={idx}>
                            <b>{idx + 1}.
                                Client Name: {client.public_name}</b>  <br />
                            [Client ID: {client.public_id}]
                            <button onClick={() => set_client_approve_status(client.public_id, 1)} style={{ margin: "0 10px" }}>Approve</button>
                        </div>
                })}
            </div>
            <div style={{ border: "dotted black 2px", width: "40%", borderRadius: "10px", padding: "10px", margin: "20px" }}> {/* upload file section */}
                {/* Approved Clients Here */}
                <h3>Approved Clients</h3>
                {clients.map((client, idx) => {
                    if (client.approved == 1)
                        return <div key={idx}>
                            <b>{idx + 1}.
                                Client Name: {client.public_name}</b>  <br />
                            [Client ID: {client.public_id}]
                            <button onClick={() => set_client_approve_status(client.public_id, 0)} style={{ margin: "0 10px" }}>Dis-Approve</button>
                        </div>
                })}
            </div>
        </div>

    );
};

export default ClientApproval;