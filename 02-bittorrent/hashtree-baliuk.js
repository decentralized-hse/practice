// Usage: node hashtree-baliuk.js data.bin
// Generate data: dd if=/dev/random bs=1024 count=13 > data.bin

var fs = require('fs');
var path = require('path');
var crypto = require('crypto')

const CHUNK_SIZE = 1024;
const EMPTY_NODE_HASH = '0'.repeat(64);

function calcHash(data) {
    return crypto.createHash('sha256').update(data).digest('hex');
}

function calcNodeHash(left, right) {
    if (left == EMPTY_NODE_HASH || right == EMPTY_NODE_HASH) {
        return EMPTY_NODE_HASH;
    }

    return calcHash(left + right);
}

// Fetches input filename and returns it.
function fetchFilename() {
    if (process.argv.length < 3) {
        console.log('Wrong number of arguments, you must provide input filename: node hashtree-baliuk.js data.txt');
        process.exit(1);
    }

    return process.argv[2];
}

// Reads data from the input file and splits it into chunks of size CHUNK_SIZE.
function readChunks(filename) {
    try {
        var data = fs.readFileSync(path=filename, options={
            'encoding': 'binary',
        });
    } catch (e) {
        console.log('Failed to read file "%s":\n%s', filename, e);
        process.exit(1);
    }
    
    if (data.length % CHUNK_SIZE != 0) {
        console.log('File size must be divisible by %d without remainder [%d bytes given]', CHUNK_SIZE, data.length);
        process.exit(1);
    }

    let chunks = [];
    for (let i = 0; i < data.length; i += CHUNK_SIZE) {
        chunks.push(data.slice(i, i + CHUNK_SIZE));
    }

    return chunks;
}

// Builds Merkle Hash tree from the read chunks.
function buildHashtree(chunks) {
    let treeSize = chunks.length * 2 - (chunks.length % 2);
    let hashes = Array(treeSize).fill(EMPTY_NODE_HASH);
    for (let i = 0; i < hashes.length; i += 2) {
        hashes[i] = calcHash(chunks[i / 2]);
    }

    // Fill higher layers.
    for (let layerPower = 2; layerPower < hashes.length; layerPower *= 2) {
        let start = layerPower / 2 - 1;
        for (let i = start; i + layerPower < hashes.length; i += layerPower * 2) {
            let left = i;
            let right = i + layerPower;

            hashes[(left + right) / 2] = calcNodeHash(hashes[left], hashes[right]);
        }
    }

    return hashes;
}

// Writes hashtree to the output file.
function writeHashes(hashes, outputFilename) {
    let joined = hashes.join('\n');
    if (joined.length > 0) {
        joined += '\n';
    }

    fs.writeFileSync(outputFilename, joined);
}

function run() {
    // Reading configuration.
    const filename = fetchFilename();

    // Reading content.
    console.log('Reading chunks of size %d from "%s"...', CHUNK_SIZE, filename);

    const chunks = readChunks(filename);

    console.log('Read %d chunks, total size: %d bytes', chunks.length, CHUNK_SIZE * chunks.length);

    // Building hashtree.
    console.log('Start building hashtree...');

    const hashes = buildHashtree(chunks);

    console.log('Hash tree has been built, size: %d nodes', hashes.length);

    // Output.
    const outputFilename = filename + '.hashtree';
    console.log('Start writing result to "%s"...', outputFilename);

    writeHashes(hashes, outputFilename);

    console.log('Result has been written to "%s"!', outputFilename);
}

run();