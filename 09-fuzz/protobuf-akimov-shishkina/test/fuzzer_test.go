package test

import (
	"bytes"
	"errors"
	"io"
	"os"
	"protobuf-akimov-shishkina-fuzzer/usecases"

	"testing"
)

const (
	filename_bin      string = "testdata.bin"
	filename_protobuf string = "testdata.protobuf"
)

func FuzzProtobuf(f *testing.F) {
	f.Fuzz(func(t *testing.T, input_bytes []byte) {
		// create test
		bin_in_file, err := os.OpenFile(filename_bin, os.O_RDWR|os.O_CREATE, 0666)
		if err != nil {
			t.Error(err)
			return
		}
		defer bin_in_file.Close()

		// write test data to test-fs file
		n, err := bin_in_file.Write(input_bytes)
		if err != nil || n != len(input_bytes) {
			t.Error("cannot create test file")
			return
		}

		// load from file
		bin_students, load_bin_err := usecases.LoadFromBinary(filename_bin)
		t.Log("students, error: ", bin_students, load_bin_err)

		if load_bin_err != nil {
			switch {
			case errors.Is(load_bin_err, io.ErrUnexpectedEOF):
				return
			case errors.Is(load_bin_err, usecases.ErrIncorrectFormat):
				return
			default:
				t.Error(load_bin_err)
				return
			}
		}

		err = usecases.SaveToProto(bin_students, filename_protobuf)
		if err != nil {
			t.Error(err)
			return
		}

		students, err := usecases.LoadFromProto(filename_protobuf)
		if err != nil {
			t.Error(err)
			return
		}
		err = usecases.SaveToBinary(students, filename_bin)
		if err != nil {
			t.Error(err)
			return
		}

		// check serialized data
		out_bytes, err := io.ReadAll(bin_in_file)
		if err != nil {
			t.Error(err)
			return
		}

		if !bytes.Equal(input_bytes, out_bytes) {
			t.Errorf("serialized bytes do not equal test data")
			return
		}
	})
}
