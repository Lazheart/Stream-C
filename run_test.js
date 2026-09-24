const fs = require('fs');
const createStreamingModule = require('./web/public/streaming.js');

async function test() {
  const mod = await createStreamingModule();
  const csvText = fs.readFileSync('data/data_clean.csv', 'utf8');

  console.log('CSV loaded into memory:', csvText.length, 'bytes');

  const lengthBytes = mod.lengthBytesUTF8(csvText);
  console.log('lengthBytes:', lengthBytes);
  const ptr = mod._malloc(lengthBytes + 1);
  if (!ptr) throw new Error('Failed to allocate memory for CSV');

  console.log('Memory allocated at:', ptr);

  try {
    mod.stringToUTF8(csvText, ptr, lengthBytes + 1);
    console.log('String copied to WASM memory. Calling wasm_load_csv...');
    const count = mod.ccall('wasm_load_csv', 'number', ['number'], [ptr]);
    console.log('Movies loaded:', count);
  } catch (e) {
    console.error('Error during init:', e);
  } finally {
    mod._free(ptr);
  }
}

test().catch(console.error);
