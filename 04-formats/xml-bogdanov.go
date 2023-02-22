package main

import (
	"encoding/binary"
	"encoding/xml"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path"
	"strings"
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

type projectXML struct {
	Repo string `xml:"project>repo"`
	Mark uint8  `xml:"project>mark"`
}

type StudentXML struct {
	Name       string  `xml:"name"`
	Login      string  `xml:"login"`
	Group      string  `xml:"group"`
	Practice   []int32 `xml:"practice"`
	projectXML `xml:"project"`
	Mark       float32 `xml:"mark"`
}

func goStruct2xmlStruct(student Student) StudentXML {
	xmlStudent := StudentXML{
		Name:     strings.TrimRight(string(student.Name[:]), "\x00"),
		Login:    strings.TrimRight(string(student.Login[:]), "\x00"),
		Group:    strings.TrimRight(string(student.Group[:]), "\x00"),
		Practice: make([]int32, 8),
		projectXML: projectXML{
			Repo: strings.TrimRight(string(student.Project.Repo[:]), "\x00"),
			Mark: student.Project.Mark,
		},
		Mark: student.Mark,
	}
	for i, practice := range student.Practice {
		xmlStudent.Practice[i] = int32(practice)
	}
	return xmlStudent
}

func xmlStruct2goStruct(xmlStudent StudentXML) Student {
	var student Student

	copy(student.Name[:], xmlStudent.Name)
	copy(student.Login[:], xmlStudent.Login)
	copy(student.Group[:], xmlStudent.Group)
	copy(student.Project.Repo[:], xmlStudent.projectXML.Repo)
	for i, practice := range xmlStudent.Practice {
		student.Practice[i] = uint8(practice)
	}
	student.Project.Mark = xmlStudent.projectXML.Mark
	student.Mark = xmlStudent.Mark

	return student
}

func readBinaryFile(filepath string) ([]Student, error) {
	var students []Student

	f, err := os.Open(filepath)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	for {
		var student Student

		if err := binary.Read(f, binary.LittleEndian, &student); err != nil {
			if err == io.EOF {
				return students, nil
			}
			return nil, err
		}

		students = append(students, student)
	}
}

func readXMLFile(filepath string) ([]Student, error) {
	xmlFile, err := os.Open(filepath)
	if err != nil {
		return nil, fmt.Errorf("Could not open file: %v", err)
	}
	defer xmlFile.Close()

	byteValue, err := ioutil.ReadAll(xmlFile)
	if err != nil {
		return nil, fmt.Errorf("Could not read file contents: %v", err)
	}

	var studentsXML []StudentXML
	err = xml.Unmarshal(byteValue, &studentsXML)
	if err != nil {
		return nil, fmt.Errorf("Could not unmarshall XML: %v", err)
	}

	var students []Student
	for _, studentXML := range studentsXML {
		students = append(students, xmlStruct2goStruct(studentXML))
	}

	return students, nil
}

func writeBinaryFile(filepath string, students []Student) error {
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

func writeXMLFile(filepath string, students []Student) error {
	file, err := os.Create(filepath)
	if err != nil {
		return err
	}
	defer file.Close()

	var studentsXML []StudentXML
	for _, student := range students {
		studentsXML = append(studentsXML, goStruct2xmlStruct(student))
		log.Print(studentsXML[len(studentsXML)-1])
	}

	encoder := xml.NewEncoder(file)
	encoder.Indent("", "\t")
	if err := encoder.Encode(studentsXML); err != nil {
		return err
	}

	return nil
}

func main() {
	inputFilename := os.Args[1]
	outputFilename := os.Args[2]

	if _, err := os.Stat(inputFilename); err != nil {
		log.Panic(err)
	}

	inputExtension := path.Ext(inputFilename)
	outputExtension := path.Ext(outputFilename)

	if inputExtension != ".xml" && outputExtension != ".xml" ||
		inputExtension == ".xml" && outputExtension == ".xml" {
		log.Panic("Exactly one file should be an .xml file")
	}

	var read func(filename string) ([]Student, error)
	var write func(filename string, students []Student) error

	if inputExtension == ".xml" {
		read = readXMLFile
		write = writeBinaryFile
	} else {
		read = readBinaryFile
		write = writeXMLFile
	}

	students, err := read(inputFilename)
	if err != nil {
		log.Panic(err)
	}
	err = write(outputFilename, students)
	if err != nil {
		log.Panic(err)
	}

	fmt.Printf("%s => %s\n", inputFilename, outputFilename)
}
