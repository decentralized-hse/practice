var fs = require('fs')

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

const STUDENT_SIZE = 128

function cStructsToJson(path) {
    var fileData = Buffer.from(fs.readFileSync(path, 'UTF-8'), 'UTF-8')
    if (fileData.length % STUDENT_SIZE != 0) {
        throw new Error('File size (' + fileData.length + ') is invalid (should be divided by ' + STUDENT_SIZE + ')')
    }
    var nStudents = fileData.length / STUDENT_SIZE
    var students = new Array(nStudents)

    for (var i = 0; i != nStudents; ++i) {
        students[i] = cStructToJson(fileData.slice(i * STUDENT_SIZE, (i + 1) * STUDENT_SIZE))
    }

    return students
}


function cStructToJson(buf) {
    return {
        'name' : buf.slice(0, 32).toString(),
        'login' : buf.slice(32, 48).toString(),
        'group' : buf.slice(48, 56).toString(),
        'practice' : new TextEncoder().encode(buf.slice(56, 64)),
        'project' : {
            'repo' : buf.slice(64, 123).toString(),
            'mark' : Buffer.from(buf.slice(123, 124)).readUint8()
        },
        'mark' : Buffer.from(buf.slice(124, 128)).readFloatLE()
    }
}

function jsonToKV(jsonList, path) {
    var logger = fs.createWriteStream(path, {flags: 'w'})

    for (var i = 0; i != jsonList.length; ++i) {
        logger.write((i == 0 ? '' : '\n') + studentToKV(jsonList[i], i))
    }
}

function studentToKV(studentJson, index) {
    return `[${index}].name: ${studentJson.name}
[${index}].login: ${studentJson.login}
[${index}].group: ${studentJson.group}
[${index}].practice: ${studentJson.practice}
[${index}].project.repo: ${studentJson.project.repo}
[${index}].project.mark: ${studentJson.project.mark}
[${index}].mark: ${studentJson.mark}`
}

function kvToJson(path) {
    var re = /\[(\d+)\]\.(name|login|group|practice|project\.repo|project\.mark|mark): (.*[^\n])/
    var fileData = fs.readFileSync(path, 'UTF-8')
    var students = []
    var curStudent = {}
    var index = 0

    fileData.split('\n').forEach(line => {
        var match = line.match(re)
        var curIndex = match[1]
        if (index == curIndex) {
        } else if (index + 1 == curIndex) {
            students.push(curStudent)
            curStudent = {}
            ++index
        } else {
            throw new Error("Got unexpeted index (" + match[1] + ") in line: " + line)
        }
        curStudent[match[2]] = match[3]
    })
    students.push(curStudent)

    return students
}

function jsonToBin(jsonList, path) {
    var logger = fs.createWriteStream(path, {flags: 'w'})

    for (let student of jsonList) {
        var finalMark = Buffer.alloc(4)
        finalMark.writeFloatLE(student.mark)

        var projectMark = Buffer.alloc(1)
        projectMark.writeUInt8(Number(student['project.mark']))

        logger.write(Buffer.concat([
            Buffer.from(student.name, 0, 32),
            Buffer.from(student.login, 0, 16),
            Buffer.from(student.group, 0, 8),
            Buffer.from(new Uint8Array(student.practice.split(",").map(Number))),
            Buffer.from(student['project.repo'], 0, 59),
            projectMark,
            finalMark,
        ]))
    }
}

var inputPath = process.argv[2]

if (inputPath.includes('bin')) {
    outputPath = inputPath.replace('bin', 'kv')
    jsonToKV(cStructsToJson(inputPath), outputPath)
} else {
    outputPath = inputPath.replace('kv', 'bin')
    jsonToBin(kvToJson(inputPath), outputPath)
}
