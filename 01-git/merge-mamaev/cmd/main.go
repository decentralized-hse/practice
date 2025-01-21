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
	const RFC822 = "02 Jan 06 15:04 MST"

	if len(os.Args) != 4 {
		fmt.Println("Usage: ./merge-mamaev <target_hash> <merged_hash> <comment>")
		os.Exit(1)
	}

	target := os.Args[1]
	merged := os.Args[2]
	comment := os.Args[3]
	commit := mergeTools.MergeDirectories(target, merged, strings.Split(comment, "\n"))

	sort.Strings(commit.Parent)

	parentHash, err := CreateBlob(commit.Parent)
	if err != nil {
		log.Fatal("Cannot create file in this directory " + err.Error())
		os.Exit(1)
	}

	newFileContent := append(commit.Content, fmt.Sprintf(".parent:\t%s", parentHash))
	sort.Strings(newFileContent)

	treeHash, err := CreateBlob(newFileContent)
	loc, err := time.LoadLocation("Europe/Moscow")
	if err != nil {
		log.Fatal("Cannot load TimeZone " + err.Error())
		os.Exit(1)
	}
	date := time.Now().In(loc)
	//To prevent inconsistent state(tree with .commit file, but old hash in its name) need to create new tree refers to old
	commitContent := []string{"Root: " + treeHash, "Date: " + date.Format(RFC822) + "\n", comment}

	commitHash, err := CreateBlob(commitContent)
	commitTreeContent := make([]string, 0)
	for _, line := range newFileContent {
		if strings.HasPrefix(line, ".parent") {
			continue
		}
		commitTreeContent = append(commitTreeContent, line)

	}

	commitTreeContent = append(commitTreeContent, fmt.Sprintf(".parent:\t%s", treeHash), fmt.Sprintf(".commit:\t%s", commitHash))

	sort.Strings(commitTreeContent)

	result, err := CreateBlob(commitTreeContent)
	if err != nil {
		log.Fatal("cannot create merge tree " + err.Error())
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

	file.Write([]byte(fileText))
	defer file.Close()

	return hash, nil
}
