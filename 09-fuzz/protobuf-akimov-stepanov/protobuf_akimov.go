package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io/ioutil"
	"os"
	"reflect"
	"strings"
	"unicode/utf8"

	"google.golang.org/protobuf/proto"
)

type project struct {
	Repo [59]byte
	Mark uint8
}

type student struct {
	Name     [32]byte
	Login    [16]byte
	Group    [8]byte
	Practice [8]uint8
	project
	Mark float32
}

func main() {
	path := os.Args[1]
	if strings.Split(path, ".")[1] == "bin" {
		if !CheckData(path) {
			fmt.Fprintln(os.Stderr, "Malformed input")
			os.Exit(-1)
		}
		content, _ := os.ReadFile(path)
		file, _ := os.Open(path)
		defer file.Close()

		var st student
		var alldata []byte
		err := binary.Read(file, binary.LittleEndian, &st)

		if err == nil {
			fmt.Println(string(st.Name[:]), string(st.Login[:]), string(st.Group[:]), st.Practice, st.Mark)
			fmt.Println(string(st.project.Repo[:]), st.project.Mark)

			slice := [8]byte{}
			copy(slice[:], st.Practice[:])

			// Создаю структуру .proto
			s := &Student{
				Name:     string(st.Name[:]),
				Login:    string(st.Login[:]),
				Group:    string(st.Group[:]),
				Practice: slice[:],
				Project:  &Project{Repo: string(st.project.Repo[:]), Mark: uint32(st.project.Mark)},
				Mark:     st.Mark,
			}

			// bin -> protobuf
			uData, _ := proto.Marshal(s)

			//fix 1
			if !reflect.DeepEqual(content[124:128], []byte{0, 0, 0, 0}) {
				copy(uData[len(uData)-4:], content[124:])
			}

			// записываем в файл
			alldata = append(alldata, uData...)
		}
		ioutil.WriteFile(strings.Split(path, ".")[0]+".protobuf", alldata, 0644)
	} else {
		in, _ := ioutil.ReadFile(path)
		content := in
		const size_one_student = 142

		// file, _ := os.Create(strings.Split(path, ".")[0] + ".bin")
		// defer file.Close()

		var s = &Student{}
		err := proto.Unmarshal(in[:size_one_student], s)

		if err == nil {
			// конвертируем в необзодимые нам типы
			var name [32]byte
			copy(name[:], s.Name)
			var login [16]byte
			copy(login[:], s.Login)
			var group [8]byte
			copy(group[:], s.Group)
			var pr_repo [59]byte
			copy(pr_repo[:], s.Project.Repo)
			var pract [8]uint8
			copy(pract[:], s.Practice)

			st := &student{
				Name:     name,
				Login:    login,
				Group:    group,
				Practice: pract,
				project:  project{Repo: pr_repo, Mark: uint8(s.Project.Mark)},
				Mark:     s.Mark,
			}

			//fix 2
			buf := &bytes.Buffer{}
			binary.Write(buf, binary.LittleEndian, st)
			answer := make([]byte, 128)
			buf.Read(answer)
			if len(content) > 139 {
				copy(answer[124:], content[len(content)-4:])
			}
			err1 := os.WriteFile(strings.Split(path, ".")[0]+".bin", answer, 0644)

			if err1 != nil {
				fmt.Println("Write failed", err1)
			}
		}
	}
}

func CheckData(path string) bool {
	content, err := os.ReadFile(path)
	if err != nil {
		return false
	}
	l := len(content)
	if l != 128 {
		return false
	}
	if !utf8.Valid(content[0:32]) {
		return false
	}
	if !utf8.Valid(content[32:48]) {
		return false
	}
	if !utf8.Valid(content[48:56]) {
		return false
	}
	if !utf8.Valid(content[64:123]) {
		return false
	}
	return true
}

func BinToProto(content []byte) (ans []byte) {
	var st student
	var alldata []byte
	buf := bytes.NewBuffer(content)
	binary.Read(buf, binary.LittleEndian, &st)

	slice := [8]byte{}
	copy(slice[:], st.Practice[:])

	// Создаю структуру .proto
	s := &Student{
		Name:     string(st.Name[:]),
		Login:    string(st.Login[:]),
		Group:    string(st.Group[:]),
		Practice: slice[:],
		Project:  &Project{Repo: string(st.project.Repo[:]), Mark: uint32(st.project.Mark)},
		Mark:     st.Mark,
	}

	// bin -> protobuf
	uData, _ := proto.Marshal(s)

	//fix
	alldata = append(alldata, uData...)
	if !reflect.DeepEqual(content[124:128], []byte{0, 0, 0, 0}) {
		copy(alldata[len(alldata)-4:], content[124:])
	}
	return alldata
}

func ProtoToBin(content []byte) (ans []byte) {
	in := content
	const size_one_student = 142

	var s = &Student{}
	proto.Unmarshal(in[:size_one_student], s)
	// конвертируем в необзодимые нам типы
	var name [32]byte
	copy(name[:], s.Name)
	var login [16]byte
	copy(login[:], s.Login)
	var group [8]byte
	copy(group[:], s.Group)
	var pr_repo [59]byte
	copy(pr_repo[:], s.Project.Repo)
	var pract [8]uint8
	copy(pract[:], s.Practice)

	st := &student{
		Name:     name,
		Login:    login,
		Group:    group,
		Practice: pract,
		project:  project{Repo: pr_repo, Mark: uint8(s.Project.Mark)},
		Mark:     s.Mark,
	}

	buf := &bytes.Buffer{}
	binary.Write(buf, binary.LittleEndian, st)
	answer := make([]byte, 128)
	buf.Read(answer)

	//fix
	if len(content) > 139 {
		copy(answer[124:], content[len(content)-4:])
	}

	return answer
}
