package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"io"
	"testing"

	"github.com/spf13/afero"
)

func ReadStudentsFromFile(fs afero.Fs, filename string) ([]Student, error) {
	file, err := fs.Open(filename)
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
		fs := afero.NewMemMapFs()

		fuzzingFile, err := fs.Create(fuzzing_fname)
		if err != nil {
			t.Fatal(err)
		}

		defer fs.Remove(fuzzing_fname)
		defer fs.Remove(capnproto_fname)
		defer fs.Remove(result_fname)

		n, err := fuzzingFile.Write(data)
		t.Log(len(data))
		if err != nil || n != len(data) {
			fuzzingFile.Close()
			t.Error("cannot create test file")
			return
		}
		if err := fuzzingFile.Close(); err != nil {
			t.Log("error closing file:", err)
		}
		// ----------------
		_, err = ReadStudentsFromFile(fs, fuzzing_fname)
		correct_data := true
		if err != nil {
			correct_data = false
		}
		t.Logf("Data status %v, err : %v\n", correct_data, err)

		students, rerr := readBinary(fs, fuzzing_fname)
		if !correct_data {
			if rerr == nil {
				t.Fatalf("Target incorrectly validated the data; Data err %v", rerr)
			}
			return
		}
		err = writeCapnproto(fs, students, capnproto_fname)
		if err != nil {
			t.Fatalf("writeCapnproto error occured %v", err)
		}

		students, err = readCapnproto(fs, capnproto_fname)
		if err != nil {
			t.Fatalf("readCapnproto error occured %v", err)
		}

		err = writeBinary(fs, students, result_fname)

		if err != nil {
			t.Fatalf("writeBinary error occured %v", err)
		}

		out_file, err := fs.Open(result_fname)
		if err != nil {
			t.Error(err)
			return
		}

		out_bytes, err := io.ReadAll(out_file)
		if err != nil {
			t.Error(err)
			return
		}

		if !bytes.Equal(data, out_bytes) {
			t.Error("serialzed bytes does not equal to test data")
			return
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

// func compareFiles(file1Path, file2Path string) (bool, error) {
//     file1, err := os.Open(file1Path)
//     if err != nil {
//         return false, err
//     }
//     defer file1.Close()

//     file2, err := os.Open(file2Path)
//     if err != nil {
//         return false, err
//     }
//     defer file2.Close()

//     for {
//         b1 := make([]byte, 1024)
//         _, err1 := file1.Read(b1)

//         b2 := make([]byte, 1024)
//         _, err2 := file2.Read(b2)

//         if err1 != nil || err2 != nil {
//             if err1 == io.EOF && err2 == io.EOF {
//                 return true, nil // оба файла закончились, и до сих пор они идентичны
//             }
//             if err1 == io.EOF || err2 == io.EOF {
//                 return false, nil // один файл закончился раньше другого
//             }
//             return false, fmt.Errorf("files read error: %v, %v", err1, err2)
//         }

//         if !bytes.Equal(b1, b2) {
//             return false, nil
//         }
//     }
// }
