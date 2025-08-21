// Download.js: Download file in chunks and store in IndexedDB

const CHUNK_SIZE = 1 * 1024 * 1024; // 1 MB (should match server)

// IndexedDB helpers
function openDB(dbName = 'chunked-downloads', storeName = 'chunks') {
  return new Promise((resolve, reject) => {
    const req = indexedDB.open(dbName, 1);
    req.onupgradeneeded = () => {
      req.result.createObjectStore(storeName, { keyPath: 'index' });
    };
    req.onsuccess = () => resolve(req.result);
    req.onerror = () => reject(req.error);
  });
}

function storeChunk(db, storeName, index, data) {
  return new Promise((resolve, reject) => {
    const tx = db.transaction(storeName, 'readwrite');
    tx.objectStore(storeName).put({ index, data });
    tx.oncomplete = resolve;
    tx.onerror = () => reject(tx.error);
  });
}

function getAllChunks(db, storeName) {
  return new Promise((resolve, reject) => {
    const tx = db.transaction(storeName, 'readonly');
    const store = tx.objectStore(storeName);
    const req = store.getAll();
    req.onsuccess = () => resolve(req.result.sort((a, b) => a.index - b.index));
    req.onerror = () => reject(req.error);
  });
}

// Download file in chunks and store in IndexedDB
const PARALLEL_DOWNLOADS = 4;
async function downloadFile(fileId, totalSize, onProgress = () => {}) {
  const db = await openDB();
  const storeName = 'chunks';
  const numChunks = Math.ceil(totalSize / CHUNK_SIZE);
  let downloadedBytes = 0;
  const downloaded = new Array(numChunks).fill(0);

  async function downloadChunk(i) {
    const resp = await fetch(`/download/${fileId}/${i}`);
    if (!resp.ok) throw new Error(`Chunk ${i} failed: ${resp.status}`);
    const data = await resp.arrayBuffer();
    await storeChunk(db, storeName, i, data);
    downloadedBytes += data.byteLength;
    downloaded[i] = data.byteLength;
    onProgress(Math.min(downloadedBytes, totalSize), totalSize);
  }

  let idx = 0;
  async function worker() {
    while (idx < numChunks) {
      const current = idx++;
      await downloadChunk(current);
    }
  }

  await Promise.all(Array.from({ length: PARALLEL_DOWNLOADS }, worker));

  // Reassemble file
  const chunks = await getAllChunks(db, storeName);
  const buffers = chunks.map(c => c.data);
  const blob = new Blob(buffers);
  return blob;
}

// Example usage:
// downloadFile('dummy123', 12345678, (done, total) => {
//   console.log(`Progress: ${(done/total*100).toFixed(2)}%`);
// }).then(blob => {
//   // Save blob as file
//   const a = document.createElement('a');
//   a.href = URL.createObjectURL(blob);
//   a.download = 'myfile.bin';
//   a.click();
// });
