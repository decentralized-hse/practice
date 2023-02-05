var ed25519 = require('ed25519');
var fs = require('fs');
var path = require('path');
var crypto = require('crypto')

function main() {
    if (process.argv.length < 3) {
        throw new Error("Not enough arguments");
    }

    const filePath = path.join(__dirname, `${process.argv[2]}.root`);
    const secPath = path.join(__dirname, '.sec');
    const pubPath = path.join(__dirname, '.pub');
    const signaturePath = path.join(__dirname, `${process.argv[2]}.sign`);

    const file = fs.readFileSync(filePath);
    const keyPair = ed25519.MakeKeypair(crypto.randomBytes(32));
    fs.writeFileSync(secPath, keyPair.privateKey);
    fs.writeFileSync(pubPath, keyPair.publicKey);

    signature = ed25519.Sign(file, keyPair);

    fs.writeFileSync(signaturePath, signature);
}

main();
