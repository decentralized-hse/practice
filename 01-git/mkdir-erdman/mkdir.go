package main

import (
	"crypto/sha256"
	"fmt"
	"os"
	"sort"
	"strings"
)

func ParseArgs() (string, string) {
	args := os.Args[1:]
	if len(args) != 2 {
		fmt.Println("Usage: ./mkdir <path> <hash>")
		os.Exit(1)
	}
	return args[0], args[1]
}

func MakeDir(path string, hash string) string {
	list := GetFileList(hash)
	var filteredList []string
	for _, elem := range list {
		if !strings.Contains(elem, ".parent/") && elem != "" {
			filteredList = append(filteredList, elem)
		}
	}
	list = filteredList

	newDirHash := WriteBlob("")
	list = append(list, fmt.Sprintf(".parent/\t%s", hash))

	if strings.Contains(path, "/") {
		dirList := strings.Split(path, "/")
		parentDir := hash
		for i := range dirList {
			if i+1 == len(dirList) {
				f, err := os.OpenFile(parentDir, os.O_APPEND|os.O_WRONLY|os.O_CREATE, 0600)
				CheckError(err)
				defer f.Close()
				f.WriteString(fmt.Sprintf("%s/\t%s\n", dirList[i], newDirHash))
			} else {
				data, err := os.ReadFile(parentDir)
				CheckError(err)
				if strings.Contains(string(data), dirList[i]) {
					parentList := GetFileList(parentDir)
					found := false
					for _, f := range parentList {
						if strings.Contains(f, dirList[i]) {
							parentDir = strings.Split(f, "\t")[1]
							found = true
							break
						}
					}
					if found {
						continue
					}
					fmt.Printf("incorrect structure\n")
					os.Exit(1)
				} else {
					fmt.Printf("%s doesn't exist\n", dirList[i])
					os.Exit(1)
				}
			}
		}
	} else {
		list = append(list, fmt.Sprintf("%s/\t%s", path, newDirHash))
	}
	sort.Strings(list)
	newDirData := strings.Join(list, "\n") + "\n"
	newRootHash := Sha256(newDirData)
	err := os.WriteFile(newRootHash, []byte(newDirData), 0666)
	CheckError(err)
	return newRootHash
}

func GetFileList(path string) []string {
	data, err := os.ReadFile(path)
	CheckError(err)
	list := strings.Split(string(data), "\n")
	return list
}

func Sha256(s string) string {
	h := sha256.New()
	h.Write([]byte(s))
	return fmt.Sprintf("%x", h.Sum(nil))
}

func WriteBlob(data string) string {
	hash := Sha256(data)
	err := os.WriteFile(hash, []byte(""), 0666)
	CheckError(err)
	return hash
}

func CheckError(err error) {
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		os.Exit(1)
	}
}

func main() {
	path, hash := ParseArgs()
	resultHash := MakeDir(path, hash)
	fmt.Printf("New hash: %s\n", resultHash)
}
