import React, { useEffect, useState } from 'react';
import * as web_socket from "../modules/web_socket.js";

const ClientApproval = ({remote_clients, set_remote_clients }) => {

    const set_client_approve_status = (client_public_id, new_approval_state) => {
        // find client
        const cur_client = remote_clients[client_public_id];
        // Guard
        if (!cur_client) {
            return;
        }
        // Guard
        if (cur_client.approved == new_approval_state) {
            return;
        }
        // change state (object update)
        set_remote_clients(prev => ({
            ...prev,
            [client_public_id]: {
                ...prev[client_public_id],
                approved: new_approval_state
            }
        }));
        // send req to web socket 
        /// TODO: [should be done before changing state]
        web_socket.change_client_approval_state(client_public_id, cur_client.public_name, new_approval_state)
    }

    return (
        <div style={{ margin: "5% 10%" }}>
            <h2>Approve Incomming Clients</h2>
            <div style={{ border: "dotted black 2px", width: "40%", borderRadius: "10px", padding: "10px", margin: "20px" }}> {/* upload file section */}
                {/* Approve Clients Here */}
                <h3>DisApproved Clients</h3>
                {Object.entries(remote_clients).map(([client_id, client], idx) => {
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
                {Object.entries(remote_clients).map(([client_id, client], idx) => {
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