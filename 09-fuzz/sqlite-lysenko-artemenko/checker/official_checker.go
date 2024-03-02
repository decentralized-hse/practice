package checker

import (
	"encoding/binary"
	"fmt"
	"os"
)

type student struct {
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

type IncorrectFormatError struct {
	where string
}

func (e *IncorrectFormatError) Error() string {
	return fmt.Sprintf("Malformed input at %s\n", e.where)
}

func checkStr(str []byte, name string) error {
	i := 0

	for i < len(str) && str[i] != 0 {
		i++
	}

	for i < len(str) && str[i] == 0 {
		i++
	}

	if i != len(str) {
		return &IncorrectFormatError{name}
	}

	return nil
}

func CheckStudent(student *student) error {
	if err := checkStr(student.Name[:], "name"); err != nil {
		return err
	}

	if err := checkStr(student.Login[:], "login"); err != nil {
		return err
	}

	if err := checkStr(student.Group[:], "group"); err != nil {
		return err
	}

	for _, p := range student.Practice[:] {
		if p != 0 && p != 1 {
			return &IncorrectFormatError{"practice"}
		}
	}

	if err := checkStr(student.Project.Repo[:], "repo"); err != nil {
		return err
	}

	if student.Project.Mark > 10 {
		return &IncorrectFormatError{"project mark"}
	}

	if student.Mark < 0 || student.Mark > 10 {
		return &IncorrectFormatError{"mark"}
	}

	return nil
}

func CheckFile(file *os.File) error {
	var student student
	for {
		err := binary.Read(file, binary.LittleEndian, &student)
		if err != nil {
			break
		}

		if err := CheckStudent(&student); err != nil {
			return err
		}
	}

	return nil
}
