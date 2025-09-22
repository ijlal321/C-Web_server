import { useState, useEffect } from 'react'
import * as utils from "./modules/utils.js";
import * as web_socket from "./modules/web_socket.js";
import './App.css'
import TransferDashboard from './pages/TransferDashboard.jsx';
import ClientApproval from './pages/ClientApproval.jsx';

function App() {

  const [self_client, set_self_client] = useState({public_id:-1, public_name:"name here", approved: false, files:{}});
  const [remote_clients, set_remote_clients] = useState({});
  const [private_id, set_private_id] = useState(); /// used for reconnecting. not implemented yet.

  useEffect(()=>{
    web_socket.register_all_clients(set_self_client, set_remote_clients, self_client, remote_clients);

    // init websocket
    const app_mode = import.meta.env.VITE_APP_MODE; 
    if (app_mode == "master_app"){
        web_socket.init();
      }else{
        const generated_private_id = utils.generateRandomId();
        set_private_id(generated_private_id);
        web_socket.init(generated_private_id);
      }
  }, []);

  useEffect(() => {
    
    web_socket.register_client_getter(self_client, remote_clients);
    
    // rest of your useEffect code...
  }, [remote_clients, self_client]);


  // Connecting State
  if (self_client.public_id == -1){
    return <h2>Connecting...</h2>
  }

  // Waiting for APporval State
  if (!self_client.approved && self_client.public_id != 0){
    return <h2>Waiting for Approval</h2>
  }

  // Ready State
  return (
    <>
    <h1>SHARE WITH ME APP DEMO UI</h1>
      <div>
        {self_client.public_id == 0 && 
          <ClientApproval remote_clients={remote_clients} set_remote_clients={set_remote_clients}  />
        }
        <TransferDashboard self_client={self_client} set_self_client={set_self_client} remote_clients={remote_clients} set_remote_clients={set_remote_clients} />
      </div>
    </>
  )
}

export default App
