// Usage node peaks-panesh.js <filename>

const fs = require('fs');

const layersCount = 32;
const nullHash = '0'.repeat(64);

class Node {
    constructor(layer, offset) {
        this.value = (offset << (layer + 1)) | ((1 << layer) - 1);
        this.offset = offset;
    }

    getNextOffset(isCorrectNode) {
        if (isCorrectNode) {
            // Getting node without parent
            return (this.offset + 1) * 2;
        }
        // This node doesn't have a hash, so we just moving offset for left node of next level
        return this.offset * 2;
    }
}

function getPeaks(hashes) {
    const peaks = new Array(layersCount).fill(nullHash);
    let offset = 0;
    for (let layer = layersCount - 1; layer >= 0; layer--) {
        const node = new Node(layer, offset);
        if (node.value < hashes.length && hashes[node.value] !== nullHash) {
            peaks[layer] = hashes[node.value];
            offset = node.getNextOffset(true);
        } else {
            offset = node.getNextOffset(false);
        }
    }
    return peaks;
}

function main() {
    if (process.argv.length < 3) {
        console.error('Usage: node peaks-panesh.js <filename>');
        return;
    }
    const filename = process.argv[2];
    const hashes = fs.readFileSync(`${filename}.hashtree`).toString().split('\n').filter(x => x);
    const peaks = getPeaks(hashes);
    fs.writeFileSync(`${filename}.peaks`, peaks.join('\n') + '\n');
}

main();
