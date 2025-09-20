

// ================= HANDLING OPCODES ============= //
/*
    HOW TO READ OPCODES
1. FIRST PART WILL TELL WHO IS SENDING
2. 2ND PART WILL TELL WHAT HE WANTS
*/
export const WsOPCodes = createEnum([
  'CLIENT_REGISTER',  // client asking server to register
  'CLIENT_REGISTER_ACK', // PUBLIC_ID    // tell client if registered or not + public id sent
  'NEW_CLIENT_REGISTERED',  // tell master app a new client registered

  'MASTER_APP_REGISTER', // master app asking to register
  'MASTER_APP_REGISTER_ACK',    // send ack to master app telling if auth successful

  'APPROVE_CLIENT', // MASTER APP tell server to approve this client
  'DIS_APPROVE_CLIENT',  // .. disapprove this client
  'CLIENT_APPROVED',    // server tell everyone this client is approved
  'CLIENT_DIS_APPROVED', //  ... dissapproved

  'ADD_FILES',   // Client: HEY ADD THIS [LIST] OF FILES I HAVE TO YOUR DB
  'REMOVE_FILE',    // client telling server to remove this file (singular)
  'FILES_ADDED',    // server broadcast new file list
  'FILE_REMOVED',   // server broadcast removed file (singlular)

  'REQUEST_CHUNK',       // SERVER WILL FIND THAT CHUNK AND LOAD INTO MEMORY
  'REQUEST_CHUNK_UPLOAD', // SERVER ASKING CHUNK OWNER TO UPLOAD THAT CHUNK TO SERVER
  'CHUNK_READY',  // SERVER NOTIFYING PPL WHO WANT THAT CHUNK THAT IT IS READY

  'UPLOAD_FILE',         // ask client to upload this file
  'UPDATE_NAME',         // ask other party to UPDATE SENDER NAME
  'REGISTER',            // send a unique uuid, and ask server to register it
  'RECONNECT',           // send previous UUID and try to reconnect
  'ASK_FILES'            // GIMME ALL FILES YOU GOT
]);

function createEnum(keys) {
  const enumObj = {};
  keys.forEach((key, index) => {
    enumObj[key] = index;
  });
  return Object.freeze(enumObj);
}

    // USAGE
// console.log(WsOPCodes.CLIENT_REGISTER); // 0
// console.log(WsOPCodes.ASK_FILES);       // 19

// ============ OTHER UTLITITY FNS =============== //


// Generate random private_id - for session authentication.
export function generateRandomId() {
    let arr = new Uint8Array(16);
    window.crypto.getRandomValues(arr);
    return Array.from(arr, b => b.toString(16).padStart(2, '0')).join('');
}

export function generate_small_id(){
  return Math.random().toString(36).slice(2, 10);
}

export function get_key_from_file(file){
  return `${file.name}_${file.size}`;
}


export function get_unique_files(old_files, new_files){
    // remove duplicate files
    const existing = new Set(Object.values(old_files).map(get_key_from_file));
    const uniqueFiles = Array.from(new_files).filter(f => !existing.has(get_key_from_file(f)));

    return uniqueFiles;
}

