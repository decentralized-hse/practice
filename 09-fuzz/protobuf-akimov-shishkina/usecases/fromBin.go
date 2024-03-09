package usecases

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"math"
	"os"
	"protobuf-akimov-shishkina-fuzzer/protobuf"
	"strings"
	"unicode/utf8"

	"google.golang.org/protobuf/proto"
)

func LoadFromBinary(filename string) ([]Student, error) {
	file, err := os.Open(filename)
	if err != nil {
		return nil, fmt.Errorf("error opening file: %v", err)
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

		if err := CheckStudent(&student); err != nil {
			return nil, err
		}
		students = append(students, student)
	}
	return students, nil
}

func SaveToProto(students []Student, filename string) error {
	alldata := []byte{}

	for _, st := range students {
		slice := [8]uint32{}
		for i := 0; i < 8; i++ {
			slice[i] = uint32(st.Practice[i])
		}
		// Создаю структуру .proto
		student := &protobuf.StudentProto{
			Name:     string(st.Name[:]),
			Login:    string(st.Login[:]),
			Group:    string(st.Group[:]),
			Practice: slice[:],
			Project:  &protobuf.ProjectProto{Repo: string(st.Project.Repo[:]), Mark: uint32(st.Project.Mark)},
			Mark:     st.Mark,
		}

		// bin -> protobuf
		uData, err := proto.Marshal(student)
		if err != nil {
			return err
		}

		// записываем в файл
		alldata = append(alldata, uData...)
	}

	return os.WriteFile(strings.Split(filename, ".")[0]+".protobuf", alldata, 0644)
}

func CheckStudent(student *Student) error {
	checkStr := func(str []byte) error {
		if !utf8.Valid(str) {
			return ErrIncorrectFormat
		}
		return nil
	}

	if err := checkStr(student.Name[:]); err != nil {
		return err
	}

	if err := checkStr(student.Login[:]); err != nil {
		return err
	}

	if err := checkStr(student.Group[:]); err != nil {
		return err
	}

	for _, p := range student.Practice[:] {
		if p != 0 && p != 1 {
			return ErrIncorrectFormat
		}
	}

	if err := checkStr(student.Project.Repo[:]); err != nil {
		return err
	}

	if student.Mark < 0 || student.Project.Mark > 10 {
		return ErrIncorrectFormat
	}

	if math.IsNaN(float64(student.Mark)) {
		return ErrIncorrectFormat
	}

	if student.Mark < 0 || student.Mark > 10 {
		return ErrIncorrectFormat
	}

	return nil
}
