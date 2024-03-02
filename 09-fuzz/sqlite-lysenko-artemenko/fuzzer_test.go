package main

import (
	"os"
	"sqlite-lysenko-artemenko-fuzzer/checker"
	solution "sqlite-lysenko-artemenko-fuzzer/sqlite-lysenko"
	"testing"

	"github.com/google/uuid"
)

func FuzzSqlite(f *testing.F) {
	f.Fuzz(func(t *testing.T, data []byte) {
		// generate bin name
		bin_uuid := uuid.New()
		bin_path := bin_uuid.String()

		// generate sqlite name
		db_uuid := uuid.New()
		db_path := db_uuid.String()

		// create test
		bin_file, err := os.Create(bin_path)
		if err != nil {
			t.Error(err)
		}
		defer func() {
			bin_file.Close()
			os.Remove(bin_path)
		}()

		bin_file.Write(data)

		// create empty database
		db_file, err := os.Create(db_path)
		if err != nil {
			t.Error(err)
		}
		defer func() {
			db_file.Close()
			os.Remove(db_path)
		}()

		bin_students, load_bin_err := solution.LoadFromBinary(bin_path)

		// check solution forbids incorrect data format
		if fmt_check_error := checker.CheckFile(bin_file); fmt_check_error != nil {
			if load_bin_err == nil {
				t.Error("solution permits incorrect format")
			}
		}

		// if data incorrect skip test
		if load_bin_err != nil {
			return
		}

		// serialize parsed students
		if err := solution.SaveToDatabase(db_path, bin_students); err != nil {
			t.Error(err)
		}

		// parse students from database
		db_students, err := solution.LoadFromDatabase(db_path)
		if err != nil {
			t.Error(err)
		}

		// check round-trip
		if len(db_students) != len(bin_students) {
			t.Error("round trip failed")
		}

		for i := 0; i < len(db_students); i += 1 {
			if db_students[i].Name != bin_students[i].Name {
				t.Log(db_students[i].Name)
				t.Log(bin_students[i].Name)
				t.Error("round trip failed: Name")
			}

			if db_students[i].Login != bin_students[i].Login {
				t.Log(db_students[i].Login)
				t.Log(bin_students[i].Login)
				t.Error("round trip failed: Login")
			}

			if db_students[i].Group != bin_students[i].Group {
				t.Log(db_students[i].Group)
				t.Log(bin_students[i].Group)
				t.Error("round trip failed: Group")
			}

			if db_students[i].Practice != bin_students[i].Practice {
				t.Log(db_students[i].Practice)
				t.Log(bin_students[i].Practice)
				t.Error("round trip failed: Practice")
			}

			if db_students[i].Project.Repo != bin_students[i].Project.Repo {
				t.Log(db_students[i].Project.Repo)
				t.Log(bin_students[i].Project.Repo)
				t.Error("round trip failed: Project.Repo")
			}

			if db_students[i].Project.Mark != bin_students[i].Project.Mark {
				t.Log(db_students[i].Project.Mark)
				t.Log(bin_students[i].Project.Mark)
				t.Error("round trip failed: Project.Mark")
			}

			if db_students[i].Mark != bin_students[i].Mark {
				t.Log(db_students[i].Mark)
				t.Log(bin_students[i].Mark)
				t.Error("round trip failed: Mark")
			}
		}
	})
}
