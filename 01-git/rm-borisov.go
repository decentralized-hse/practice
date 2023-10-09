package main

import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"os"
	"sort"
	"strings"
)

func exitIfError(err error) {
	if err != nil {
		fmt.Println(fmt.Sprintf("Error: %s", err))
		os.Exit(1)
	}
}

func removeFromSliceByIndex[T any](slice []T, index int) []T {
	return append(slice[:index], slice[index+1:]...)
}

func splitTreeLine(treeLine string) (string, string) {
	splittedTreeLine := strings.Split(treeLine, "\t")
	if len(splittedTreeLine) != 2 {
		fmt.Fprintf(os.Stderr, "Error: the tree line '%s' doesn't contain exactly 1 '\\t' char.\n", treeLine)
		os.Exit(1)
	}
	return splittedTreeLine[0], splittedTreeLine[1]
}

func createTreeLine(objectName string, objectSha256Hash string) string {
	return strings.Join([]string{objectName, objectSha256Hash}, "\t")
}

func readSortedTreeLines(treeSha256Hash string) []string {
	treeByteContent, err := os.ReadFile(treeSha256Hash)
	exitIfError(err)
	return strings.Split(string(treeByteContent), "\n")
}

func getObjectIndexInTree(sortedTreeLines []string, objectName string) int {
	objectIndex := sort.SearchStrings(sortedTreeLines, objectName)
	if objectIndex == len(sortedTreeLines) {
		fmt.Fprintf(os.Stderr, "Error: can't find the %s object.\n", objectName)
		os.Exit(1)
	}

	treeLineName, _ := splitTreeLine(sortedTreeLines[objectIndex])
	if len(treeLineName) != len(objectName)+1 ||
		objectName != treeLineName[:len(treeLineName)-1] {
		fmt.Fprintf(os.Stderr, "Error: can't find the %s object.\n", objectName)
		os.Exit(1)
	}
	return objectIndex
}

func GetHexEncodedSha256Hash(bytesToHash []byte) string {
	sha256HashBytes := sha256.Sum256(bytesToHash)
	return hex.EncodeToString(sha256HashBytes[:])
}

func createTree(sortedTreeLines []string) string {
	newTreeByteContent := []byte(strings.Join(sortedTreeLines, "\n"))
	newTreeSha256Hash := GetHexEncodedSha256Hash(newTreeByteContent)
	err := os.WriteFile(newTreeSha256Hash, newTreeByteContent, 0644)
	exitIfError(err)
	return newTreeSha256Hash
}

func removeObjectFromTreeLinesByIndexRecursively(sortedTreeLines []string, objectLineIndex int, nextObjectsNamesPath []string) []string {
	treeLineName, treeLineHash := splitTreeLine(sortedTreeLines[objectLineIndex])
	switch lastObjectNameChar := treeLineName[len(treeLineName)-1]; lastObjectNameChar {
	case '/':
		if len(nextObjectsNamesPath) > 0 {
			newSubtreeSha256Hash := removeObjectFromTreeRecursively(treeLineHash, nextObjectsNamesPath)
			sortedTreeLines[objectLineIndex] = createTreeLine(treeLineName, newSubtreeSha256Hash)
		}
	case ':':
		if len(nextObjectsNamesPath) > 0 {
			fmt.Fprintf(os.Stderr, "Error: the %s object is not a tree.\n", treeLineName)
			os.Exit(1)
		}
	default:
		fmt.Fprintf(os.Stderr, "Error: the %s object name doesn't end with ':' or '/'.\n", treeLineName)
		os.Exit(1)
	}
	if len(nextObjectsNamesPath) == 0 {
		sortedTreeLines = removeFromSliceByIndex(sortedTreeLines, objectLineIndex)
	}
	return sortedTreeLines
}

func removeObjectFromTreeRecursively(treeSha256Hash string, objectsNamesPath []string) string {
	sortedTreeLines := readSortedTreeLines(treeSha256Hash)
	objectLineIndex := getObjectIndexInTree(sortedTreeLines, objectsNamesPath[0])
	return createTree(removeObjectFromTreeLinesByIndexRecursively(
		sortedTreeLines, objectLineIndex, objectsNamesPath[1:]))
}

func getObjectsNames(objectPath string) []string {
	objectNames := strings.Split(string(objectPath), "/")
	for _, objectName := range objectNames {
		if objectName == "" {
			fmt.Fprintf(os.Stderr, "Error: some of the object names is an empty string.\n")
			os.Exit(1)
		}
	}
	return objectNames
}

func verifyArgsLength(args []string) {
	if len(args) != 3 {
		fmt.Fprintf(os.Stderr, "Usage: %s object_path root_tree_sha256_hash\n", args[0])
		os.Exit(1)
	}
}

func isObjectPathCharValid(char rune) bool {
	if char <= ' ' || char == '\t' || char == ':' {
		return false
	}
	return true
}

func verifyObjectPath(objectPath string) {
	for _, ch := range objectPath {
		if !isObjectPathCharValid(ch) {
			fmt.Fprintf(os.Stderr, "Error: the object path contains the invalid character '%c'.\n", ch)
			os.Exit(1)
		}
	}
}

func main() {
	args := os.Args
	verifyArgsLength(args)
	verifyObjectPath(args[1])
	objectNames := getObjectsNames(args[1])
	fmt.Print(removeObjectFromTreeRecursively(args[2], objectNames))
}
