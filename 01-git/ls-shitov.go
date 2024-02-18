package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strings"
)

func main() {
	argv := os.Args
	if len(argv) != 3 {
		log.Fatal("Wrong arguments, example: ./ls-shitov dir/ 834af9243b300ed2fb9235e74e158c0bcdfd34ef9590ba93b56c70e9f970040f")
	}
	path := argv[1]
	root := argv[2]
	hash, err := FindDir(path, root)
	if err != nil {
		log.Fatal(err.Error())
	}
	ListDirsRecursively(hash, "")
}

func FindDir(path string, root string) (string, error) {
	if path == "." || path == "./" || path == "" {
		return root, nil
	}
	splitted := strings.SplitN(path, "/", 2)
	if len(splitted) < 2 {
		return "", fmt.Errorf("failed to find '%s' for root '%s', is it directory?", path, root)
	}
	curDir, remainingPath := splitted[0], splitted[1]
	curDir += "/"
	file, err := os.Open(root)
	if err != nil {
		return "", fmt.Errorf("failed to open '%s': %s", root, err)
	} else {
		defer file.Close()
	}
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		lineSplitted := strings.SplitN(line, "\t", 2)
		if curDir == lineSplitted[0] {
			return FindDir(remainingPath, lineSplitted[1])
		}
	}
	return "", fmt.Errorf("failed to find '%s' for root '%s'", curDir, root)
}

func ListDirsRecursively(hash string, prefix string) {
	file, err := os.Open(hash)
	if err != nil {
		log.Fatalf("Failed to open '%s': %s", hash, err)
	} else {
		defer file.Close()
	}
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		lineSplitted := strings.SplitN(line, "\t", 2)
		objectName, objectHash := lineSplitted[0], lineSplitted[1]
		if objectName[len(objectName)-1] == ':' {
			fmt.Println(prefix + objectName[:len(objectName)-1])
		} else {
			fmt.Println(objectName)
			ListDirsRecursively(objectHash, prefix+objectName)
		}
	}
}
