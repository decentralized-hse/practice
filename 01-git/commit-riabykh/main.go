package main

import (
	"bufio"
	"bytes"
	"crypto/sha256"
	"fmt"
	"html/template"
	"os"
	"sort"
	"strings"
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

func writeFile(data []byte) (string, error) {
	h := sha256.New()
	h.Write(data)
	fn := fmt.Sprintf("%x", h.Sum(nil))
	if err := os.WriteFile(fn, data, 0644); err != nil {
		return "", err
	}
	return fn, nil
}

func makeCommitText(rootHash string, comment string) (string, error) {
	data := struct {
		Hash        string
		Date        string
		Annotations string
	}{rootHash, time.Now().Format("02 Jan 2006 15:04:05 MST"), comment}
	var rendered bytes.Buffer
	if err := commitTpl.Execute(&rendered, data); err != nil {
		return "", err
	}
	return rendered.String(), nil
}

func readFile(fn string) ([]string, error) {
	data, err := os.ReadFile(fn)
	if err != nil {
		return nil, err
	}
	return strings.Split(string(data), "\n"), nil
}

func commit(prevRoot string, comment string) (string, error) {
	lines, err := readFile(prevRoot)
	if err != nil {
		return "", err
	}
	var result []string
	for _, line := range lines {
		if !strings.HasPrefix(line, ".commit") && !strings.HasPrefix(line, ".parent") && line != "" {
			result = append(result, line)
		}
	}
	text, err := makeCommitText(prevRoot, comment)
	if err != nil {
		return "", err
	}
	commitFn, err := writeFile([]byte(text))
	if err != nil {
		return "", err
	}
	result = append(result, fmt.Sprintf(".parent/\t%s", prevRoot), fmt.Sprintf(".commit:\t%s", commitFn))
	sort.Slice(result, func(i, j int) bool {
		return result[i] < result[j]
	})
	return writeFile([]byte(strings.Join(result, "\n") + "\n"))
}

func main() {
	if len(os.Args) != 2 {
		panic(fmt.Sprintf("args count mismatch: must be 2, but go %d", len(os.Args)))
	}
	hash := os.Args[1]

	fmt.Printf("type your commit data:\n")
	reader := bufio.NewReader(os.Stdin)
	annotations, err := reader.ReadString('\n')
	if err != nil {
		panic(fmt.Sprintf("failed to read string from stdin: %v", err))
	}
	newRootHash, err := commit(hash, annotations)
	if err != nil {
		panic(fmt.Sprintf("failed to commit: %v", err))
	}
	fmt.Printf("new root: %s\n", newRootHash)
}
