var ed25519 = require('ed25519');
var fs = require('fs');
var path = require('path');

function main() {
    if (process.argv.length < 3) {
        throw new Error("Not enough arguments");
    }

    const pubKeyPath = path.join(__dirname, `${process.argv[2]}.pub`);
    const messagePath = path.join(__dirname, `${process.argv[2]}.root`);
    const signPath = path.join(__dirname, `${process.argv[2]}.sign`);

    try {
        var hexPubKey = fs.readFileSync(pubKeyPath).toString();
        var pubKey = new Uint8Array(hexPubKey.match(/[\da-f]{2}/gi).map(function (h) {
            return parseInt(h, 16)
        }))
    } catch (e) {
        console.log('Failed to read file "%s":\n%s', pubKeyPath, e);
        process.exit(1);
    }

    try {
        var message = fs.readFileSync(messagePath);
    } catch (e) {
        console.log('Failed to read file "%s":\n%s', pubKeyPath, e);
        process.exit(1);
    }

    try {
        var hexSign = fs.readFileSync(signPath).toString();
        var sign = new Uint8Array(hexSign.match(/[\da-f]{2}/gi).map(function (h) {
            return parseInt(h, 16)
        }))
    } catch (e) {
        console.log('Failed to read file "%s":\n%s', pubKeyPath, e);
        process.exit(1);
    }

    var res = ed25519.Verify(message, sign, pubKey);
    console.log('Result check:', res);
    if (res == false) {
        process.exit(1);
    }
}

main()
