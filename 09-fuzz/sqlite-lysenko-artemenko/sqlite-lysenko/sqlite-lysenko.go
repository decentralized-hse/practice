package solution

import (
	"database/sql"
	"encoding/binary"
	"errors"
	"io"
	"log"
	"math"
	"os"
	"reflect"
	"strings"

	_ "github.com/mattn/go-sqlite3"
)

func createTable(db *sql.DB) error {
	const DROP_TABLE_SQL = `
		DROP TABLE IF EXISTS Students
	`
	if _, err := db.Exec(DROP_TABLE_SQL); err != nil {
		return err
	}

	const CREATE_TABLE_SQL = `
		CREATE TABLE Students (
			id           INTEGER NOT NULL PRIMARY KEY,
			name         VARCHAR(32),
			login        VARCHAR(16),
			group_       VARCHAR(8),
			practice_1   TINYINT,
			practice_2   TINYINT,
			practice_3   TINYINT,
			practice_4   TINYINT,
			practice_5   TINYINT,
			practice_6   TINYINT,
			practice_7   TINYINT,
			practice_8   TINYINT,
			project_repo VARCHAR(59),
			project_mark TINYINT,
			mark_bits    INTEGER
		);
	`

	_, err := db.Exec(CREATE_TABLE_SQL)

	return err
}

type Student struct {
	Name     [32]uint8
	Login    [16]uint8
	Group    [8]uint8
	Practice [8]uint8
	Project  struct {
		Repo [59]uint8
		Mark uint8
	}
	Mark float32
}

func (s *Student) insertIntoTable(db *sql.DB, id int) error {
	const INSERT_INTO_TABLE_SQL = `
		insert into Students (
			id, name, login, group_, practice_1,
			practice_2, practice_3, practice_4, practice_5, practice_6,
			practice_7, practice_8, project_repo, project_mark, mark_bits)
		values (
			$1, $2, $3, $4, $5,
			$6, $7, $8, $9, $10,
			$11, $12, $13, $14, $15
		);
	`

	_, err := db.Exec(
		INSERT_INTO_TABLE_SQL,
		id,
		s.Name[:],
		s.Login[:],
		s.Group[:],
		s.Practice[0],
		s.Practice[1],
		s.Practice[2],
		s.Practice[3],
		s.Practice[4],
		s.Practice[5],
		s.Practice[6],
		s.Practice[7],
		s.Project.Repo[:],
		s.Project.Mark,
		math.Float32bits(s.Mark),
	)

	return err
}

func loadFromTable(db *sql.DB) ([]Student, error) {
	const SELECT_FROM_TABLE_SQL = "select * from Students;"
	rows, err := db.Query(SELECT_FROM_TABLE_SQL)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	students := make([]Student, 0)
	for rows.Next() {
		var row struct {
			Id          int
			Name        string
			Login       string
			Group       string
			Practice1   uint8
			Practice2   uint8
			Practice3   uint8
			Practice4   uint8
			Practice5   uint8
			Practice6   uint8
			Practice7   uint8
			Practice8   uint8
			ProjectRepo string
			ProjectMark uint8
			MarkBits    uint32
		}
		reflectVal := reflect.ValueOf(&row).Elem()
		numFields := reflectVal.NumField()
		fields := make([]interface{}, numFields)
		for i := 0; i < numFields; i++ {
			field := reflectVal.Field(i)
			fields[i] = field.Addr().Interface()
		}

		if err = rows.Scan(fields...); err != nil {
			return nil, err
		}

		student := Student{}
		copy(student.Name[:], []byte(row.Name))
		copy(student.Login[:], []byte(row.Login))
		copy(student.Group[:], []byte(row.Group))
		for i := 0; i < 8; i++ {
			student.Practice[i] = *fields[4+i].(*uint8)
		}
		copy(student.Project.Repo[:], []byte(row.ProjectRepo))
		student.Project.Mark = row.ProjectMark
		student.Mark = math.Float32frombits(uint32(row.MarkBits))

		students = append(students, student)
	}

	if err = rows.Err(); err != nil {
		return nil, err
	}

	return students, nil
}

func LoadFromDatabase(path string) ([]Student, error) {
	db, err := sql.Open("sqlite3", path)
	if err != nil {
		return nil, err
	}
	defer db.Close()

	return loadFromTable(db)
}

func SaveToDatabase(path string, students []Student) error {
	db, err := sql.Open("sqlite3", path)
	if err != nil {
		return err
	}
	defer db.Close()

	if err := createTable(db); err != nil {
		return err
	}

	for i, student := range students {
		if err := student.insertIntoTable(db, i); err != nil {
			return err
		}
	}

	return nil
}

func LoadFromBinary(path string) ([]Student, error) {
	file, err := os.Open(path)
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

		students = append(students, student)
	}

	return students, nil
}

func SaveToBinary(path string, students []Student) error {
	file, err := os.Create(path)
	if err != nil {
		return err
	}
	defer file.Close()

	for _, student := range students {
		if err := binary.Write(file, binary.LittleEndian, &student); err != nil {
			return err
		}
	}

	return nil
}

func main() {
	if len(os.Args) != 2 {
		log.Fatal("Arguments: <path to file>")
	}

	path := os.Args[1]
	if strings.HasSuffix(path, ".bin") {
		students, err := LoadFromBinary(path)
		if err != nil {
			log.Fatal(err)
		}

		err = SaveToDatabase(strings.TrimSuffix(path, ".bin")+".sqlite", students)
		if err != nil {
			log.Fatal(err)
		}
	} else if strings.HasSuffix(path, ".sqlite") {
		students, err := LoadFromDatabase(path)
		if err != nil {
			log.Fatal(err)
		}

		err = SaveToBinary(strings.TrimSuffix(path, ".sqlite")+".bin", students)
		if err != nil {
			log.Fatal(err)
		}
	} else {
		log.Fatal("Unknown file format")
	}
}
