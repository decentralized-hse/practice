#!/usr/bin/env node

const fs = require('fs');
const makePath = require('path');
const crypto = require('crypto');

class Args {
    constructor(path, hash) {
        this.path = path;
        this.hash = hash;
    }
}

const validateArgs = (rowArgs) => {
    if (rowArgs.length !== 4) {
        throw new Error('Должно быть 3 аргумента');
    }

    if (rowArgs[3].length !== 64) {
        throw new Error('Третьим аргументом должен передаваться хэш кодировки sha256');
    }

    const path = rowArgs[2].trim();
    const pathArray = path === '' ? [] : path.split('/');
    const hash = rowArgs[3];

    return new Args(pathArray, hash);
}

const findAndPrintFile = async(args) => {
    let hash = args.hash;
    const file = await fs.promises.readFile(makePath.join(...args.path));
    const fileValidHash = crypto.createHash('sha256').update(file).digest('hex');

    if (hash !== fileValidHash) {
        throw new Error(`Целостность файла была нарушена`);
    }

    const lines = file.toString().split('\n');
    lines.forEach((line) => console.log(line));
}

const main = async() => {
    const args = process.argv;
    try {
        const validArgs = validateArgs(args);
        await findAndPrintFile(validArgs);
    } catch (err) {
        console.error(err.message);
        process.exit(1);
    }
}

main();
