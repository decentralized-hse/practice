package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	student "fuzz_capnp/capnp-trenin"
	"io"

	// "log"
	// "path"

	// "log"
	"os"
	// "path"
	"strings"

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

func (s *Student) UnmarshalCapnproto(b []byte) error {
	msg, err := capnp.Unmarshal(b)
	if err != nil {
		return err
	}
	cs, err := student.ReadRootStudent(msg)
	if err != nil {
		return err
	}
	proj, err := cs.Project()
	if err != nil {
		return err
	}
	prac, err := cs.Practice()
	if err != nil {
		return err
	}
	name, err := cs.Name()
	if err != nil {
		return err
	}
	login, err := cs.Login()
	if err != nil {
		return err
	}
	group, err := cs.Group()
	if err != nil {
		return err
	}
	repo, err := proj.Repo()
	if err != nil {
		return err
	}
	*s = Student{
		Practice: listToArray(prac),
		Project: Project{
			Mark: proj.Mark(),
		},
		Mark: cs.Mark(),
	}

	copy(s.Name[:], name[:])
	copy(s.Login[:], login[:])
	copy(s.Group[:], group[:])
	copy(s.Project.Repo[:], repo[:])

	return nil
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

func (s Student) MarshalCapnproto() ([]byte, error) {
	arena := capnp.SingleSegment(nil)
	msg, seg, err := capnp.NewMessage(arena)
	if err != nil {
		return nil, err
	}

	cs, err := student.NewRootStudent(seg)
	if err != nil {
		return nil, err
	}

	cs.SetName(byteToString(s.Name[:]))
	cs.SetLogin(byteToString(s.Login[:]))
	cs.SetGroup(byteToString(s.Group[:]))
	prac, err := capnp.NewData(seg, s.Practice[:])
	if err != nil {
		return nil, err
	}
	cs.SetPractice(prac)
	proj, err := student.NewStudent_Project(seg)
	if err != nil {
		return nil, err
	}
	proj.SetRepo(byteToString(s.Project.Repo[:]))
	proj.SetMark(s.Project.Mark)
	cs.SetProject(proj)
	cs.SetMark(s.Mark)

	b, err := msg.Marshal()
	if err != nil {
		return nil, err
	}

	return b, nil
}

func readBinary(filename string) ([]Student, error) {
	file, err := os.Open(filename)
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

		students = append(students, st)
	}

	return students, nil
}

func writeBinary(students []Student, filename string) error {
	file, err := os.Create(filename)
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

func readCapnproto(filename string) ([]Student, error) {
	data, err := os.ReadFile(filename)
	if err != nil {
		return nil, err
	}

	ss := strings.Split(string(data), ", ")

	var students []Student
	for _, s := range ss {
		var ns Student
		err := ns.UnmarshalCapnproto([]byte(s))
		if err != nil {
			return nil, err
		}
		students = append(students, ns)
	}

	return students, nil
}

func writeCapnproto(students []Student, filename string) error {
	file, err := os.Create(filename)
	if err != nil {
		return fmt.Errorf("failed to open file '%s': %w", filename, err)
	}
	defer file.Close()

	for i, s := range students {
		b, err := s.MarshalCapnproto()
		if err != nil {
			return err
		}
		file.Write(b)
		if i != len(students)-1 {
			file.Write([]byte(", "))
		}
	}

	return nil
}

// func main() {
// 	filename := os.Args[len(os.Args)-1]

// 	givenExt := path.Ext(filename)
// 	var resultExt string
// 	var read func(filename string) ([]Student, error)
// 	var write func(students []Student, filename string) error

// 	switch givenExt {
// 	case ".bin":
// 		resultExt = ".capnproto"
// 		read, write = readBinary, writeCapnproto

// 	case ".capnproto":
// 		resultExt = ".bin"
// 		read, write = readCapnproto, writeBinary

// 	default:
// 		log.Fatalf("Invalid extension '%s'", givenExt)
// 	}

// 	givenFilename := filename
// 	resultFilename := filename[:len(filename)-len(givenExt)] + resultExt

// 	students, err := read(givenFilename)
// 	if err != nil {
// 		log.Fatalf("Failed to read data: %s", err)
// 	}
// 	fmt.Printf("Writing into %s...\n", resultFilename)
// 	err = write(students, resultFilename)
// 	if err != nil {
// 		log.Fatalf("Failed to write data: %s", err)
// 	}

// }
