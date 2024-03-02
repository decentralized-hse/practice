package test

import (
	"bytes"
	"database/sql"
	"fmt"

	"testing"

	"github.com/spf13/afero"

	checker "sqlite-lysenko-artemenko-fuzzer/checker"
	solution "sqlite-lysenko-artemenko-fuzzer/sqlite-lysenko"
)

const (
	bin_path string = "data.bin"
	db_path  string = ":memory:"
)

func FuzzSqlite(f *testing.F) {
	f.Fuzz(func(t *testing.T, data []byte) {
		fs := afero.NewMemMapFs()

		// create test
		bin_file, err := fs.Create(bin_path)
		if err != nil {
			t.Error(err)
			return
		}
		defer bin_file.Close()

		// write test data to test-fs file
		n, err := bin_file.Write(data)
		if err != nil || n != len(data) {
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
		bin_students, load_bin_err := solution.LoadFromBinary(fs, bin_path)

		// check solution forbids incorrect data format
		if fmt_check_error := checker.CheckFile(bin_file); fmt_check_error != nil {
			if load_bin_err == nil {
				t.Error("solution permits incorrect format")
				return
			}
		}

		// if data incorrect skip test
		if load_bin_err != nil {
			return
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
	})
}
