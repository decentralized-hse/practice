package main

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"log"
	"os"
	"strconv"
)

func getHashes(filepath string) []string {
	inputFile, err := os.Open(fmt.Sprintf("%s.hashtree", filepath))
	if err != nil {
		log.Fatalf("error on open file: %v\n", err)
	}
	defer inputFile.Close()
	inputReader := bufio.NewReader(inputFile)
	var hashes []string
	for more := true; more; {
		line, err := inputReader.ReadString('\n')
		if errors.Is(err, io.EOF) {
			more = false
		} else if err != nil {
			log.Fatalf("error on reading file: %v", err)
		}
		if len(line) != 0 {
			hashes = append(hashes, line)
		}
	}
	return hashes
}

func getIndex(block, level int) int {
	return (1<<level)*(2*block+1) - 1
}

func getSiblingByParent(current, parent, childDist int) int {
	if parent-childDist == current {
		return parent + childDist
	}
	return parent - childDist
}

func getProof(hashes []string, blockNumber int) []string {
	var res []string
	current := 2 * blockNumber
	level := 1
	childDist := 1
	for ; ; level++ {
		parent := getIndex(blockNumber, level)
		sibling := getSiblingByParent(current, parent, childDist)
		if sibling >= len(hashes) {
			break
		}
		res = append(res, hashes[sibling])
		childDist <<= 1
		current = parent
	}
	return res
}

func main() {
	if len(os.Args) < 3 {
		log.Fatalf("not enough arguments\n")
	}
	hashes := getHashes(os.Args[1])
	blockNumber, err := strconv.Atoi(os.Args[2])
	if err != nil {
		log.Fatalf("error on parsing block number: %v", err)
	}
	file, err := os.OpenFile(fmt.Sprintf("%s.%s.proof", os.Args[1], os.Args[2]), os.O_CREATE|os.O_RDWR|os.O_TRUNC, 0644)
	if err != nil {
		log.Fatalf("error on open file: %v", err)
	}
	defer file.Close()
	for _, line := range getProof(hashes, blockNumber) {
		_, err = file.WriteString(line + "\n")
		if err != nil {
			log.Fatalf("error on writing result to file: %v", err)
		}
	}
}
