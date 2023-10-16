#!/usr/bin/env node

const fs = require('fs');
const {createHash} = require('node:crypto');

if (process.argv.length !== 4) {
    throw new Error('Должно быть ровно 3 аргумента')
}

const [, , path, hash] = process.argv;

if (hash.length !== 64) {
    throw new Error('Невалидный SHA-256 хэш')
}

function isValidChar(c) {
    return !(c <= ' ' || c === '\t' || c === ':')
}

if (path.split('').some(c => !isValidChar(c))) {
    throw new Error('Недопустимые символы')
}

const pathObjectsNames = path.split('/')

if (pathObjectsNames.some(name => name === '')) {
    throw new Error('Имя какого-то из объектов пути является пустым')
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

function createTree(tree) {
    let newData = tree.join('\n')
    let newHash = createHash('sha256').update(newData).digest('hex')
    fs.writeFileSync(newHash, newData);
    return newHash
}

function createBlob(name, newHash) {
    return name + '\t' + newHash
}

function removeObjectFromTreeByIndex(pathObjectsNames, index, tree) {
    let dataSplit = tree[index].split('\t')
    let name = dataSplit[0]
    let hash = dataSplit[1]
    switch (name[name.length - 1]) {
        case '/':
            if (pathObjectsNames.length > 0) {
                let newHash = removeObjectFromTree(pathObjectsNames, hash)
                tree[index] = createBlob(name, newHash)
            }
            break
        case ':':
            if (pathObjectsNames.length > 0) {
                throw new Error('Такого файла нет')
            }
            break
    }
    if (pathObjectsNames.length === 0) {
        tree = tree.slice(0, index)
            .concat(tree.slice(index + 1, tree.length))
    }
    return tree
}

function removeObjectFromTree(pathObjectsNames, parentHash) {
    let treeLines = readTree(parentHash).split('\n')
    let objectTreeIndex = getObjectIndex(treeLines, pathObjectsNames[0])

    if (objectTreeIndex === -1) {
        throw new Error('Файла нет в дереве')
    }

    return createTree(removeObjectFromTreeByIndex(
        pathObjectsNames.slice(1, pathObjectsNames.length),
        objectTreeIndex, treeLines
    ))
}

removeObjectFromTree(pathObjectsNames, hash)
