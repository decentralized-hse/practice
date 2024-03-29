package main

import (
	"bytes"
	"encoding/binary"
	"encoding/gob"
	"errors"
	"fmt"
	"io"
	"log"
	"math"
	"os"
	"path"
	"strconv"
	"strings"
	"unicode"
	"unicode/utf8"

	"github.com/cockroachdb/pebble/sstable"
	"github.com/cockroachdb/pebble/vfs"
)

// ############################################################## //
// ########################### Common ########################### //
// ############################################################## //

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

type FileWrapper struct {
	file *os.File
}

func (f FileWrapper) Finish() error {
	return f.file.Close()
}

func (f FileWrapper) Abort() {}

func (f FileWrapper) Write(p []byte) error {
	_, err := f.file.Write(p)
	return err
}

func validateString(b []byte) error {
	i := len(b)
	for i > 0 {
		i--
		if b[i] != 0 {
			break
		}
	}

	if i == 0 {
		return errors.New("null string")
	}

	for i > 0 {
		i--
		if b[i] == 0 {
			return errors.New("null symbol encountered")
		}
	}

	return nil
}

func validateUtf8(b []byte) error {
	if err := validateString(b); err != nil {
		return err
	}

	if !utf8.Valid(b) {
		return errors.New("not a valid UTF-8 encoding")
	}
	return nil
}

func validateASCII(b []byte) error {
	if err := validateString(b); err != nil {
		return err
	}

	for _, c := range b {
		if c > unicode.MaxASCII {
			return errors.New("invalid ASCII symbol")
		}
	}
	return nil
}

func validateBool(b byte) error {
	if b == 0 || b == 1 {
		return nil
	}
	return errors.New("not a bool")
}

func validateBoundedFloat(f float32, min float32, max float32) error {
	if math.IsInf(float64(f), 0) {
		return errors.New("value is infinite")
	}
	if math.IsNaN(float64(f)) {
		return errors.New("value is not a number")
	}
	if f < min || f > max {
		return errors.New("out of bounds")
	}
	if f == 0 && math.Signbit(float64(f)) {
		return errors.New("negative zero")
	}
	return nil
}

func (project Project) Validate() error {
	if err := validateASCII(project.Repo[:]); err != nil {
		return errors.New("project.Repo invalid: " + err.Error())
	}
	if project.Mark > 10 {
		return errors.New("project.Mark is greater than 10")
	}
	return nil
}

func (student Student) Validate() error {
	if err := validateUtf8(student.Name[:]); err != nil {
		return errors.New("student.Name invalid: " + err.Error())
	}
	if err := validateASCII(student.Login[:]); err != nil {
		return errors.New("student.Login invalid: " + err.Error())
	}
	if err := validateASCII(student.Group[:]); err != nil {
		return errors.New("student.Name invalid: " + err.Error())
	}
	if err := student.Project.Validate(); err != nil {
		return errors.New("student.Project invalid: " + err.Error())
	}
	for _, practice := range student.Practice {
		if err := validateBool(practice); err != nil {
			return errors.New("student.Practice invalid: " + err.Error())
		}
	}
	if err := validateBoundedFloat(float32(student.Mark), 0, 10); err != nil {
		return errors.New("student.Mark invalid: " + err.Error())
	}
	return nil
}

// ############################################################## //
// ############################ App  ############################ //
// ############################################################## //

func readBinary(filename string) ([]Student, error) {
	file, err := os.Open(filename)
	if err != nil {
		fmt.Print("Cannot open file")
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
			if errors.Is(err, io.ErrUnexpectedEOF) {
				return nil, err
			} else {
				panic(err)
			}
		}

		if err := st.Validate(); err != nil {
			fmt.Print("Malformed input")
			return nil, err
		}
		students = append(students, st)
	}

	return students, nil
}

func writeBinary(students []Student, filename string) error {
	file, err := os.Create(filename)
	if err != nil {
		panic(err)
	}
	defer file.Close()

	for _, s := range students {
		err := binary.Write(file, binary.LittleEndian, &s)
		if err != nil {
			panic(err)
		}
	}
	return nil
}

func writeSSTable(students []Student, filename string) error {
	file, err := os.Create(filename)
	if err != nil {
		panic(err)
	}
	wrapper := FileWrapper{file}

	w := sstable.NewWriter(wrapper, sstable.WriterOptions{})
	for i, s := range students {
		var sBuf bytes.Buffer
		enc := gob.NewEncoder(&sBuf)
		err = enc.Encode(s)
		if err != nil {
			log.Fatal("encode error:", err)
		}

		w.Set([]byte(strconv.Itoa(i)), sBuf.Bytes())
	}
	return w.Close()
}

func readSSTable(filename string) ([]Student, error) {
	file, err := vfs.Default.Open(filename)

	if err != nil {
		fmt.Print("Cannot open file")
		return nil, err
	}
	readable, err := sstable.NewSimpleReadable(file)
	if err != nil {
		panic(err)
	}
	r, err := sstable.NewReader(readable, sstable.ReaderOptions{})
	if err != nil {
		panic(err)
	}
	i, err := r.NewIter(nil, nil)
	if err != nil {
		panic(err)
	}
	students := []Student{}
	for key, value := i.First(); key != nil; key, value = i.Next() {
		dec := gob.NewDecoder(bytes.NewBuffer(value.InPlaceValue()))
		var s Student
		dec.Decode(&s)
		if err := s.Validate(); err != nil {
			fmt.Print("Malformed input")
			return nil, err
		}
		students = append(students, s)
	}
	if err := i.Close(); err != nil {
		panic(err)
	}
	return students, nil
}

func main() {
	filename := os.Args[1]
	ext := path.Ext(filename)
	switch {
	case ext == ".bin":
		students, _ := readBinary(filename)
		newFilename := strings.TrimSuffix(filename, ext) + ".sstable"
		log.Println(filename, "->", newFilename)
		writeSSTable(students, newFilename)
		return
	case ext == ".sstable":
		students, _ := readSSTable(filename)
		newFilename := strings.TrimSuffix(filename, ext) + ".bin"
		log.Println(filename, "->", newFilename)
		writeBinary(students, newFilename)
		return
	default:
		log.Println("Unsupported format")
		return
	}
}
