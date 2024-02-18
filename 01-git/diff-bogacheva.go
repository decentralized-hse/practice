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
	prevFiles, isPrevDirectory, err := getFilesInDir(prevHash)
	if err != nil {
		return "", err
	}
	newFiles, isNewDirectory, err := getFilesInDir(newHash)
	if err != nil {
		return "", err
	}
	var diff strings.Builder
	for file, prevHash := range prevFiles {
		if isPrevDirectory[file] {
			continue
		}
		if newHash, ok := newFiles[file]; ok {
			if isNewDirectory[file] {
				continue
			}
			if prevHash != newHash {
				diff.WriteString(fmt.Sprintf("- %s\n", filepath.Join(currentPath, file)))
			}
			delete(newFiles, file)
		} else {
			diff.WriteString(fmt.Sprintf("- %s\n", filepath.Join(currentPath, file)))
		}
	}

	for file := range newFiles {
		if isNewDirectory[file] {
			continue
		}
		diff.WriteString(fmt.Sprintf("+ %s\n", filepath.Join(currentPath, file)))
	}

	for dir := range prevFiles {
		prevIsDir := isPrevDirectory[dir]
		newIsDir := isNewDirectory[dir]

		if prevIsDir && newIsDir {
			subDiff, _ := getDiff(prevFiles[dir], newFiles[dir], filepath.Join(currentPath, dir))

			if subDiff != "" {
				diff.WriteString(fmt.Sprintf("d %s\n", filepath.Join(currentPath, dir)))
				diff.WriteString(subDiff)
			}
		}
	}

	return diff.String(), nil
}

func getFilesInDir(path string) (map[string]string, map[string]bool, error) {
	files := make(map[string]string)
	isDirectory := make(map[string]bool)
	data, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, nil, err
	}

	lines := strings.Split(string(data), "\n")
	for _, line := range lines {
		line = strings.TrimSpace(line)

		// Пропустить пустые строки
		if line == "" {
			continue
		}

		parts := strings.SplitN(line, ":\t", 2)
		isDirectoryCur := false
		if len(parts) != 2 {
			parts = strings.SplitN(line, "/\t", 2)
			isDirectoryCur = true
		}
		if len(parts) != 2 {
			return nil, nil, err
		}

		name := strings.TrimSpace(parts[0])
		hash := strings.TrimSpace(parts[1])

		files[name] = hash
		isDirectory[name] = isDirectoryCur
	}

	return files, isDirectory, nil
}
