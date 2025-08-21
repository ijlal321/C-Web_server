const CHUNK_SIZE = 8 * 1024 * 1024; // 8 MB
const PARALLEL_UPLOADS = 4; // number of simultaneous uploads

// Split file into chunks
function* chunkFile(file) {  // Generator Function (Pause And resume.)
  let offset = 0, index = 0;
  while (offset < file.size) {
    const end = Math.min(offset + CHUNK_SIZE, file.size);
    yield { index, blob: file.slice(offset, end) };  // 
    offset = end; index++;
  }
}

// Upload one file in parallel chunks
async function uploadFile(file, onProgress = () => {}) {
  // Step 1: Init session
  let res = await fetch("/upload/init", { method: "POST" });
  let { uploadId } = await res.json();

  const chunks = [...chunkFile(file)];  // Get Array of chunks from Generator.
  let uploadedBytes = 0;   // total bytes uploaded
  const uploaded = new Array(chunks.length).fill(0);   // bytes uploaded in each chunk [0, 1024, 2000, ...]

  async function uploadChunk({ index, blob }) {
    return new Promise((resolve, reject) => {
      const xhr = new XMLHttpRequest();
      xhr.open("PUT", `/upload/${uploadId}/${index}`);

      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          uploadedBytes += (e.loaded - uploaded[index]); // delta
          uploaded[index] = e.loaded;
          onProgress(uploadedBytes, file.size);
        }
      };

      xhr.onload = () => (xhr.status >= 200 && xhr.status < 300) 
        ? resolve() : reject(new Error("Chunk upload failed"));
      xhr.onerror = () => reject(new Error("XHR error"));
      xhr.send(blob);
    });
  }

  // Step 2: Worker pool for parallel uploads
  let i = 0;  // This is a shared index (like a pointer) into the chunks array. Each worker will "take the next chunk" by incrementing i.
  async function worker() {
    while (i < chunks.length) {
      const current = chunks[i++]; // as JS is 1 thread, so No RACE CONDITION issue here.
      await uploadChunk(current);  // move to next chunk only when this is done.
    }
  }

  // launch PARALLEL_UPLOADS nr of concurrent ops (Genius Approach. CLeaner, better IMO)
  await Promise.all(Array.from({ length: PARALLEL_UPLOADS }, worker));

  // Step 3: Mark upload complete
  await fetch(`/upload/${uploadId}/complete`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ name: file.name, size: file.size })
  });

  console.log("✅ Upload complete");
}




//  USAGE =============================================
/*
<input type="file" id="file" />
<script>
document.getElementById("file").onchange = (e) => {
  const file = e.target.files[0];
  uploadFile(file, (done, total) => {
    console.log(`Progress: ${(done/total*100).toFixed(2)}%`);
  });
};
</script>
*/
