

// ================= HANDLING OPCODES ============= //
/*
    HOW TO READ OPCODES
1. FIRST PART WILL TELL WHO IS SENDING
2. 2ND PART WILL TELL WHAT HE WANTS
*/
export const WsOPCodes = createEnum([
  'CLIENT_REGISTER',
  'UI_REGISTER',
  'PUBLIC_ID',         // Sending your public ID
  'ADD_CLIENT',
  'UI_APPROVE_CLIENT',
  'SERVER_APPROVE_CLIENT',
  'UI_DIS_APPROVE_CLIENT',
  'SERVER_DIS_APPROVE_CLIENT',
  'CLIENT_ADD_FILES',    // HEY ADD THIS [LIST] OF FILES I HAVE TO YOUR DB
  'CLIENT_REMOVE_FILE',  // REMOVE THIS SINGLE FILE
  'UI_ADD_FILES',        // 10
  'UI_REMOVE_FILE',

  'REQUEST_CHUNK',       // SERVER WILL FIND THAT CHUNK AND LOAD INTO MEMORY
  'SERVER_UPLOAD_CHUNK', // SERVER ASKING CHUNK OWNER TO UPLOAD THAT CHUNK TO SERVER
  'SERVER_CHUNK_READY',  // SERVER NOTIFYING PPL WHO WANT THAT CHUNK THAT IT IS READY

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

export function get_key_from_file(file){
  return `${file.name}_${file.size}`;
}


export function get_unique_files(old_files, new_files){
    // remove duplicate files
    const existing = new Set(old_files.map(f => get_key_from_file(f)));
    const uniqueFiles = Array.from(new_files).filter(f => !existing.has(get_key_from_file(f)));

    return uniqueFiles;
}

