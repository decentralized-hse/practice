package main

import (
	"bytes"
	"database/sql"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"log"
	"math"
	"os"
	"reflect"
	"strings"

	_ "github.com/mattn/go-sqlite3"
)

func createTable(db *sql.DB) error {
	const CREATE_TABLE_SQL = `
		create table Students (
			id           integer not null primary key,
			name         varchar(32),
			login        varchar(16),
			group_       varchar(8),
			practice_1   tinyint,
			practice_2   tinyint,
			practice_3   tinyint,
			practice_4   tinyint,
			practice_5   tinyint,
			practice_6   tinyint,
			practice_7   tinyint,
			practice_8   tinyint,
			project_repo varchar(59),
			project_mark tinyint,
			mark_bits    integer
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
		insert into Students values (
			%d, '%s', '%s', '%s', %d, %d, %d, %d, %d, %d, %d, %d, '%s', %d, %d
		);
	`
	query := fmt.Sprintf(
		INSERT_INTO_TABLE_SQL,
		id,
		strings.ReplaceAll(string(bytes.Trim(s.Name[:], "\x00")), "'", "''"),
		strings.ReplaceAll(string(bytes.Trim(s.Login[:], "\x00")), "'", "''"),
		strings.ReplaceAll(string(bytes.Trim(s.Group[:], "\x00")), "'", "''"),
		s.Practice[0],
		s.Practice[1],
		s.Practice[2],
		s.Practice[3],
		s.Practice[4],
		s.Practice[5],
		s.Practice[6],
		s.Practice[7],
		strings.ReplaceAll(string(bytes.Trim(s.Project.Repo[:], "\x00")), "'", "''"),
		s.Project.Mark,
		math.Float32bits(s.Mark),
	)
	_, err := db.Exec(query)
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

func loadFromDatabase(path string) ([]Student, error) {
	db, err := sql.Open("sqlite3", path)
	if err != nil {
		return nil, err
	}
	defer db.Close()

	return loadFromTable(db)
}

func saveToDatabase(path string, students []Student) error {
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

func loadFromBinary(path string) ([]Student, error) {
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

func saveToBinary(path string, students []Student) error {
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
		students, err := loadFromBinary(path)
		if err != nil {
			log.Fatal(err)
		}

		err = saveToDatabase(strings.TrimSuffix(path, ".bin")+".sqlite", students)
		if err != nil {
			log.Fatal(err)
		}
	} else if strings.HasSuffix(path, ".sqlite") {
		students, err := loadFromDatabase(path)
		if err != nil {
			log.Fatal(err)
		}

		err = saveToBinary(strings.TrimSuffix(path, ".sqlite")+".bin", students)
		if err != nil {
			log.Fatal(err)
		}
	} else {
		log.Fatal("Unknown file format")
	}
}
