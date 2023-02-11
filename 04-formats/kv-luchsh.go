package main

import (
	"bufio"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

type Project struct {
	Repo [63]byte
	Mark uint8
}

type Student struct {
	Name     [32]byte
	Login    [16]byte
	Group    [8]byte
	Practice [8]uint8
	Project
	Mark float32
}

func ReadBinary(path string) ([]Student, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	var students []Student
	for {
		var st Student
		err := binary.Read(file, binary.LittleEndian, &st)
		if errors.Is(err, io.EOF) {
			break
		}
		if err != nil {
			return nil, err
		}

		students = append(students, st)
	}

	return students, nil
}

func WriteBinary(sts []Student, path string) error {
	file, err := os.OpenFile(path, os.O_TRUNC|os.O_WRONLY|os.O_CREATE, 0o644)
	if err != nil {
		return err
	}
	defer file.Close()

	for _, st := range sts {
		err := binary.Write(file, binary.LittleEndian, &st)
		if err != nil {
			return err
		}
	}

	return nil
}

func WriteKeyValue(sts []Student, path string) (err error) {
	var out []byte
	for id, st := range sts {
		out = append(out, []byte(fmt.Sprintf("[%v].name: %v\n", id, st.Name))...)
		out = append(out, []byte(fmt.Sprintf("[%v].login: %v\n", id, st.Login))...)
		out = append(out, []byte(fmt.Sprintf("[%v].group: %v\n", id, st.Group))...)
		for i, p := range st.Practice {
			out = append(out, []byte(fmt.Sprintf("[%v].practice.[%v]: %v\n", id, i, p))...)
		}
		out = append(out, []byte(fmt.Sprintf("[%v].project.repo: %v\n", id, st.Project.Repo))...)
		out = append(out, []byte(fmt.Sprintf("[%v].project.mark: %v\n", id, st.Project.Mark))...)
		out = append(out, []byte(fmt.Sprintf("[%v].mark: %.5f\n", id, st.Mark))...)
	}

	if err := os.WriteFile(path, out, 0o644); err != nil {
		return err
	}
	return nil
}

func ReadKeyValue(path string) (sts []Student, err error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()
	scanner := bufio.NewScanner(file)
	d := make(map[int]Student)
	var i int
	for scanner.Scan() {
		kv := strings.Split(scanner.Text(), " ")
		value := strings.Join(kv[1:], " ")
		var id int
		var field string
		if _, err := fmt.Sscanf(kv[0][:len(kv[0])-1], "[%d].%s", &id, &field); err != nil && !errors.Is(err, io.EOF) {
			return nil, err
		}
		if _, ok := d[id]; !ok {
			d[id] = Student{}
		}
		st := d[id]
		if field == "name" {
			copy(st.Name[:], []byte(value))
		} else if field == "login" {
			copy(st.Login[:], []byte(value))
		} else if field == "group" {
			copy(st.Group[:], []byte(value))
		} else if field == "project.repo" {
			copy(st.Project.Repo[:], []byte(value))
		} else if field == "project.mark" {
			if mark, err := strconv.ParseUint(value, 10, 8); err != nil {
				return nil, err
			} else {
				st.Project.Mark = uint8(mark)
			}
		} else if field == "mark" {
			if mark, err := strconv.ParseFloat(value, 32); err != nil {
				return nil, err
			} else {
				st.Mark = float32(mark)
			}
		} else if _, err := fmt.Sscanf(field, "practice.[%d]", &i); err == nil || errors.Is(err, io.EOF) {
			if practice, err := strconv.ParseUint(value, 10, 8); err != nil {
				return nil, err
			} else {
				st.Practice[i] = uint8(practice)
			}
		} else {
			return nil, fmt.Errorf("Unknown [%s]", field)
		}
		d[id] = st
	}

	res := make([]Student, len(d))
	for k, v := range d {
		res[k] = v
	}

	return res, nil
}

func main() {
	if len(os.Args) < 2 {
		panic("Not enough arguments")
	}

	path := os.Args[1]
	ext := filepath.Ext(path)
	switch {
	case ext == ".bin":
		sts, err := ReadBinary(path)
		if err != nil {
			panic(err)
		}
		WriteKeyValue(sts, strings.TrimSuffix(path, ext)+".kv")

		return
	case ext == ".kv":
		sts, err := ReadKeyValue(path)
		if err != nil {
			panic(err)
		}
		WriteBinary(sts, strings.TrimSuffix(path, ext)+".bin")

		return
	default:
		panic(fmt.Sprintf("Unknown extension [%s]", ext))
	}
}
