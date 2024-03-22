package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	student "fuzz_capnp/capnp-trenin"
	"io"
	"log"
	"os"
	"path"

	"github.com/spf13/afero"

	"capnproto.org/go/capnp/v3"
)

type Project struct {
	Repo [59]byte
	Mark uint8
}

type Student struct {
	Name     [32]uint8
	Login    [16]uint8
	Group    [8]uint8
	Practice [8]uint8
	Project  struct {
		Repo [59]uint8
		Mark uint8
	}
	Mark float32
}

func listToArray(list capnp.UInt8List) [8]byte {
	var res [8]byte
	var len int
	len = 8
	if list.Len() < 8 {
		len = list.Len()
	}

	for i := 0; i < len; i++ {
		res[i] = list.At(i)
	}
	return res
}

func byteToString(b []byte) string {
	var zeroSuff = len(b)
	for zeroSuff > 0 && b[zeroSuff-1] == 0 {
		zeroSuff--
	}

	if zeroSuff == 0 {
		return ""
	}
	return string(b[:zeroSuff])
}

func MarshalCapnproto(studentsSlice []Student) ([]byte, error) {
	msg, seg, err := capnp.NewMessage(capnp.SingleSegment(nil))
	if err != nil {
		return nil, err
	}

	capnpStudents, err := student.NewRootStudents(seg)
	if err != nil {
		return nil, err
	}

	list, err := capnpStudents.NewList(int32(len(studentsSlice)))
	if err != nil {
		return nil, err
	}

	for i, s := range studentsSlice {
		capnpStudent := list.At(i)
		if err != nil {
			return nil, err
		}

		capnpStudent.SetName(byteToString(s.Name[:]))
		capnpStudent.SetLogin(byteToString(s.Login[:]))
		capnpStudent.SetGroup(byteToString(s.Group[:]))
		practiceList, err := capnpStudent.NewPractice(int32(len(s.Practice)))
		if err != nil {
			return nil, err
		}
		for i, p := range s.Practice {
			practiceList.Set(i, p)
		}

		project, err := capnpStudent.NewProject()
		if err != nil {
			return nil, err
		}
		project.SetRepo(byteToString(s.Project.Repo[:]))
		project.SetMark(s.Project.Mark)

		capnpStudent.SetMark(s.Mark)
	}

	return msg.Marshal()
}

func readBinary(fs afero.Fs, filename string) ([]Student, error) {
	file, err := fs.Open(filename)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	var students []Student
	for {
		var st Student
		err := binary.Read(file, binary.LittleEndian, &st)
		if errors.Is(err, io.EOF) {
			break
		}

		if err != nil {
			return nil, err
		}

		if err := validateStudent(st); err != nil {
			return nil, err
		}

		students = append(students, st)
	}

	return students, nil
}

func writeBinary(fs afero.Fs, students []Student, filename string) error {
	file, err := fs.Create(filename)
	if err != nil {
		return err
	}
	defer file.Close()

	for _, st := range students {
		err := binary.Write(file, binary.LittleEndian, &st)
		if err != nil {
			return err
		}
	}

	return nil
}

func readCapnproto(fs afero.Fs, filename string) ([]Student, error) {
	file, err := fs.Open(filename)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	data, err := afero.ReadAll(file)
	if err != nil {
		return nil, err
	}

	msg, err := capnp.Unmarshal(data)
	if err != nil {
		return nil, err
	}

	capnpStudents, err := student.ReadRootStudents(msg)
	if err != nil {
		return nil, err
	}

	list, err := capnpStudents.List()
	if err != nil {
		return nil, err
	}

	var students []Student
	fmt.Println("aaaaaaaaaaa", list.Len())
	for i := 0; i < list.Len(); i++ {
		cs := list.At(i)
		var student Student

		proj, err := cs.Project()
		if err != nil {
			return nil, err
		}
		prac, err := cs.Practice()
		if err != nil {
			return nil, err
		}
		name, err := cs.Name()
		if err != nil {
			return nil, err
		}
		login, err := cs.Login()
		if err != nil {
			return nil, err
		}
		group, err := cs.Group()
		if err != nil {
			return nil, err
		}
		repo, err := proj.Repo()
		if err != nil {
			return nil, err
		}
		student = Student{
			Practice: listToArray(prac),
			Project: Project{
				Mark: proj.Mark(),
			},
			Mark: cs.Mark(),
		}

		copy(student.Name[:], name[:])
		copy(student.Login[:], login[:])
		copy(student.Group[:], group[:])
		copy(student.Project.Repo[:], repo[:])

		students = append(students, student)
	}

	return students, nil
}

func writeCapnproto(fs afero.Fs, students []Student, filename string) error {
	file, err := fs.Create(filename)
	if err != nil {
		return fmt.Errorf("failed to open file '%s': %w", filename, err)
	}
	defer file.Close()
	fmt.Printf("Students count %v", len(students))

	b, err := MarshalCapnproto(students)
	if err != nil {
		return err
	}
	n, err := file.Write(b)
	if err != nil || n != len(b) {
		return err
	}

	return nil
}

func main() {
	filename := os.Args[len(os.Args)-1]

	givenExt := path.Ext(filename)
	var resultExt string
	var read func(fs afero.Fs, filename string) ([]Student, error)
	var write func(gs afero.Fs, students []Student, filename string) error
	fs := afero.NewOsFs()
	switch givenExt {
	case ".bin":
		resultExt = ".capnproto"
		read, write = readBinary, writeCapnproto

	case ".capnproto":
		resultExt = ".bin"
		read, write = readCapnproto, writeBinary

	default:
		log.Fatalf("Invalid extension '%s'", givenExt)
	}

	givenFilename := filename
	resultFilename := filename[:len(filename)-len(givenExt)] + resultExt

	students, err := read(fs, givenFilename)
	if err != nil {
		log.Fatalf("Failed to read data: %s", err)
	}
	fmt.Printf("Writing into %s...\n", resultFilename)
	err = write(fs, students, resultFilename)
	if err != nil {
		log.Fatalf("Failed to write data: %s", err)
	}

}
