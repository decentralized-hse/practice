var fs = require('fs');
var path = require('path');

if (process.argv.length < 4) {
    throw new Error("Provide a file name and block number...");
}

const fileR = path.join(__dirname, `${process.argv[2]}.hashtree`);
const block = parseInt(process.argv[3]);
var node = block * 2;

console.log(`reading ${process.argv[2]}.hashtree...`);
const hashtree = fs.readFileSync(fileR, 'utf8').split(' ');
var len = hashtree.length;

var res = []
for (let i = 1; true; i++) {
	const pow = (1 << i);
	let par = (node + (node ^ pow)) / 2;
	if (par + pow - 1 >= len)
		break;

	res.push(hashtree[node ^ pow]);
	node = par;
}

console.log(`putting the proof into ${process.argv[2]}.proof...`)
const fileW = path.join(__dirname, `${process.argv[2]}.proof`);
fs.writeFileSync(fileW, res.join('\n'))

console.log("all done!");