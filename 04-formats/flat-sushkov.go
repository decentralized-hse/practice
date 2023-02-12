package main

import (
	"bytes"
	"encoding/binary"
	"errors"
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

const flatBufferMetadataSize = 16
const structSize = 128
const defaultRootTableOffset = 12

var defaultVTable = [3]uint16{
	6, // size of vtable
	8, // size of object
	4, // offset to only field data
}

func readFlat(file *os.File) []Student {
	buf, err := io.ReadAll(file)
	if err != nil {
		log.Fatalf("error on read file: %v", err)
	}
	rootOffset := int32(binary.LittleEndian.Uint32(buf[:4]))
	vtableOffset := rootOffset - int32(binary.LittleEndian.Uint32(buf[rootOffset:rootOffset+4]))
	fieldsOffset := vtableOffset + 4
	firstFieldDataOffset := rootOffset + int32(binary.LittleEndian.Uint16(buf[fieldsOffset:fieldsOffset+2]))
	vectorOffset := firstFieldDataOffset + int32(binary.LittleEndian.Uint32(buf[firstFieldDataOffset:firstFieldDataOffset+4]))
	vectorSize := int(binary.LittleEndian.Uint32(buf[vectorOffset : vectorOffset+4]))
	elemOffset := vectorOffset + 4
	var students []Student
	for i := 0; i < vectorSize; i++ {
		var student Student
		err = binary.Read(bytes.NewReader(buf[elemOffset:elemOffset+structSize]), binary.LittleEndian, &student)
		if err != nil {
			log.Fatalf("error on read to struct: %v", err)
		}
		students = append(students, student)
		elemOffset += structSize
	}
	return students
}

func writeFlat(file *os.File, students []Student) {
	buf := bytes.NewBuffer(make([]byte, 0, flatBufferMetadataSize+structSize))
	var metadata []byte
	// offset to root table
	metadata = binary.LittleEndian.AppendUint32(metadata, defaultRootTableOffset)
	// padding 16 bit
	metadata = binary.LittleEndian.AppendUint16(metadata, 0)
	// vtable
	for _, item := range defaultVTable {
		metadata = binary.LittleEndian.AppendUint16(metadata, item)
	}
	// offset to vtable
	metadata = binary.LittleEndian.AppendUint32(metadata, uint32(2*len(defaultVTable)))
	// offset to vector
	metadata = binary.LittleEndian.AppendUint32(metadata, uint32(4))
	// size of vector
	metadata = binary.LittleEndian.AppendUint32(metadata, uint32(len(students)))
	buf.Write(metadata)
	for _, student := range students {
		err := binary.Write(buf, binary.LittleEndian, &student)
		if err != nil {
			log.Fatalf("error on write struct to temporary buffer: %v", err)
		}
	}
	_, err := file.Write(buf.Bytes())
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
