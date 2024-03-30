package main

import (
	"encoding/binary"
	"encoding/xml"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path"
	"strings"
	"unicode/utf8"
)

type BadInputError struct {
	Info string
}

func (s BadInputError) Error() string {
	return "Переданы невалидные данные" + s.Info
}

var invalidUTFError = BadInputError{"Передана невалидная utf8 строка"}
var invalidStringError = BadInputError{"Передана невалидная строка"}

type ProjectC struct {
	Repo [59]byte
	Mark uint8
}

type StudentC struct {
	Name     [32]byte
	Login    [16]byte
	Group    [8]byte
	Practice [8]uint8
	Project  ProjectC
	Mark     float32
}

type StudentsXML struct {
	Students []StudentXML `xml:"student"`
}

type projectXML struct {
	Repo string `xml:"repo"`
	Mark uint8  `xml:"mark"`
}

type StudentXML struct {
	Name     string     `xml:"name"`
	Login    string     `xml:"login"`
	Group    string     `xml:"group"`
	Practice []int32    `xml:"practice"`
	Project  projectXML `xml:"project"`
	Mark     float32    `xml:"mark"`
}

func ValidateStrings(student StudentC) error {
	if !utf8.ValidString(strings.TrimRight(string(student.Name[:]), "\x00")) {
		return invalidUTFError
	}
	if !utf8.ValidString(strings.TrimRight(string(student.Login[:]), "\x00")) {
		return invalidUTFError
	}
	if !utf8.ValidString(strings.TrimRight(string(student.Group[:]), "\x00")) {
		return invalidUTFError
	}
	if !utf8.ValidString(strings.TrimRight(string(student.Project.Repo[:]), "\x00")) {
		return invalidUTFError
	}

	i := 0
	for ; i < len(student.Name) && student.Name[i] != 0; i++ {}
	for ; i < len(student.Name) && student.Name[i] == 0; i++ {}
	if i != len(student.Name) {
		return invalidStringError
	}

	i = 0
	for ; i < len(student.Login) && student.Login[i] != 0; i++ {}
	for ; i < len(student.Login) && student.Login[i] == 0; i++ {}
	if i != len(student.Login) {
		return invalidStringError
	}

	i = 0
	for ; i < len(student.Group) && student.Group[i] != 0; i++ {}
	for ; i < len(student.Group) && student.Group[i] == 0; i++ {}
	if i != len(student.Group) {
		return invalidStringError
	}

	i = 0
	for ; i < len(student.Project.Repo) && student.Project.Repo[i] != 0; i++ {}
	for ; i < len(student.Project.Repo) && student.Project.Repo[i] == 0; i++ {}
	if i != len(student.Project.Repo) {
		return invalidStringError
	}
	
	return nil
}

func ValidatePractice(student StudentC) error {
	for _, practice := range student.Practice {
		if !(practice == uint8(0) || practice == uint8(1)) {
			return BadInputError{"practice должена иметь значение либо 0, либо 1"}
		}
	}
	return nil
}

func ValidateMark(student StudentC) error {
	if student.Mark < 0 || student.Mark > 10 || student.Mark != student.Mark {
		return BadInputError{"mark имеет невалидное значение"}
	}
	if student.Project.Mark < 0 || student.Project.Mark > 10 {
		return BadInputError{"project mark имеет невалидное значение"}
	}
	return nil
}

func goStruct2xmlStruct(student StudentC) StudentXML {
	xmlStudent := StudentXML{
		Name:     fmt.Sprintf("%q", []byte(strings.TrimRight(string(student.Name[:]), "\x00"))),
		Login:    fmt.Sprintf("%q", []byte(strings.TrimRight(string(student.Login[:]), "\x00"))),
		Group:    fmt.Sprintf("%q", []byte(strings.TrimRight(string(student.Group[:]), "\x00"))),
		Practice: make([]int32, 8),
		Project: projectXML{
			Repo: fmt.Sprintf("%q", []byte(strings.TrimRight(string(student.Project.Repo[:]), "\x00"))),
			Mark: student.Project.Mark,
		},
		Mark: student.Mark,
	}
	for i, practice := range student.Practice {
		xmlStudent.Practice[i] = int32(practice)
	}
	return xmlStudent
}

