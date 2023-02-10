package main

import (
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"os"
	"path"
)

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

type projectJSONCompatible struct {
	Repo string `json:"repository"`
	Mark uint8  `json:"mark"`
}

type studentJSONCompatible struct {
	Name                  string   `json:"name"`
	Login                 string   `json:"login"`
	Group                 string   `json:"group"`
	Practice              [8]uint8 `json:"practice"`
	projectJSONCompatible `json:"project"`
	Mark                  float32 `json:"mark"`
}

func (s *Student) UnmarshalJSON(b []byte) error {
	var comp studentJSONCompatible
	err := json.Unmarshal(b, &comp)
	if err != nil {
		return err
	}

	*s = Student{
		Practice: comp.Practice,
		Project: Project{
			Mark: comp.projectJSONCompatible.Mark,
		},
		Mark: comp.Mark,
	}

	copy(s.Name[:], comp.Name[:])
	copy(s.Login[:], comp.Login[:])
	copy(s.Group[:], comp.Group[:])
	copy(s.Project.Repo[:], comp.projectJSONCompatible.Repo[:])

	return nil
}

func (s Student) MarshalJSON() ([]byte, error) {
	comp := studentJSONCompatible{
		Name:     string(s.Name[:]),
		Login:    string(s.Login[:]),
		Group:    string(s.Group[:]),
		Practice: s.Practice,
		projectJSONCompatible: projectJSONCompatible{
			Repo: string(s.Project.Repo[:]),
			Mark: s.Project.Mark,
		},
		Mark: s.Mark,
	}

	return json.Marshal(comp)
}

func readBinary(filename string) ([]Student, error) {
	file, err := os.Open(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to open file '%s': %w", filename, err)
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
			return nil, fmt.Errorf("failed to read next item: %w", err)
		}

		students = append(students, st)
	}

	return students, nil
}

func writeBinary(students []Student, filename string) error {
	file, err := os.Create(filename)
	if err != nil {
		return fmt.Errorf("failed to open file '%s': %w", filename, err)
	}
	defer file.Close()

	for idx, st := range students {
		err := binary.Write(file, binary.LittleEndian, &st)
		if err != nil {
			return fmt.Errorf("failed to write item #%d: %w", idx, err)
		}
	}

	return nil
}

func readJSON(filename string) ([]Student, error) {
	file, err := os.Open(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to open file '%s': %w", filename, err)
	}
	defer file.Close()

	var students []Student
	err = json.NewDecoder(file).Decode(&students)
	if err != nil {
		return nil, fmt.Errorf("failed to read json: %w", err)
	}

	return students, nil
}

func writeJSON(students []Student, filename string) error {
	file, err := os.Create(filename)
	if err != nil {
		return fmt.Errorf("failed to open file '%s': %w", filename, err)
	}
	defer file.Close()

	encoder := json.NewEncoder(file)
	encoder.SetIndent("", "\t")
	err = encoder.Encode(&students)
	if err != nil {
		return fmt.Errorf("failed to write json: %w", err)
	}

	return nil
}

func main() {
	filename := os.Args[len(os.Args)-1]
	fmt.Println("Given filename:", filename)

	givenExt := path.Ext(filename)
	var wantedExt string
	var read func(filename string) ([]Student, error)
	var write func(students []Student, filename string) error

	switch givenExt {
	case ".bin":
		wantedExt = ".json"
		read, write = readBinary, writeJSON

	case ".json":
		wantedExt = ".bin"
		read, write = readJSON, writeBinary

	default:
		log.Fatalf("Invalid extension '%s', supported options: .json, .bin", givenExt)
	}

	givenFilename := filename
	wantedFilename := filename[:len(filename)-len(givenExt)] + wantedExt

	fmt.Printf("Converting from '%s' to '%s'...\n", givenExt, wantedExt)

	fmt.Printf("Reading from %s...\n", givenFilename)
	students, err := read(givenFilename)
	if err != nil {
		log.Fatalf("Failed to read data: %s", err)
	}

	fmt.Printf("Read %d records\n", len(students))

	fmt.Printf("Writing into %s...\n", wantedFilename)
	err = write(students, wantedFilename)
	if err != nil {
		log.Fatalf("Failed to write data: %s", err)
	}

	fmt.Println("Done")
}
