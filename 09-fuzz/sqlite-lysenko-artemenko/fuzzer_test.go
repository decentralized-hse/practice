package test

import (
	"bytes"
	"database/sql"
	"errors"
	"fmt"
	"io"

	"testing"

	"github.com/spf13/afero"

	solution "sqlite-lysenko-artemenko-fuzzer/sqlite-lysenko"
)

const (
	bin_in_path  string = "data_in.bin"
	bin_out_path string = "data_out.bin"
	db_path      string = ":memory:"
)

func FuzzSqlite(f *testing.F) {
	f.Fuzz(func(t *testing.T, input_bytes []byte) {
		fs := afero.NewMemMapFs()

		// create test
		bin_in_file, err := fs.Create(bin_in_path)
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

		// open connection
		db, err := sql.Open("sqlite3", db_path)
		if err != nil {
			t.Error(err)
			return
		}
		defer db.Close()

		// load from file
		bin_students, load_bin_err := solution.LoadFromBinary(fs, bin_in_path)

		if load_bin_err != nil {
			switch {
			case errors.Is(load_bin_err, io.ErrUnexpectedEOF):
				return
			case errors.Is(load_bin_err, solution.IncorrectFormatError):
				return
			default:
				t.Error(load_bin_err)
				return
			}
		}

		// serialize parsed students
		if err := solution.SaveToTable(db, bin_students); err != nil {
			t.Error(err)
			return
		}

		// parse students from database
		db_students, err := solution.LoadFromTable(db)
		if err != nil {
			t.Error(err)
			return
		}

		// check round-trip
		if len(db_students) != len(bin_students) {
			t.Error("round trip failed")
			return
		}

		for i := 0; i < len(db_students); i += 1 {
			db := []byte(fmt.Sprintf("%v", db_students[i]))
			bin := []byte(fmt.Sprintf("%v", bin_students[i]))

			if !bytes.Equal(db, bin) {
				t.Error("round trip failed")
				return
			}
		}

		// check serialization
		if err := solution.SaveToBinary(fs, bin_out_path, db_students); err != nil {
			t.Error(err)
			return
		}

		// check serialized data
		out_file, err := fs.Open(bin_out_path)
		if err != nil {
			t.Error(err)
			return
		}

		out_bytes, err := io.ReadAll(out_file)
		if err != nil {
			t.Error(err)
			return
		}

		if !bytes.Equal(input_bytes, out_bytes) {
			t.Error("serialzed bytes does not equal to test data")
			return
		}
	})
}
