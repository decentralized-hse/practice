package main

import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"log"
	"merge/internal/mergeTools"
	"os"
	"sort"
	"strings"
	"time"
)

func main() {
	if len(os.Args) != 4 {
		fmt.Println("Usage: ./merge-mamaev <target_hash> <merged_hash> <comment>")
		os.Exit(1)
	}

	target := os.Args[1]
	merged := os.Args[2]
	comment := os.Args[3]
	fmt.Println(target)
	fmt.Println(merged)
	commit := mergeTools.MergeDirectories(target, merged, strings.Split(comment, "\n"))

	sort.Strings(commit.Parent)

	parentHash, err := CreateBlob(commit.Parent)
	if err != nil {
		log.Fatal("Cannot create file in this directory")
		os.Exit(1)
	}

	newFileContent := append(commit.Content, fmt.Sprintf(".parent\t%s", parentHash))
	sort.Strings(newFileContent)

	treeHash, err := CreateBlob(newFileContent)

	loc, err := time.LoadLocation("Europe/Moscow")
	if err != nil {
		log.Fatal("Cannot load TimeZone")
		os.Exit(1)
	}
	date := time.Now().In(loc)

	commitContent := []string{"Root: " + treeHash, "Date: " + date.Format("01 Jan 2005 12:12:12") + " MSK\n", comment}

	commitHash, err := CreateBlob(commitContent)
	commitTreeContent := make([]string, 0)
	for _, line := range newFileContent {
		if strings.HasPrefix(line, ".parent") {
			continue
		}
		commitTreeContent = append(commitTreeContent, line)

	}

	commitTreeContent = append(commitTreeContent, fmt.Sprintf(".parent\t%s", treeHash), fmt.Sprintf(".commit\t%s", commitHash))

	sort.Strings(commitTreeContent)

	result, err := CreateBlob(commitTreeContent)
	if err != nil {
		log.Fatal("cannot create merge tree")
		os.Exit(1)
	}

	fmt.Println("Merge successful. New tree hash:", result)
}

func CreateBlob(content []string) (string, error) {
	hasher := sha256.New()
	fileText := strings.Join(content, "\n")
	hasher.Write([]byte(fileText))
	hash := hex.EncodeToString(hasher.Sum(nil))

	file, err := os.Create(hash)
	if err != nil {
		return "", err
	}

	defer file.Close()

	file.Write([]byte(fileText))

	return hash, nil
}
