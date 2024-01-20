const {readFileSync, writeFileSync} = require('fs'),
    {createHash: createHash} = require('node:crypto'),
    pathModule = require('path');


main()

function main(){
    let [, , inputPath, hash] = process.argv;
    if (hash.length !== 64) {
        throw new Error('Not valid root hash');
    }

    const parsedPath = pathModule.parse(inputPath);
    const newDirName = parsedPath.name + parsedPath.ext;

    if (['\t', ':', '/'].some(invalidChar => newDirName.includes(invalidChar))) {
        throw new Error("Directory name shouldn`t contains symbols \\t : /")
    }

    const newRootHash = addNewFolder(hash, ['root'].concat(parsedPath.dir.split('/')).concat(newDirName))
    console.log('New version root hash: ' + newRootHash)
}

function addNewFolder(currentHash, parsedPath) {
    const currentDirName = parsedPath[0];
    const nextDirName = parsedPath[1];

    const items = readFile(currentHash)
    let lines = items.split('\n').filter(x => x.length > 2);

    if (parsedPath.length === 2){
        let newDirHash = createFile(nextDirName, '')
        lines.push(`${nextDirName}/\t${newDirHash}`);
        return createFile(currentDirName, new TextEncoder().encode(lines.join('\n') + '\n'));
    }

    const [nextHash, index] = findBlob(parsedPath[1], parseBlobs(items))
    let newDirHash = addNewFolder(nextHash, parsedPath.slice(1), nextDirName)

    lines[index] = lines[index].slice(0, -64) + newDirHash;
    return createFile(currentDirName, new TextEncoder().encode(lines.join('\n') + '\n'));
}

function findBlob(blobName, array) {
    for (let i = 0; i < array.length; i++) {
        if (array[i].name === blobName) {
            return [array[i].hash, i];
        }
    }

    throw new Error(`Can't find folder ${blobName}`);
}

function createFile(name, bytesData) {
    const hash = createHash('sha256').update(name + bytesData).digest('hex')
    writeFileSync(hash, bytesData);
    return hash
}

function readFile(fileHash) {
    return readFileSync(fileHash).toString();
}

function parseBlobs(inputString) {
    const entries = inputString.split('\n');
    return entries.filter(x => x.length > 1).map(entry => {
        const isDirectory = entry.includes('/\t');
        const parts = entry.split(isDirectory ? '/\t' : ':\t');
        return {name: parts[0], hash: parts[1], isDirectory: isDirectory};
    });
}