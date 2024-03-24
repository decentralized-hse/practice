package usecases

import (
	"encoding/binary"
	"os"
	"protobuf-akimov-shishkina-fuzzer/protobuf"
	"strings"

	"google.golang.org/protobuf/proto"
)

func LoadFromProto(filename string) ([]*protobuf.StudentProto, error) {
	data, err := os.ReadFile(filename)
	if err != nil {
		return nil, err
	}

	var students []*protobuf.StudentProto
	for len(data) > 0 {
		var student protobuf.StudentProto
		if err := proto.Unmarshal(data, &student); err != nil {
			return nil, ErrIncorrectFormat
		}
		students = append(students, &student)
		data = data[proto.Size(&student):]
	}
	return students, nil
}

func SaveToBinary(students []*protobuf.StudentProto, filename string) error {
	file, err := os.Create(strings.Split(filename, ".")[0] + ".bin")
	if err != nil {
		return err
	}
	defer file.Close()

	for _, student := range students {
		err = binary.Write(file, binary.LittleEndian, convertStudentProtoToStudent(student))
		if err != nil {
			return err
		}
	}
	return nil
}

func convertProjectProtoToProject(protoProject *protobuf.ProjectProto) Project {
	var project Project
	copy(project.Repo[:], protoProject.Repo)
	project.Mark = uint8(protoProject.Mark)
	return project
}

func convertStudentProtoToStudent(protoStudent *protobuf.StudentProto) Student {
	var student Student
	copy(student.Name[:], protoStudent.Name)
	copy(student.Login[:], protoStudent.Login)
	copy(student.Group[:], protoStudent.Group)
	for i, v := range protoStudent.Practice {
		if i < len(student.Practice) {
			student.Practice[i] = uint8(v)
		}
	}
	student.Project = convertProjectProtoToProject(protoStudent.Project)
	student.Mark = protoStudent.Mark
	return student
}
