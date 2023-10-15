package main

import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"os"
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

func isCharHex(char rune) bool {
	return ('0' <= char && char <= '9' || 'a' <= char && char <= 'f' || 'A' <= char && char <= 'F')
}

func isStringHex(str string) bool {
	for _, char := range str {
		if !isCharHex(char) {
			return false
		}
	}
	return true
}

func isStringSha256Hash(str string) bool {
	return len(str) == 64 && isStringHex(str)
}

func readTreeLines(treeSha256Hash string) []string {
	if !isStringSha256Hash(treeSha256Hash) {
		fmt.Fprintf(os.Stderr, "Error: %s isn't a SHA256 hash.\n", treeSha256Hash)
		os.Exit(1)
	}
	treeByteContent, err := os.ReadFile(treeSha256Hash)
	exitIfError(err)
	if getHexEncodedSha256Hash(treeByteContent) != treeSha256Hash {
		fmt.Fprintf(os.Stderr, "Error: the hash of the %s tree isn't equal to its name.\n", treeSha256Hash)
		os.Exit(1)
	}
	return strings.Split(string(treeByteContent), "\n")
}

func getObjectIndexInTree(treeLines []string, objectName string) int {
	for index, treeLine := range treeLines {
		treeLineObjectName, _ := splitTreeLine(treeLine)
		if len(treeLineObjectName) == len(objectName)+1 ||
			objectName == treeLineObjectName[:len(treeLineObjectName)-1] {
			return index
		}
	}
	return -1
}

func getHexEncodedSha256Hash(bytesToHash []byte) string {
	sha256HashBytes := sha256.Sum256(bytesToHash)
	return hex.EncodeToString(sha256HashBytes[:])
}

func createTree(treeLines []string) string {
	newTreeByteContent := []byte(strings.Join(treeLines, "\n"))
	newTreeSha256Hash := getHexEncodedSha256Hash(newTreeByteContent)
	err := os.WriteFile(newTreeSha256Hash, newTreeByteContent, 0644)
	exitIfError(err)
	return newTreeSha256Hash
}

func removeObjectFromTreeLinesByIndexRecursively(treeLines []string, objectLineIndex int, nextObjectsNamesPath []string) []string {
	treeLineName, treeLineHash := splitTreeLine(treeLines[objectLineIndex])
	switch lastObjectNameChar := treeLineName[len(treeLineName)-1]; lastObjectNameChar {
	case '/':
		if len(nextObjectsNamesPath) > 0 {
			newSubtreeSha256Hash := removeObjectFromTreeRecursively(treeLineHash, nextObjectsNamesPath)
			treeLines[objectLineIndex] = createTreeLine(treeLineName, newSubtreeSha256Hash)
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
		treeLines = removeFromSliceByIndex(treeLines, objectLineIndex)
	}
	return treeLines
}

func removeObjectFromTreeRecursively(treeSha256Hash string, objectsNamesPath []string) string {
	treeLines := readTreeLines(treeSha256Hash)
	objectLineIndex := getObjectIndexInTree(treeLines, objectsNamesPath[0])
	if objectLineIndex == -1 {
		fmt.Fprintf(os.Stderr, "Error: can't find the %s object.\n", objectsNamesPath[0])
		os.Exit(1)
	}
	return createTree(removeObjectFromTreeLinesByIndexRecursively(
		treeLines, objectLineIndex, objectsNamesPath[1:]))
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

func verifyObjectPathChars(objectPath string) {
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
	verifyObjectPathChars(args[1])
	objectNames := getObjectsNames(args[1])
	fmt.Printf("%s\n", removeObjectFromTreeRecursively(args[2], objectNames))
}
