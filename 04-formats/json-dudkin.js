const fs = require('node:fs');
const process = require('process');
const { Buffer } = require('buffer');

// struct Student {
//     // имя может быть и короче 32 байт, тогда в хвосте 000
//     // имя - валидный UTF-8
//     char    name[32];
//     // ASCII [\w]+
//     char    login[16];
//     char    group[8];
//     // 0/1, фактически bool
//     uint8_t practice[8];
//     struct {
//         // URL
//         char    repo[59];
//         uint8_t mark;
//     } project;
//     // 32 bit IEEE 754 float
//     float   mark;
// };

const C_STRUCT_SIZE = 128

const fields = {
    'name': {offset: 0, length: 32},
    'login': {offset: 32, length: 16},
    'group': {offset: 48, length: 8},
    'practice': {offset: 56, length: 8},
    'project_repo': {offset: 64, length: 59},
    'project_mark': {offset: 123, length: 1},
    'mark': {offset: 124, length: 4},
}

function getBinaryField(buf, fieldName) {
    return buf.slice(fields[fieldName].offset, fields[fieldName].offset + fields[fieldName].length);
}

function trimNullBytes(str) {
    return str.replace(/\\x00+$/, '');
}

function getStringField(buf, fieldName) {
    return trimNullBytes(getBinaryField(buf, fieldName).toString());
}

function writeStringField(buf, fieldName, value) {
    const fieldProperties = fields[fieldName];
    if (value.length > fieldProperties.length) {
        console.warn(`${fieldName} with value ${value} will be truncated`)
    }
    buf.write(value, fieldProperties.offset, fieldProperties.length)
}

function convertBinToJSON(inputFile, outputFile) {
    var inputStream = fs.createReadStream(inputFile);
    var outputStream = fs.createWriteStream(outputFile);
    var isFirstChunk = true;
    inputStream.on('readable', () => {
        var chunk;
        while ((chunk = inputStream.read(C_STRUCT_SIZE)) != null) {  // consume data
            if (chunk.length != C_STRUCT_SIZE) {
                throw new Error(`File ${inputFile} is truncated (read ${chunk.length} bytes. expected ${C_STRUCT_SIZE})`);
            }
            var student = {
                'name' : getStringField(chunk, 'name'),
                'login' : getStringField(chunk, 'login'),
                'group' : getStringField(chunk, 'group'),
                'practice' : Array.from(getBinaryField(chunk, 'practice')),
                'project' : {
                    'repo' : getStringField(chunk, 'project_repo'),
                    'mark' : getBinaryField(chunk, 'project_mark').readUint8(),
                },
                'mark' : getBinaryField(chunk, 'mark').readFloatLE(),
            };
            if (isFirstChunk) {
                outputStream.write(
                    "[\n" + JSON.stringify(student)
                );
                isFirstChunk = false;
            } else {
                outputStream.write(
                    ",\n" + JSON.stringify(student)
                );
            }
        }
    }).on('end', function() {
        outputStream.write("\n]\n");
        outputStream.end();
    });
}

function convertJSONToBin(inputFile, outputFile) {
    var students = JSON.parse(fs.readFileSync(inputFile));
    var outputStream = fs.createWriteStream(outputFile);

    for (var student of students) {
        var buf = Buffer.alloc(C_STRUCT_SIZE);  // filled with null bytes
        writeStringField(buf, 'name', student.name);
        writeStringField(buf, 'login', student.login);
        writeStringField(buf, 'group', student.group);
        if (student.practice.length > fields['practice'].length) {
            console.warn(`practice has length of ${student.practice.length}, will be truncated to ${fields['practice'].length}`);
        }
        Buffer.from(student.practice).copy(buf, fields['practice'].offset, 0, fields['practice'].length);
        writeStringField(buf, 'project_repo', student.project.repo);
        buf.writeUInt8(student.project.mark, fields['project_mark'].offset);
        buf.writeFloatLE(student.mark, fields['mark'].offset);

        outputStream.write(buf);
    }
    outputStream.end();
}

function main() {
    var inputFile = process.argv[2]

    if (inputFile.endsWith('.bin')) {
        convertBinToJSON(inputFile, inputFile.replace(/.bin$/, '.json'));
    } else if (inputFile.endsWith('.json')) {
        convertJSONToBin(inputFile, inputFile.replace(/.json$/, '.bin'));
    } else {
        throw new Error(`Not a valid input file: ${inputFile} (expected a .bin or .json file)`);
    }
}

main();
