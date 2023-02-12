package main

import (
	"encoding/binary"
	"fmt"
	"google.golang.org/protobuf/proto"
	"io/ioutil"
	"os"
	"strings"
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
	path := "students.protobuf"
	if strings.Split(path, ".")[1] == "bin" {
		file, _ := os.Open(path)
		defer file.Close()

		var st student
		var alldata []byte
		err := binary.Read(file, binary.LittleEndian, &st)

		for err == nil {
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

			// записываем в файл
			alldata = append(alldata, uData...)

			// считывем следующего студента
			err = binary.Read(file, binary.LittleEndian, &st)
		}

		ioutil.WriteFile(strings.Split(path, ".")[0]+".protobuf", alldata, 0644)
	} else {
		in, _ := ioutil.ReadFile(path)
		const size_one_student = 142

		file, _ := os.Create(strings.Split(path, ".")[0] + ".bin1")
		defer file.Close()

		var s = &Student{}
		err := proto.Unmarshal(in[:size_one_student], s)

		for err == nil {
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

			err1 := binary.Write(file, binary.LittleEndian, st)
			if err1 != nil {
				fmt.Println("Write failed", err1)
			}

			in = in[size_one_student:]
			err = proto.Unmarshal(in[:size_one_student], s)
		}
	}
}
