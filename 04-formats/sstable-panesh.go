package main

import (
	"encoding/binary"
	"encoding/gob"
	"bytes"
	"errors"
	"io"
	"log"
	"os"
	"path"
	"strconv"
	"strings"

	"github.com/cockroachdb/pebble/sstable"
	"github.com/cockroachdb/pebble/vfs"
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

func readBinary(filename string) []Student {
	file, err := os.Open(filename)
	if err != nil {
		panic(err)
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
			panic(err)
		}
		students = append(students, st)
	}

	return students
}

func writeBinary(students []Student, filename string) {
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
}

func writeSSTable(students []Student, filename string) error {
	file, err := os.Create(filename)
	if err != nil {
		panic(err)
	}
	defer file.Close()

	w := sstable.NewWriter(file, sstable.WriterOptions{})
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

func readSSTable(filename string) ([]Student) {
	file, err := vfs.Default.Open(filename)
	
	if err != nil {
		panic(err)
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
		students = append(students, s)
	}
	if err := i.Close(); err != nil {
		panic(err)
	}
	return students
}

func main() {
	filename := os.Args[1]
	ext := path.Ext(filename)
	switch {
	case ext == ".bin":
		students := readBinary(filename)
		newFilename := strings.TrimSuffix(filename, ext)+".sstable"
		log.Println(filename, "->", newFilename)
		writeSSTable(students, newFilename)
		return
	case ext == ".sstable":
		students := readSSTable(filename)
		newFilename := strings.TrimSuffix(filename, ext)+".bin"
		log.Println(filename, "->", newFilename)
		writeBinary(students, newFilename)
		return
	default:
		panic("Not implemented")
	}
}