func xmlStruct2goStruct(xmlStudent StudentXML) (StudentC, error) {
	var student StudentC
    
	tt := student.Name[:]
	if _, err := fmt.Sscanf(xmlStudent.Name, "%q", &tt); err != nil {
		return student, err
	}
	copy(student.Name[:], tt)

	tt = student.Login[:]
	if _, err := fmt.Sscanf(xmlStudent.Login, "%q", &tt); err != nil {
		return student, err
	}
	copy(student.Login[:], tt)

	tt = student.Group[:]
	if _, err := fmt.Sscanf(xmlStudent.Group, "%q", &tt); err != nil {
		return student, err
	}
    copy(student.Group[:], tt)

	tt = student.Project.Repo[:]
	if _, err := fmt.Sscanf(xmlStudent.Project.Repo, "%q", &tt); err != nil {
		return student, err
	}
	copy(student.Project.Repo[:], tt)

	for i, practice := range xmlStudent.Practice {
		student.Practice[i] = uint8(practice)
	}
	student.Project.Mark = xmlStudent.Project.Mark
	student.Mark = xmlStudent.Mark

	return student, nil
}

func readBinaryFile(filepath string) ([]StudentC, error) {
	var students []StudentC

	f, err := os.Open(filepath)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	for {
		var student StudentC

		if err := binary.Read(f, binary.LittleEndian, &student); err != nil {
			if err == io.EOF {
				return students, nil
			}
			return nil, err
		}

		if err := ValidatePractice(student); err != nil {
			return nil, err
		}
		if err := ValidateStrings(student); err != nil {
			return nil, err
		}
		if err := ValidateMark(student); err != nil {
			return nil, err
		}

		students = append(students, student)
	}
}

func readXMLFile(filepath string) ([]StudentC, error) {
	xmlFile, err := os.Open(filepath)
	if err != nil {
		return nil, fmt.Errorf("Could not open file: %v", err)
	}
	defer xmlFile.Close()

	byteValue, err := ioutil.ReadAll(xmlFile)
	if err != nil {
		return nil, fmt.Errorf("Could not read file contents: %v", err)
	}

	var studentsXML StudentsXML
	err = xml.Unmarshal(byteValue, &studentsXML)
	if err != nil {
		return nil, fmt.Errorf("Could not unmarshall XML: %v", err)
	}

	var students []StudentC
	for _, studentXML := range studentsXML.Students {
		st, err := xmlStruct2goStruct(studentXML)
		if err != nil {
			return nil, fmt.Errorf("Could not convert XML: %v", err)
		}
		students = append(students, st)
	}

	for _, student := range students {
		if err := ValidatePractice(student); err != nil {
			return nil, err
		}
		if err := ValidateStrings(student); err != nil {
			return nil, err
		}
		if err := ValidateMark(student); err != nil {
			return nil, err
		}
	}

	return students, nil
}

func writeBinaryFile(filepath string, students []StudentC) error {
	f, err := os.Create(filepath)
	if err != nil {
		return err
	}
	defer f.Close()

	for _, student := range students {
		if err := binary.Write(f, binary.LittleEndian, &student); err != nil {
			return err
		}
	}

	return nil
}

func writeXMLFile(filepath string, students []StudentC) error {
	file, err := os.Create(filepath)
	if err != nil {
		return err
	}
	defer file.Close()

	var studentsXML []StudentXML
	for _, student := range students {
		err := ValidateStrings(student)
		if err != nil {
			return err
		}
		err = ValidatePractice(student)
		if err != nil {
			return err
		}
		studentsXML = append(studentsXML, goStruct2xmlStruct(student))
	}
	result := StudentsXML{Students: studentsXML}

	encoder := xml.NewEncoder(file)
	encoder.Indent("", "\t")
	if err := encoder.Encode(result); err != nil {
		return err
	}

	return nil
}

func properExit(err error) {
	fmt.Printf("%v\n", err)
	os.Exit(1)
}

func main() {
	if len(os.Args) != 3 {
		properExit(fmt.Errorf("Usage: cmd <input_file> <output_file>"))
	}
	inputFilename := os.Args[1]
	outputFilename := os.Args[2]

	if _, err := os.Stat(inputFilename); err != nil {
		properExit(err)
	}

	inputExtension := path.Ext(inputFilename)
	outputExtension := path.Ext(outputFilename)

	if inputExtension != ".xml" && outputExtension != ".xml" ||
		inputExtension == ".xml" && outputExtension == ".xml" {
		properExit(fmt.Errorf("Exactly one file should be an .xml file"))
	}

	var read func(filename string) ([]StudentC, error)
	var write func(filename string, students []StudentC) error

	if inputExtension == ".xml" {
		read = readXMLFile
		write = writeBinaryFile
	} else {
		read = readBinaryFile
		write = writeXMLFile
	}

	students, err := read(inputFilename)
	if err != nil {
		properExit(err)
	}
	err = write(outputFilename, students)
	if err != nil {
		properExit(err)
	}

	fmt.Printf("%s => %s\n", inputFilename, outputFilename)
}