var flatbuffers = require('flatbuffers');
var fs = require('fs');
var ds = require('./student_generated').ds
var Buffer = require('buffer/').Buffer


function parseBin(path) {
	var fileData = Buffer.from(fs.readFileSync(path, "utf8"), "utf8");
	var res = []
	for (var i = 0; i < fileData.length; i += 128) {
		var name = fileData.slice(0, 32).toString();
		var login = fileData.slice(32, 48).toString();
		var group = fileData.slice(48, 56).toString();
		var practice = Array.from(new TextEncoder().encode(fileData.slice(56, 64)));
		var repo = fileData.slice(64, 123).toString();
		var markk = new TextEncoder().encode(fileData.slice(123, 124))[0];
		var mark = new Float32Array(Buffer.from(fileData.slice(124, 128)).buffer)[0];
		res.push({
			'name' : name,
			'login' : login,
			'group' : group,
			'practice' : practice,
			'project' : {
				'repo' : repo,
				'mark' : markk
			},
			'mark' : mark
		});
	}
	return res;
}

function getFlat(binStudent) {
	studs = []
	for (var i = 0; i < N; i++) {
		var stud = binStudent[i]
		var builder = new flatbuffers.Builder(1024);

		var name = builder.createString(stud.name);
		var login = builder.createString(stud.login);
		var group = builder.createString(stud.group);

		var repo = builder.createString(stud.project.repo);
		ds.Proj.startProj(builder);
		ds.Proj.addRepo(builder, repo);
		ds.Proj.addMark(builder, stud.project.mark)
		var project = ds.Proj.endProj(builder);
		
		var practice = ds.Student.createPracticeVector(builder, stud.practice);

		ds.Student.startStudent(builder);
		ds.Student.addName(builder, name);
		ds.Student.addLogin(builder, login);
		ds.Student.addGroup(builder, group);
		ds.Student.addPractice(builder, practice);

		ds.Student.addProject(builder, project);

		ds.Student.addMark(builder, stud.mark);
		var student = ds.Student.endStudent(builder);
		studs.push(student)
	}
	var kings = ds.Students.createKingsVector(builder, studs);
	ds.Students.startStudents(builder);
	ds.Students.addKings(builder, kings);
	var students = ds.Students.endStudents(builder);
	builder.finish(students);
	var buf = builder.asUint8Array();
	return buf;
}

function dumpBin(studs) {
	res = []
	for (var i = 0; i < N; i++) {
		var king = studs.kings(i);
		var name = Buffer.from(king.name());
		var login = Buffer.from(king.login());
		var group = Buffer.from(king.group());
		var practice = [];
		for (var j = 0; j < 8; ++j) {
			practice.push(king.practice(j));
		}
		practice = Buffer.from(practice)
		var repo = Buffer.from(king.project().repo());
		var markk = Buffer.from([king.project().mark()]);
		var mark = Buffer.allocUnsafe(4);
		mark.writeFloatLE(king.mark());
		res.push(Buffer.concat([name, login, group, practice, repo, markk, mark]));
	}
	return Buffer.concat(res);
}


if (process.argv.length < 3) {
	console.log("provide arguments")
	return;
}

if (process.argv[2].split('.').pop() == 'bin') {
	console.log(`Reading binary student data from ${process.argv[2]}...`);
	var binStudents = parseBin(process.argv[2]);
	var N = binStudents.length;
	console.log(`${N} student${N > 1 ? 's' : ''} read...`);
	var fileName = process.argv[2].split('.');
	fileName.pop();
	fileName.push('flat');
	fileName = fileName.join('.');

	buf = getFlat(binStudents);
	fs.writeFileSync(fileName, buf, 'binary');
	console.log(`written to ${fileName}`);
} else if (process.argv[2].split('.').pop() == 'flat') {
	console.log(`Reading flatbuffers student data from ${process.argv[2]}...`);
	var bytes = new Uint8Array(fs.readFileSync(process.argv[2]));
	var buf = new flatbuffers.ByteBuffer(bytes);
	var students = ds.Students.getRootAsStudents(buf);
	var N = students.kingsLength();
	var binStudent = parseBin(process.argv[2]);
	console.log(`${N} student${N > 1 ? 's' : ''} read...`);
	var fileName = process.argv[2].split('.');
	fileName.pop();
	fileName.push('bin');
	fileName = fileName.join('.');
	buf = dumpBin(students);
	fs.writeFileSync(fileName, buf, 'binary');
	console.log(`written to ${fileName}`);
} else {
	console.log("wrong format");
}