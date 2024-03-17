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
	file, err := vfs.Default.Open(filename)
	if err != nil {
		return nil, err
	}
	readable, err := sstable.NewSimpleReadable(file)
	if err != nil {
		return nil, err
	}
	reader, err := sstable.NewReader(readable, sstable.ReaderOptions{})
	if err != nil {
		return nil, err
	}
	i, err := reader.NewIter(nil, nil)
	if err != nil {
		return nil, err
	}
	students := []Student{}
	for key, value := i.First(); key != nil; key, value = i.Next() {
		dec := gob.NewDecoder(bytes.NewBuffer(value.InPlaceValue()))
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
	file, err := os.Create(filename)
	if err != nil {
		return err
	}
	defer file.Close()

	w := sstable.NewWriter(file, sstable.WriterOptions{})
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
	filename := os.Args[1]
	ext := path.Ext(filename)
	switch {
	case ext == ".bin":
		students, err := readBinary(filename)
		if err != nil {
			log.Fatalf("Failed to read file %s: %v\n", filename, err)
			os.Exit(-1)
		}
		newFilename := strings.TrimSuffix(filename, ext) + ".sstable"
		log.Println(filename, "->", newFilename)
		if err = writeSSTable(students, newFilename); err != nil {
			log.Fatalf("Failed to write file %s: %v\n", newFilename, err)
			os.Exit(-1)
		}
	case ext == ".sstable":
		students, err := readSSTable(filename)
		if err != nil {
			log.Fatalf("Failed to read file %s: %v\n", filename, err)
			os.Exit(-1)
		}
		newFilename := strings.TrimSuffix(filename, ext) + ".bin"
		log.Println(filename, "->", newFilename)
		if err = writeBinary(students, newFilename); err != nil {
			log.Fatalf("Failed to write file %s: %v\n", newFilename, err)
			os.Exit(-1)
		}
	default:
		log.Fatalf("Unsupported file extension: %s\n", ext)
		os.Exit(-1)
	}
}

/*Удаление паники: В изначальном коде при возникновении ошибки вызывалась паника (panic(err)), что приводило к нежелательному завершению работы программы. В моем коде я заменила эти вызовы паники на возврат ошибок функциями чтения и записи, с последующим контролем этих ошибок в функции main.

Устранение требования к формату файла: В изначальном коде не было проверки на соответствие формата файла, что могло привести к ошибкам при попытке прочитать файл, не удовлетворяющий ожидаемому формату. В моем коде я добавила проверки на ошибки после функций чтения, чтобы обеспечить правильное преждевременное завершение, если формат файла не поддерживается.

Обратимость преобразования данных: Исходный код не гарантировал, что преобразование данных в обе стороны (из бинарного в SSTable и обратно) будет порождать идентичные данные. В моем коде, методы записи и чтения обеспечивают обратимость этого преобразования, за исключением того, что порядок студентов в исходном и конечном массиве может отличаться.*/
