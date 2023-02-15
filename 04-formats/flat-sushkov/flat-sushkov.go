package main

import (
	"04-formats/flat-sushkov/schema"
	"bytes"
	"encoding/binary"
	"errors"
	flatbuffers "github.com/google/flatbuffers/go"
	"io"
	"log"
	"os"
	"path"
)

type Project struct {
	Repo [59]byte
	Mark uint8
}

type Student struct {
	Name     [32]byte
	Login    [16]byte
	Group    [8]byte
	Practice [8]uint8
	Project
	Mark float32
}

func bytesToString(s []byte) string {
	i := bytes.IndexByte(s, 0)
	if i != -1 {
		return string(s[:i])
	}
	return string(s)
}

func readFlat(file *os.File) []Student {
	buf, err := io.ReadAll(file)
	if err != nil {
		log.Fatalf("error on read file: %v", err)
	}

	var students []Student

	root := schema.GetRootAsRoot(buf, 0)
	studentsLen := root.StudentsLength()
	for i := 0; i < studentsLen; i++ {
		var studentFlat schema.Student
		if !root.Students(&studentFlat, i) {
			log.Fatalf("don't have %d student", i)
		}
		var student Student
		copy(student.Name[:], studentFlat.Name())
		copy(student.Login[:], studentFlat.Login())
		copy(student.Group[:], studentFlat.Group())

		practiceLen := studentFlat.PracticeLength()
		for j := 0; j < practiceLen; j++ {
			student.Practice[j] = studentFlat.Practice(j)
		}

		var project schema.Project
		if studentFlat.Project(&project) == nil {
			log.Fatalf("don't have %d student's project", i)
		}
		copy(student.Project.Repo[:], project.Repo())
		student.Project.Mark = project.Mark()

		student.Mark = studentFlat.Mark()

		students = append(students, student)
	}
	return students
}

func writeFlat(file *os.File, students []Student) {
	builder := flatbuffers.NewBuilder(2048)
	studentsTables := make([]flatbuffers.UOffsetT, 0, len(students))

	for _, st := range students {
		name := builder.CreateString(bytesToString(st.Name[:]))
		login := builder.CreateString(bytesToString(st.Login[:]))
		group := builder.CreateString(bytesToString(st.Group[:]))
		schema.StudentStartPracticeVector(builder, 8)
		for i := range st.Practice {
			builder.PrependByte(st.Practice[len(st.Practice)-1-i])
		}
		practice := builder.EndVector(8)

		repo := builder.CreateString(bytesToString(st.Project.Repo[:]))
		schema.ProjectStart(builder)
		schema.ProjectAddRepo(builder, repo)
		schema.ProjectAddMark(builder, st.Project.Mark)
		project := schema.ProjectEnd(builder)

		schema.StudentStart(builder)
		schema.StudentAddName(builder, name)
		schema.StudentAddLogin(builder, login)
		schema.StudentAddGroup(builder, group)
		schema.StudentAddPractice(builder, practice)
		schema.StudentAddProject(builder, project)
		schema.StudentAddMark(builder, st.Mark)
		student := schema.StudentEnd(builder)

		studentsTables = append(studentsTables, student)
	}

	schema.RootStartStudentsVector(builder, len(students))
	for i := range studentsTables {
		builder.PrependUOffsetT(studentsTables[len(studentsTables)-1-i])
	}
	studentsVec := builder.EndVector(len(students))

	schema.RootStart(builder)
	schema.RootAddStudents(builder, studentsVec)
	root := schema.RootEnd(builder)

	builder.Finish(root)

	buf := builder.FinishedBytes()

	_, err := file.Write(buf)
	if err != nil {
		log.Fatalf("error on write flat to file: %v", err)
	}
}

func readBin(file *os.File) []Student {
	var students []Student
	for {
		var student Student
		err := binary.Read(file, binary.LittleEndian, &student)
		if errors.Is(err, io.EOF) {
			return students
		}
		if err != nil {
			log.Fatalf("error on read file to struct: %v", err)
		}
		students = append(students, student)
	}
}

func writeBin(file *os.File, students []Student) {
	for _, student := range students {
		err := binary.Write(file, binary.LittleEndian, &student)
		if err != nil {
			log.Fatalf("error on write struct to file: %v", err)
		}
	}
}

func getOutputFile(path string) *os.File {
	file, err := os.OpenFile(path, os.O_CREATE|os.O_TRUNC|os.O_RDWR, 0644)
	if err != nil {
		log.Fatalf("error on open file: %v", err)
	}
	return file
}

func main() {
	input, err := os.Open(os.Args[1])
	if err != nil {
		log.Fatalf("error on open file: %v", err)
	}
	defer input.Close()

	switch path.Ext(os.Args[1]) {
	case ".bin":
		student := readBin(input)
		outputPath := os.Args[1][:len(os.Args[1])-4] + ".flat"
		output := getOutputFile(outputPath)
		defer output.Close()
		writeFlat(output, student)
	case ".flat":
		student := readFlat(input)
		outputPath := os.Args[1][:len(os.Args[1])-5] + ".bin"
		output := getOutputFile(outputPath)
		defer output.Close()
		writeBin(output, student)
	}
}
