var ed25519 = require('ed25519');
var fs = require('fs');
var path = require('path');
var crypto = require('crypto')

function buf2hex(buffer) {
  const byteArray = new Uint8Array(buffer);

  const hexParts = [];
  for (let i = 0; i < byteArray.length; i++) {
    const hex = byteArray[i].toString(16);

    const paddedHex = ('00' + hex).slice(-2);

    hexParts.push(paddedHex);
  }

  return hexParts.join('');
}


function main() {
  if (process.argv.length < 3) {
    throw new Error('Not enough arguments');
  }

  const filePath = path.join(__dirname, `${process.argv[2]}.root`);
  const secPath = path.join(__dirname, `${process.argv[2]}.sec`);
  const pubPath = path.join(__dirname, `${process.argv[2]}.pub`);
  const signaturePath = path.join(__dirname, `${process.argv[2]}.sign`);

  const file = fs.readFileSync(filePath);
  const keyPair = ed25519.MakeKeypair(crypto.randomBytes(32));
  fs.writeFileSync(secPath, buf2hex(keyPair.privateKey));
  fs.writeFileSync(pubPath, buf2hex(keyPair.publicKey));

  signature = ed25519.Sign(file, keyPair);

  fs.writeFileSync(signaturePath, buf2hex(signature));
}

main();
