package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"io"
	"math/rand"
	"os"
	"strconv"
	"testing"
)

// ############################################################## //
// ########################### Fuzzer ########################### //
// ############################################################## //

func ParseRaw(data []byte) ([]Student, error) {
	buf := &bytes.Buffer{}
	if err := binary.Write(buf, binary.BigEndian, data); err != nil {
		panic(err)
	}

	students := make([]Student, 0)

	for {
		student := Student{}
		if err := binary.Read(buf, binary.LittleEndian, &student); errors.Is(err, io.EOF) {
			return students, nil
		}
		if err := student.Validate(); err != nil {
			return nil, err
		}
		students = append(students, student)
	}
}

func FuzzProcessData(f *testing.F) {
	f.Fuzz(func(t *testing.T, data []byte) {
		idx := uint64(rand.Uint32())<<32 + uint64(rand.Uint32())
		prefix := strconv.FormatUint(uint64(idx), 10)
		testFileBinName := prefix + "tmp.bin"
		testFileSstableName := prefix + "tmp.sstable"
		testFilBin2Name := prefix + "tmp2.sstable"

		fileBin, err := os.Create(testFileBinName)
		if err != nil {
			panic(err)
		}

		defer fileBin.Close()
		defer os.Remove(testFileBinName)
		defer os.Remove(testFileSstableName)
		defer os.Remove(testFilBin2Name)

		if n, err := fileBin.Write(data); err != nil || n != len(data) {
			panic("Cannot create test binary file")
		}
		true_students, true_err := ParseRaw(data)
		read_students, err := readBinary(testFileBinName)
		if true_err != nil && err == nil {
			t.Errorf("Solution readBinary accepted invalid format, true errorReason: %v, output_size: %d", true_err, len(read_students))
			return
		}
		if true_err == nil && err != nil {
			t.Errorf("Solution readBinary rejected valid format, errorReason: %v, true size: %d", err, len(true_students))
			return
		}

		if true_err != nil {
			// Malformed input rejected, nothing to test further
			return
		}

		if err := writeSSTable(read_students, testFileSstableName); err != nil {
			t.Fatalf("Writing sstable failed %v", err)
			return
		}

		sstable_students, err := readSSTable(testFileSstableName)
		if true_err != nil && err == nil {
			t.Errorf("Solution readSSTable accepted invalid format, true errorReason: %v, output size: %d", true_err, len(sstable_students))
			return
		}
		if true_err == nil && err != nil {
			t.Error("Solution readSSTable rejected valid format")
			return
		}

		if err := writeBinary(sstable_students, testFilBin2Name); err != nil {
			t.Fatalf("writeBinary failed %v", err)
		}

		fileBin2, err := os.Open(testFilBin2Name)
		if err != nil {
			t.Fatalf("Error while opening res: %v", err)
			return
		}

		out_bytes, err := io.ReadAll(fileBin2)
		if err != nil {
			t.Fatalf("Error while reading res: %v", err)
			return
		}

		if !bytes.Equal(data, out_bytes) {
			t.Error("Round-trip transformation is non-idempotent")
			t.Error("Original content: ", data)
			t.Error("Transformed content: ", out_bytes)
			t.Error("Original students: ", true_students)
			t.Error("SSTable students: ", sstable_students)
			return
		}
	})
}
