const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const [currentRoot, referenceRoot] = process.argv.slice(2);
if (!currentRoot || !referenceRoot) throw new Error('Usage: node asset_hash_checks.js <current-release> <reference-release>');
const currentAssets = path.join(currentRoot, 'assets');
const referenceAssets = path.join(referenceRoot, 'assets');
async function collect(directory, base, out = []) {
  for (const entry of await fs.promises.readdir(directory, { withFileTypes: true })) {
    const file = path.join(directory, entry.name);
    if (entry.isDirectory()) await collect(file, base, out);
    else out.push(path.relative(base, file));
  }
  return out;
}
async function hashFile(file) {
  const hash = crypto.createHash('sha256');
  for await (const chunk of fs.createReadStream(file)) hash.update(chunk);
  return hash.digest('hex');
}
async function main() {
  const [currentFiles, referenceFiles] = await Promise.all([
    collect(currentAssets, currentAssets), collect(referenceAssets, referenceAssets)
  ]);
  currentFiles.sort(); referenceFiles.sort();
  if (currentFiles.length !== referenceFiles.length || currentFiles.some((name, index) => name !== referenceFiles[index])) {
    throw new Error(`asset file lists differ: current=${currentFiles.length}, reference=${referenceFiles.length}`);
  }
  const workers = Array.from({ length: 8 }, async (_, worker) => {
    for (let index = worker; index < currentFiles.length; index += 8) {
      const name = currentFiles[index];
      const [currentHash, referenceHash] = await Promise.all([
        hashFile(path.join(currentAssets, name)), hashFile(path.join(referenceAssets, name))
      ]);
      if (currentHash !== referenceHash) throw new Error(`asset SHA-256 mismatch: ${name}`);
    }
  });
  await Promise.all(workers);
  const referenceVersion = path.basename(referenceRoot).replace(/^WorldMapExplorer-/, '');
  console.log(`PASS: all ${currentFiles.length.toLocaleString('en-US')} assets match ${referenceVersion} byte for byte; source pixels were not changed`);
}
main().catch(error => { console.error(`FAIL: ${error.message}`); process.exitCode = 1; });
