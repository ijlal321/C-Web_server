import { useState, useEffect } from 'react'
import * as utils from "./modules/utils.js";
import * as web_socket from "./modules/web_socket.js";
import './App.css'
import TransferDashboard from './pages/TransferDashboard.jsx';
import ClientApproval from './pages/ClientApproval.jsx';

function App() {
  const [public_id, set_public_id] = useState(-1);
  const [is_approved, set_is_approved] = useState(false);  // TODO: Default true just for testing.

  const [our_files, set_our_files] = useState([]);  // files available on this device [uploaded by user]
  const [available_files, set_available_files] = useState({});  // files available on other devices

  const [self_client, set_self_client] = useState({public_id:-1, public_name:"name here", approved: false, files:[]});
  const [remote_clients, set_remote_clients] = useState({});
  const [private_id, set_private_id] = useState();
  useEffect(()=>{
    // setting setter functions
    // web_socket.registerOnPublicIdUpdate(set_public_id);
    // web_socket.registerOnApprovalStateChange(set_is_approved);
    // web_socket.registerOnOurFilesUpdate(set_our_files);
    // web_socket.registerOnAvailableFilesUpdate(set_available_files);

    web_socket.register_all_clients(self_client, set_self_client, remote_clients, set_remote_clients);

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
        {/* <TransferDashboard self_client={self_client} set_self_client={set_self_client} remote_clients={remote_clients} set_remote_clients={set_remote_clients} */}
      </div>
    </>
  )
}

export default App
