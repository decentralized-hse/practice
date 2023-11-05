#!/usr/bin/env node

const fs = require('fs');

function isValidChar(c) {
    return !(c <= ' ' || c === '\t' || c === ':')
}

function readTree(fileHash) {
    return fs.readFileSync(fileHash).toString();
}

function getObjectIndex(lines, targetName) {
    for (let i = 0; i < lines.length; i++) {
        let name = lines[i].split('\t')[0]
        name = name.slice(0, name.length - 1)
        if (name === targetName) {
            return i
        }
    }
    return -1
}

function readObjectFromTreeByIndex(pathObjectsNames, index, tree) {
    let dataSplit = tree[index].split('\t')
    let name = dataSplit[0]
    let hash = dataSplit[1]
    switch (name[name.length - 1]) {
        case '/':
            if (pathObjectsNames.length > 0) {
                let newHash = readObjectFromTree(pathObjectsNames, hash)
                tree[index] = newHash
            }
            break
        case ':':
            if (pathObjectsNames.length > 0) {
                throw new Error('Такого файла нет')
            }
            break
    }
    if (pathObjectsNames.length === 0) {
        return fs.readFileSync(hash).toString();
    }
}

function readObjectFromTree(pathObjectsNames, parentHash) {
    let treeLines = readTree(parentHash).split('\n')
    let objectTreeIndex = getObjectIndex(treeLines, pathObjectsNames[0])

    if (objectTreeIndex === -1) {
        throw new Error('Файла нет в дереве')
    }

    return readObjectFromTreeByIndex(
        pathObjectsNames.slice(1, pathObjectsNames.length),
        objectTreeIndex, treeLines
    )
}

const main = () => {
    if (process.argv.length !== 4) {
        throw new Error('Должно быть ровно 3 аргумента')
    }

    const [, , path, hash] = process.argv;

    if (hash.length !== 64) {
        throw new Error('Невалидный SHA-256 хэш')
    }


    if (path.split('').some(c => !isValidChar(c))) {
        throw new Error('Недопустимые символы')
    }

    const pathObjectsNames = path.split('/')

    if (pathObjectsNames.some(name => name === '')) {
        throw new Error('Имя какого-то из объектов пути является пустым')
    }

    console.log(readObjectFromTree(pathObjectsNames, hash));
}

main()