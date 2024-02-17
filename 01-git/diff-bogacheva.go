package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
)

func main() {
	args := os.Args
	if len(args) != 4 {
		fmt.Println("Not all params in use")
		os.Exit(1)
	}
	prevRootHash := args[2]
	newRootHash := args[3]

	diff, err := getDiff(prevRootHash, newRootHash, args[1])
	if err != nil {
		fmt.Println("Error:", err)
		return
	}
	fmt.Println(diff)
}

func getDiff(prevHash, newHash, currentPath string) (string, error) {
	prevFiles, err := getFilesInDir(prevHash)
	if err != nil {
		return "", err
	}
	newFiles, err := getFilesInDir(newHash)
	if err != nil {
		return "", err
	}
	var diff strings.Builder
	for file, prevHash := range prevFiles {
		if newHash, ok := newFiles[file]; ok {
			if prevHash != newHash {
				diff.WriteString(fmt.Sprintf("- %s\n", filepath.Join(currentPath, file)))
			}
			delete(newFiles, file)
		} else {
			diff.WriteString(fmt.Sprintf("- %s\n", filepath.Join(currentPath, file)))
		}
	}

	for file := range newFiles {
		diff.WriteString(fmt.Sprintf("+ %s\n", filepath.Join(currentPath, file)))
	}

	for dir := range prevFiles {
		_, prevIsDir := prevFiles[dir+"/"]
		_, newIsDir := newFiles[dir+"/"]

		if prevIsDir && newIsDir {
			subDiff, err := getDiff(prevFiles[dir+"/"], newFiles[dir+"/"], filepath.Join(currentPath, dir))
			if err != nil {
				return "", err
			}

			if subDiff != "" {
				diff.WriteString(fmt.Sprintf("d %s\n", filepath.Join(currentPath, dir)))
				diff.WriteString(subDiff)
			}
		}
	}

	return diff.String(), nil
}

func getFilesInDir(hash string) (map[string]string, error) {
	files := make(map[string]string)
	dirPath := hash
	fileInfos, err := ioutil.ReadDir(dirPath)
	if err != nil {
		return nil, err
	}
	for _, fileInfo := range fileInfos {
		fileName := fileInfo.Name()
		if fileName == ".parent" {
			continue
		}
		filePath := filepath.Join(dirPath, fileName)
		if fileInfo.IsDir() {
			hash, err := ioutil.ReadFile(filepath.Join(filePath, ".parent"))
			if err != nil {
				return nil, err
			}
			files[fileName+"/"] = strings.TrimSpace(string(hash))
		} else {
			hash, err := ioutil.ReadFile(filePath)
			if err != nil {
				return nil, err
			}
			files[fileName] = strings.TrimSpace(string(hash))
		}
	}
	return files, nil
}
