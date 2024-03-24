package main

import (
	"bytes"
	"encoding/binary"
	"encoding/gob"
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

	for _, s := range students {
		err := binary.Write(file, binary.LittleEndian, &s)
		if err != nil {
			return err
		}
	}
	return nil
}

func readSSTable(filename string) ([]Student, error) {
	file, err := os.Open(filename)
	if err != nil {
		return nil, err
	}

	defer file.Close()

	reader, err := sstable.NewReader(file, sstable.ReaderOptions{})
	if err != nil {
		return nil, err
	}

	i, err := reader.NewIter(nil, nil)
	if err != nil {
		return nil, err
	}

	students := []Student{}
	for key, value := i.First(); key != nil; key, value = i.Next() {
		dec := gob.NewDecoder(bytes.NewBuffer(value))
		var s Student
		err := dec.Decode(&s)
		if err != nil {
			return nil, err
		}
		students = append(students, s)
	}

	if err := i.Close(); err != nil {
		return nil, err
	}
	return students, nil
}

func writeSSTable(students []Student, filename string) error {
	mem := vfs.NewMem()
	f, err := mem.Create(filename)
	if err != nil {
		return err
	}

	w := sstable.NewWriter(f, sstable.WriterOptions{})
	for i, s := range students {
		var sBuf bytes.Buffer
		enc := gob.NewEncoder(&sBuf)
		err = enc.Encode(s)
		if err != nil {
			return err
		}

		err = w.Set([]byte(strconv.Itoa(i)), sBuf.Bytes())
		if err != nil {
			return err
		}
	}

	return w.Close()
}

func main() {
	args := os.Args
	filename := args[1]
	ext := path.Ext(filename)
	switch {
	case ext == ".bin":
		students, err := readBinary(filename)
		if err != nil {
			log.Printf("Failed to read file %s: %v\n", filename, err)
			os.Exit(-1)
		}
		newFilename := strings.TrimSuffix(filename, ext) + ".sstable"
		log.Println(filename, "->", newFilename)
		if err = writeSSTable(students, newFilename); err != nil {
			log.Printf("Failed to write file %s: %v\n", newFilename, err)
			os.Exit(-1)
			return
		}
	case ext == ".sstable":
		students, err := readSSTable(filename)
		if err != nil {
			log.Printf("Failed to read file %s: %v\n", filename, err)
			os.Exit(-1)
		}
		newFilename := strings.TrimSuffix(filename, ext) + ".bin"
		log.Println(filename, "->", newFilename)
		if err = writeBinary(students, newFilename); err != nil {
			log.Printf("Failed to write file %s: %v\n", newFilename, err)
			os.Exit(-1)
		}
	default:
		log.Printf("Unsupported file extension: %s\n", ext)
		os.Exit(-1)
	}
}
