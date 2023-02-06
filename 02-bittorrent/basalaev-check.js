var ed25519 = require('ed25519');
var fs = require('fs');
var path = require('path');

function main() {
    if (process.argv.length < 3) {
        throw new Error("Not enough arguments");
    }

    const pubKeyPath = path.join(__dirname, `${process.argv[2]}.pub`);
    const messagePath = path.join(__dirname, `${process.argv[2]}.txt`);
    const signPath = path.join(__dirname, `${process.argv[2]}.sign`);

    try {
        var b64PubKey = fs.readFileSync(pubKeyPath);
        var pubKey = Buffer.from(b64PubKey, 'base64');
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
        var b64Sign = fs.readFileSync(signPath);
        var sign = Buffer.from(b64Sign, 'base64');
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
