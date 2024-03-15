package main

import (
	"encoding/binary"
	"errors"
	"io"
	"os"
	"testing"
	"bytes"
	"fmt"
)

func compareFiles(file1Path, file2Path string) (bool, error) {
    file1, err := os.Open(file1Path)
    if err != nil {
        return false, err
    }
    defer file1.Close()

    file2, err := os.Open(file2Path)
    if err != nil {
        return false, err
    }
    defer file2.Close()

    for {
        b1 := make([]byte, 1024)
        _, err1 := file1.Read(b1)

        b2 := make([]byte, 1024)
        _, err2 := file2.Read(b2)

        if err1 != nil || err2 != nil {
            if err1 == io.EOF && err2 == io.EOF {
                return true, nil // оба файла закончились, и до сих пор они идентичны
            }
            if err1 == io.EOF || err2 == io.EOF {
                return false, nil // один файл закончился раньше другого
            }
            return false, fmt.Errorf("files read error: %v, %v", err1, err2)
        }

        if !bytes.Equal(b1, b2) {
            return false, nil
        }
    }
}

func ReadStudentsFromFile(filename string) ([]Student, error) {
	file, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	students := make([]Student, 0)
	for {
		student := Student{}
		if err := binary.Read(file, binary.LittleEndian, &student); err != nil {
			if errors.Is(err, io.EOF) {
				break
			}

			return nil, err
		}

		if err := validateStudent(student); err != nil {
			return nil, err
		}

		students = append(students, student)
	}

	return students, nil
}

func FuzzProcessData(f *testing.F) {

	f.Fuzz(func(t *testing.T, data []byte) {
		fuzzing_fname := "fuzzing.bin"
		capnproto_fname := "result.capnproto"
		result_fname := "result.bin"
		fuzzingFile, err := os.CreateTemp("", fuzzing_fname)
		if err != nil {
			t.Fatal(err)
		}
		defer os.Remove(fuzzingFile.Name())

		resultCapnProtoFile, err := os.CreateTemp("", capnproto_fname)
		if err != nil {
			t.Fatal(err)
		}
		defer os.Remove(resultCapnProtoFile.Name())

		_, err = fuzzingFile.Write(data)
		if err != nil {
			t.Fatal(err)
		}
		_ = fuzzingFile.Close()

		resultBinFile, err := os.CreateTemp("", result_fname)
		if err != nil {
			t.Fatal(err)
		}
		defer os.Remove(resultBinFile.Name())

		// ----------------
		_, err = ReadStudentsFromFile(fuzzing_fname)
		correct_data := true
		if err != nil {
			correct_data = false
		}
		t.Logf("Data status %v, err : %v\n", correct_data, err)

		students, err := readBinary(fuzzing_fname)
		if !correct_data {
			if err == nil {
				t.Fatalf("Target incorrectly validated the data; Data err %v", err)
			}
			return
		}
		err = writeCapnproto(students, capnproto_fname)
		if err != nil {
			t.Fatalf("writeCapnproto error occured %v", err)
		}

		students, err = readCapnproto(capnproto_fname)
		if err != nil {
			t.Fatalf("writeCapnproto error occured %v", err)
		}

		err = writeBinary(students, result_fname)

		if err != nil {
			t.Fatalf("writeBinary error occured %v", err)
		}

		result, err := compareFiles(fuzzing_fname, result_fname)
		if err != nil {
			t.Fatalf("compareFiles error occured %v", err)
		}

		if !result {
			t.Fatalf("initial and result files not equal %v", err)
		}
	})
}

// func main() {
// 	students, err := ReadStudentsFromFile("example/ivanov.bin")
// 	if err != nil {
// 		fmt.Println(err)
// 		return
// 	}
// 	for _, value := range students {
// 		fmt.Println(value.Name)
// 	}
// }
