package main

import (
	"bufio"
	"bytes"
	"fmt"
	"html/template"
	"os"
	"path/filepath"
	"time"
)

const commitFileName = ".commit"
const templateText = `Root: {{.Hash}}
Date: {{.Date}}

{{.Annotations}}`

var commitTpl = MustParseTemplate()

func MustParseTemplate() *template.Template {
	tpl, err := template.New("commit").Parse(templateText)
	if err != nil {
		panic(fmt.Sprintf("failed to parse template: %v", err))
	}
	return tpl
}

func findRootDir(currentPath string, hashID string) (string, error) {
	if currentPath == "/" || currentPath == "" {
		return "", fmt.Errorf("root not found")
	}
	entry, err := os.ReadDir(currentPath)
	if err != nil {
		return "", fmt.Errorf("failed to read dir %s: %v", currentPath, err)
	}
	for _, e := range entry {
		if e.Name() == hashID {
			return currentPath, nil
		}
	}
	return findRootDir(filepath.Join(currentPath, ".."), hashID)
}

func main() {
	if len(os.Args) != 2 {
		panic(fmt.Sprintf("args count mismatch: must be 2, but go %d", len(os.Args)))
	}
	hash := os.Args[1]

	currentPath, err := filepath.Abs(".")
	if err != nil {
		panic(fmt.Sprintf("failed to cast current path to abs: %v", err))
	}
	root, err := findRootDir(currentPath, hash)
	if err != nil {
		panic(fmt.Sprintf("failed to find project root: %v", err))
	}

	fmt.Printf("type your commit data:\n")
	reader := bufio.NewReader(os.Stdin)
	annotations, err := reader.ReadString('\n')
	if err != nil {
		panic(fmt.Sprintf("failed to read string from stdin: %v", err))
	}

	data := struct {
		Hash        string
		Date        string
		Annotations string
	}{hash, time.Now().Format("02 Jan 2006 15:04:05 MST"), annotations}
	var rendered bytes.Buffer
	if err = commitTpl.Execute(&rendered, data); err != nil {
		panic(fmt.Sprintf("failed to execute template: %v", err))
	}

	commitFilepath := filepath.Join(root, commitFileName)
	if err = os.WriteFile(commitFilepath, rendered.Bytes(), 0644); err != nil {
		panic(fmt.Sprintf("failed to write data to file %s: %v", commitFilepath, err))
	}
}
