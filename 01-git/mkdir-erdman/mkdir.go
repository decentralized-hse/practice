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

	fullPath := []string{}
	dirList := strings.Split(path, "/")

	if strings.Contains(path, "/") {
		parentDir := hash
		for i := range dirList {
			fullPath = append(fullPath, parentDir)
			if i+1 == len(dirList) {
				f, err := os.OpenFile(parentDir, os.O_APPEND|os.O_WRONLY|os.O_CREATE, 0600)
				CheckError(err)
				defer f.Close()
				//f.WriteString(fmt.Sprintf("%s/\t%s\n", dirList[i], newDirHash))
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

	if len(fullPath) > 1 {
		updHash := newDirHash
		updDirIndex := len(dirList) - 1
		updDir := dirList[updDirIndex]
		for i := len(fullPath) - 1; i >= 0; i-- {
			prevList := GetFileList(fullPath[i])
			updated := false
			for j := range prevList {
				if strings.Contains(prevList[j], updDir) {
					updated = true
					prevList[j] = fmt.Sprintf("%s/\t%s", updDir, updHash)
				}
				if i == 0 && strings.Contains(prevList[j], ".parent/") {
					prevList[j] = fmt.Sprintf(".parent/\t%s", hash)
				}
			}
			if !updated {
				prevList = append(prevList, fmt.Sprintf("%s/\t%s", updDir, updHash))
			}

			sort.Strings(prevList)
			resStr := ""
			for _, el := range prevList {
				resStr += el
				resStr += "\n"
			}
			updHash = WriteBlob(resStr)

			updDirIndex--
			if updDirIndex >= 0 {
				updDir = dirList[updDirIndex]
			}
		}
		return updHash
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
	resList := []string{}
	for _, el := range list {
		if el != "" {
			resList = append(resList, el)
		}
	}
	return resList
}

func Sha256(s string) string {
	h := sha256.New()
	h.Write([]byte(s))
	return fmt.Sprintf("%x", h.Sum(nil))
}

func WriteBlob(data string) string {
	hash := Sha256(data)
	err := os.WriteFile(hash, []byte(data), 0666)
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
