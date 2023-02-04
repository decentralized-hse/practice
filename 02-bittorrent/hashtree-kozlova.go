package main

import (
	"crypto/sha256"
	"fmt"
	"io"
	"log"
	"os"
	"reflect"
)

type DataChunk []byte
type Hash []byte

const ChunkSize = 1024

var EmptyNodeHash = make(Hash, 32)

func chunkSlice(slice []byte, chunkSize int) []DataChunk {
	var chunks []DataChunk
	for i := 0; i < len(slice); i += chunkSize {
		end := i + chunkSize

		if end > len(slice) {
            panic("incomplete chunk");
		}

		chunks = append(chunks, slice[i:end])
	}

	return chunks
}

func ReadAndChunkFile(name string) ([]DataChunk, error) {
	// open file
	f, err := os.Open(name)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	// read stream
	b, err := io.ReadAll(f)
	if err != nil {
		return nil, err
	}

	if len(b)%ChunkSize != 0 {
		return nil, fmt.Errorf("invalid size of file")
	}

	chunks := chunkSlice(b, ChunkSize)
	return chunks, nil
}

var NL = []byte{'\n'}

func calculateHashForChunk(chunk DataChunk) Hash {
	h := sha256.New()
	h.Write(chunk)
	return h.Sum(nil)
}

func calculateHashForNode(left, right Hash) Hash {
	if reflect.DeepEqual(left, EmptyNodeHash) || reflect.DeepEqual(right, EmptyNodeHash) {
		return EmptyNodeHash
	}

	h := sha256.New()
    two := fmt.Sprintf("%x\n%x\n", left, right)
    h.Write([]byte(two))

	return h.Sum(nil)
}

func BuildTree(chunks []DataChunk) []Hash {
	treeSize := len(chunks)*2 - 1
	hashes := make([]Hash, treeSize)

	for i := 0; i < len(hashes); i++ {
		if i%2 == 0 {
			hashes[i] = calculateHashForChunk(chunks[i/2])
		} else {
			hashes[i] = EmptyNodeHash
		}
	}

	for layerPower := 2; layerPower < len(hashes); layerPower *= 2 {
		layerStart := layerPower - 1
		for i := layerStart; i+layerPower/2 < len(hashes); i += layerPower * 2 {
			left := i - layerPower/2
			right := i + layerPower/2

			hashes[i] = calculateHashForNode(hashes[left], hashes[right])
		}
	}

	return hashes
}

func WriteHashesToFile(name string, hashes []Hash) error {
	f, err := os.Create(name)
	if err != nil {
		return err
	}
	defer f.Close()

	for _, hash := range hashes {
		_, err := f.WriteString(fmt.Sprintf("%x\n", hash))
		if err != nil {
			return err
		}
	}

	return nil
}

func main() {
	argsWithoutProg := os.Args[1:]
	if len(argsWithoutProg) < 1 {
		log.Fatalf("no filename provided")
	}
	name := argsWithoutProg[0]
	log.Printf("reading %s...", name)
	chunks, err := ReadAndChunkFile(name)
	if err != nil {
		log.Fatalf(err.Error())
	}
	log.Println("building hash tree based on chunks")
	hashTreeAsList := BuildTree(chunks)
	outputName := name + ".hashtree"
	log.Printf("saving hashtree to %s...", outputName)
	err = WriteHashesToFile(outputName, hashTreeAsList)
	if err != nil {
		log.Fatalf(err.Error())
	}
}
