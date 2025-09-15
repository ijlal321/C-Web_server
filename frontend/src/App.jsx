import { useState, useEffect } from 'react'
import * as utils from "./modules/utils.js";
import * as web_socket from "./modules/web_socket.js";
import './App.css'

function App() {
  const [public_id, set_public_id] = useState(-1);
  const [is_approved, set_is_approved] = useState(false);

  
  useEffect(()=>{
    // setting setter functions
    web_socket.registerPublicIdSetterCallback(set_public_id);

    // init websocket
    const private_id = utils.generateRandomId();
    web_socket.init(private_id);
  }, []);

  // Connecting State
  if (public_id == -1){
    return <h2>Connecting...</h2>
  }

  // Waiting for APporval State
  if (!is_approved){
    return <h2>Waiting for Approval</h2>
  }

  // Ready State
  return (
    <>
    <h1>SHARE WITH ME APP DEMO UI</h1>
      <div>

       </div>
    </>
  )
}

export default App
