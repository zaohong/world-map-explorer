const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const [releaseRoot, sourceRoot] = process.argv.slice(2);
if (!releaseRoot || !sourceRoot) throw new Error('Usage: node write_sha256_manifests.js <release-root> <source-root>');
async function collect(root, outputName) {
  const files = [];
  async function walk(directory) {
    for (const entry of await fs.promises.readdir(directory, { withFileTypes: true })) {
      const file = path.join(directory, entry.name);
      if (entry.isDirectory()) await walk(file);
      else if (file !== path.join(root, outputName)) files.push(file);
    }
  }
  await walk(root);
  const records = new Array(files.length);
  let next = 0;
  async function worker() {
    for (;;) {
      const index = next++;
      if (index >= files.length) return;
      const file = files[index], hash = crypto.createHash('sha256');
      for await (const chunk of fs.createReadStream(file)) hash.update(chunk);
      const relative = path.relative(root, file).split(path.sep).join('\\');
      records[index] = `${hash.digest('hex')}  ${relative}`;
    }
  }
  await Promise.all(Array.from({ length: 8 }, worker));
  records.sort((a, b) => a.slice(0, 64).localeCompare(b.slice(0, 64)) || a.localeCompare(b));
  await fs.promises.writeFile(path.join(root, outputName), `${records.join('\r\n')}\r\n`, 'utf8');
  return files.length;
}
async function main() {
  const [releaseCount, sourceCount] = await Promise.all([
    collect(releaseRoot, 'SHA256SUMS.txt'), collect(sourceRoot, 'SHA256.txt')
  ]);
  console.log(`PASS: wrote SHA-256 manifests for ${releaseCount.toLocaleString('en-US')} runtime files and ${sourceCount.toLocaleString('en-US')} source/archive files`);
}
main().catch(error => { console.error(`FAIL: ${error.message}`); process.exitCode = 1; });
