const fs = require('fs');
const { exec } = require('child_process');
const crypto = require('crypto');
const { v4: uuidv4 } = require('uuid');


function randomCharString(length, charSet) {
    let randomString = '';
    const characters = charSet || 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    const charactersLength = characters.length;
    for (let i = 0; i < length; i++) {
        randomString += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return randomString;
}

function randomUtf8String(length) {
    return randomCharString(length).padEnd(length, '\0');
}

function randomAsciiString(length) {
    return randomCharString(length, 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789').padEnd(length, '\0');
}

function randomUrl(length) {
    return 'http://'.concat(randomCharString(length - 7, 'abcdefghijklmnopqrstuvwxyz')).padEnd(length, '\0');
}

function generateRandomStudentData() {
    const name = Buffer.from(randomUtf8String(32), 'utf8');
    const login = Buffer.from(randomAsciiString(16), 'utf8');
    const group = Buffer.from(randomAsciiString(8), 'utf8');
    const practice = crypto.randomBytes(8).map(x => x % 2); // Генерация 8 случайных битовых значений (0 или 1)
    const projectRepo = Buffer.from(randomUrl(59), 'utf8');
    const projectMark =  Buffer.allocUnsafe(1); // Генерация одного случайного байта для оценки
    projectMark.writeUInt8(Math.floor(Math.random() * 100), 0); // Записываем случайное число от 0 до 100 в буфер
    const mark = Buffer.allocUnsafe(4);
    mark.writeFloatLE(Math.random() * 100, 0); // Записываем случайную оценку в буфер как 32-битное число с плавающей точкой

    const data = Buffer.concat([name, login, group, Buffer.from(practice), projectRepo, projectMark, mark]);

    return data;
}

function generateMultipleStudentsData(count) {
    let studentsData = [];
    for (let i = 0; i < count; i++) {
        studentsData.push(generateRandomStudentData());
    }
    return Buffer.concat(studentsData);
}

const COUNT = 5;
const studentsData = generateMultipleStudentsData(COUNT);
console.log(studentsData);
console.log("Generated data length:", studentsData.length);

function runCommand(command, callback) {
    exec(command, (error, stdout, stderr) => {
        if (error) {
            console.error(`exec error: ${error}`);
            return;
        }
        // console.log(`stdout: ${stdout}`);
        // console.error(`stderr: ${stderr}`);
        callback();
    });
}

function testConversion(callback) {
    const uniqueId = uuidv4();
    const originalFilename = `temp_original_${uniqueId}.bin`;
    let fuken = originalFilename.split('.');
    const convertedFilename = `${fuken[0]}.flat`;

    const originalData = generateMultipleStudentsData(1);
    fs.writeFileSync(originalFilename, originalData);

    runCommand(`node solution.js ${originalFilename}`, () => {
        runCommand(`node solution.js ${convertedFilename}`, () => {
            const convertedData = fs.readFileSync(originalFilename);
            const result = originalData.compare(convertedData) == 0 ? "Тест пройден: Данные совпадают" : "Ошибка теста: Данные не совпадают";
            const completed = originalData.compare(convertedData) == 0 ? true : false;
            // console.log(result);
            fs.unlinkSync(originalFilename);
            fs.unlinkSync(convertedFilename);
            callback(completed);
        });
    });

}

function runTests(testCount) {
    let completedTests = 0;
    const testCompletedCallback = (completed) => {
        completedTests++;
        if (!completed) {
            console.log(`Выполнено тестов: ${completedTests} из ${testCount}`);
            console.log("Тест не пройден");
            return;
        }
        console.log(`Выполнено тестов: ${completedTests}`);
        if (completedTests === testCount) {
            console.log("Все тесты завершены");
        }
    };

    for (let i = 0; i < testCount; i++) {
        testConversion(testCompletedCallback);
    }
}

const TEST_COUNT = 100;
runTests(TEST_COUNT);