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
)

type ProjectC struct {
	Repo [59]uint8
	Mark uint8
}

type StudentC struct {
	Name     [32]uint8
	Login    [16]uint8
	Group    [8]uint8
	Practice [8]uint8
	Project  ProjectC
	Mark     float32
}

type StudentsXML struct {
	Students []StudentXML `xml:"student"`
}

type projectXML struct {
	Repo []int32 `xml:"repo"`
	Mark uint8   `xml:"mark"`
}

type StudentXML struct {
	Name     []int32    `xml:"name"`
	Login    []int32    `xml:"login"`
	Group    []int32    `xml:"group"`
	Practice []int32    `xml:"practice"`
	Project  projectXML `xml:"project"`
	Mark     float32    `xml:"mark"`
}

func goStruct2xmlStruct(student StudentC) StudentXML {
	xmlStudent := StudentXML{
		Name:  toInt32(student.Name[:])[:],
		Login: toInt32(student.Login[:])[:],
		Group: toInt32(student.Group[:])[:],
		Project: projectXML{
			Repo: toInt32(student.Project.Repo[:])[:],
			Mark: student.Project.Mark,
		},
		Mark:     student.Mark,
		Practice: toInt32(student.Practice[:])[:],
	}
	return xmlStudent
}

func toInt32(uint []uint8) []int32 {
	var res []int32
	for _, u := range uint {
		res = append(res, int32(u))
	}
	return res
}

func toUInt(ints []int32) []uint8 {
	var res []uint8
	for _, u := range ints {
		res = append(res, uint8(u))
	}
	return res
}

func xmlStruct2goStruct(xmlStudent StudentXML) StudentC {
	var student StudentC

	copy(student.Name[:], toUInt(xmlStudent.Name[:]))
	copy(student.Login[:], toUInt(xmlStudent.Login[:]))
	copy(student.Group[:], toUInt(xmlStudent.Group[:]))
	copy(student.Project.Repo[:], toUInt(xmlStudent.Project.Repo[:]))
	for i, practice := range xmlStudent.Practice {
		student.Practice[i] = uint8(practice)
	}
	student.Project.Mark = xmlStudent.Project.Mark
	student.Mark = xmlStudent.Mark

	return student
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
		students = append(students, xmlStruct2goStruct(studentXML))
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
		log.Panic(err)
	}
	err = write(outputFilename, students)
	if err != nil {
		log.Panic(err)
	}

	fmt.Printf("%s => %s\n", inputFilename, outputFilename)
}
