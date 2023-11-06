const fs = require('fs');
const {createHash} = require('node:crypto');
const pathModule = require('path');

let [, , inputPath, hash] = process.argv;

if (hash.length !== 64) {
    throw new Error('sha256 невалиден');
}

const parsedPath = pathModule.parse(inputPath);
const fileName = parsedPath.name + parsedPath.ext;

if (!isFileNameValid(fileName)) {
    throw new Error("Имя файла не должно содержать символы \\t : /")
}
let inputFile = [];

process.stdin.on("readable", () => {
    let chunk;
    while (null !== (chunk = process.stdin.read())) {
        inputFile += chunk;
    }
});

process.stdin.on("end", () => {
    const [dirToChange, lineToUpdateIndex] = dirDown(hash)
    const lastHash = updateDir(dirToChange, lineToUpdateIndex, inputFile)

    console.log(lastHash)
    process.exit()
})

function updateDir(dirToChange, lineToUpdateIndex) {
    let currentHash = addFile(inputFile)

    for (let i = dirToChange.length - 1; i >= 0; i--) {
        let lines = dirToChange[i].split('\n').filter(x => x.length > 2);
        const lineIndexToUpdate = lineToUpdateIndex[i];
        if (lineIndexToUpdate <= lines.length - 1) {
            lines[lineIndexToUpdate] = lines[lineIndexToUpdate].slice(0, -64) + currentHash;
        } else {
            lines.push(`${fileName}:\t${currentHash}`)
        }
        const newDir = lines.join('\n')+'\n';

        const encoded = new TextEncoder().encode(newDir)
        currentHash = addFile(encoded)
    }
    return currentHash;
}

function dirDown(startHash) {
    let currentHash = startHash;
    let dirToChange = [];
    let lineToUpdateIndex = [];

    parsedPath.dir.split(pathModule.sep)
        .filter(e => e)
        .forEach(name => {
        const items = readFile(currentHash)
        const dirOrFileArray = parseFile(items)
        const [hash, index] = findDirOrFile(name, dirOrFileArray, true)

        if (!hash) {
            console.error(`Can't find folder ${name}`)
            process.exit(-1)
        } else {
            lineToUpdateIndex.push(index)
            dirToChange.push(items)
            currentHash = hash
        }
    })

    const lastFolder = readFile(currentHash);
    dirToChange.push(lastFolder)
    const dirOrFileArray = parseFile(lastFolder)

    const [_, index] = findDirOrFile(fileName, dirOrFileArray, false)

    lineToUpdateIndex.push(index)
    return [dirToChange, lineToUpdateIndex]
}

function findDirOrFile(name, array, isDirectory) {
    for (let i = 0; i < array.length; i++) {
        if (array[i].isDirectory === isDirectory && array[i].name === name) {
            return [array[i].hash, i];
        }
    }
    return [null, array.length]
}

function addFile(bytesData) {
    const hash = createHash('sha256').update(bytesData).digest('hex')
    fs.writeFileSync(hash, bytesData);
    return hash
}

function readFile(fileHash) {
    return fs.readFileSync(fileHash).toString();
}

function parseFile(inputString) {
    const entries = inputString.split('\n');
    return entries.filter(x => x.length > 1).map(entry => {
        const isDirectory = entry.includes('/\t');
        const parts = entry.split(isDirectory ? '/\t' : ':\t');
        return {name: parts[0], hash: parts[1], isDirectory: isDirectory};
    });
}

function isFileNameValid(fileName) {
    return !['\t', ':', '/'].some(invalidChar => fileName.includes(invalidChar));
}
