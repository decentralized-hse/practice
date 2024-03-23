package main

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"math"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
)

var ErrInvalidInput = errors.New("malformed input")

type Project struct {
	Repo [59]byte
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

func (s *Student) Validate() error {
	if err := validateString(s.Group[:], 8); err != nil {
		return err
	}
	if err := validateString(s.Name[:], 32); err != nil {
		return err
	}
	if err := validateString(s.Login[:], 16); err != nil {
		return err
	}
	if err := validateString(s.Project.Repo[:], 59); err != nil {
		return err
	}
	if err := s.validatePractice(); err != nil {
		return err
	}
	if err := s.validateMarks(); err != nil {
		return err
	}
	return nil
}

func validateFloats(data []byte) error {
	for i := 0; i < len(data); i += 128 {
		b := data[i+124 : i+128]
		val := math.Float32frombits(binary.LittleEndian.Uint32(b))
		if math.IsInf(float64(val), 0) || math.IsNaN(float64(val)) {
			return fmt.Errorf("invalid float32 %v", b)
		}
	}
	return nil
}

func validateString(b []byte, maxIdx int) error {
	k := bytes.IndexByte(b, 0)
	if k < 0 {
		return nil
	}
	for j := k; j < maxIdx; j++ {
		if b[j] != 0 {
			return fmt.Errorf("invalid input")
		}
	}
	return nil
}

func (s *Student) validatePractice() error {
	for _, val := range s.Practice {
		if val != 0 && val != 1 {
			return fmt.Errorf("invalid input")
		}
	}
	return nil
}

func (s *Student) validateMarks() error {
	if s.Project.Mark > 10 {
		return fmt.Errorf("invalid input")
	}
	if s.Mark < 0 || s.Mark > 10 || math.IsNaN(float64(s.Mark)) {
		return fmt.Errorf("invalid input")
	}
	return nil
}

func ReadBinary(path string) ([]Student, error) {
	byteData, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}

	if len(byteData)%128 != 0 {
		return nil, fmt.Errorf("invalid input")
	}
	if err := validateFloats(byteData); err != nil {
		return nil, err
	}

	r := bytes.NewReader(byteData[:])

	var students []Student
	for {
		var st Student
		err := binary.Read(r, binary.LittleEndian, &st)
		if errors.Is(err, io.EOF) {
			break
		}
		if err != nil {
			return nil, err
		}
		if err = st.Validate(); err != nil {
			return nil, ErrInvalidInput
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

func CToGoString(b []byte) string {
	i := bytes.IndexByte(b, 0)
	if i < 0 {
		i = len(b)
	}
	return string(b[:i])
}

func WriteKeyValue(sts []Student, path string) (err error) {
	var out string
	for id, st := range sts {
		out += fmt.Sprintf("[%v].name: %q\n", id, CToGoString(st.Name[:]))
		out += fmt.Sprintf("[%v].login: %q\n", id, CToGoString(st.Login[:]))
		out += fmt.Sprintf("[%v].group: %q\n", id, CToGoString(st.Group[:]))
		for i, p := range st.Practice {
			out += fmt.Sprintf("[%v].practice.[%v]: %v\n", id, i, p)
		}
		out += fmt.Sprintf("[%v].project.repo: %q\n", id, CToGoString(st.Project.Repo[:]))
		out += fmt.Sprintf("[%v].project.mark: %v\n", id, st.Project.Mark)
		out += fmt.Sprintf("[%v].mark: %s\n", id, strconv.FormatFloat(float64(st.Mark), 'f', -1, 32))
	}

	if err := os.WriteFile(path, []byte(out), 0o644); err != nil {
		return err
	}
	return nil
}

func SortedKeys(m map[int]Student) []int {
	keys := make([]int, len(m))
	i := 0
	for k := range m {
		keys[i] = k
		i++
	}
	sort.Ints(keys)
	return keys
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
		if field == "name" || field == "group" || field == "login" || field == "project.repo" {
			if _, err = fmt.Sscanf(value, "%q", &value); err != nil {
				return nil, err
			}
		}
		if field == "name" {
			copy(st.Name[:], value)
		} else if field == "login" {
			copy(st.Login[:], value)
		} else if field == "group" {
			copy(st.Group[:], value)
		} else if field == "project.repo" {
			copy(st.Project.Repo[:], value)
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
	for k := range SortedKeys(d) {
		res[k] = d[k]
	}

	if len(d) == 0 {
		return nil, nil
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
			if errors.Is(err, ErrInvalidInput) {
				os.Exit(1)
			}
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
