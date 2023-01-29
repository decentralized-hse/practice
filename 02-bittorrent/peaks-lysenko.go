package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strings"
)

const TOTAL_LAYERS = 32

var ZERO_HASH = strings.Repeat("0", 64)

// =====================================================
// Functions taken from
// https://github.com/gritzko/binmap/blob/master/bin.h
// and
// https://github.com/gritzko/binmap/blob/master/bin.cpp
func layerBits(index uint32) uint32 {
	return index ^ (index + 1)
}

func isRight(index uint32) bool {
	return (index & (layerBits(index) + 1)) != 0
}

func sibling(index uint32) uint32 {
	return index ^ (layerBits(index) + 1)
}

func parent(index uint32) uint32 {
	lbs := layerBits(index)
	nlbs := (^uint32(2) + 1) + (^lbs + 1)
	return (index | lbs) & nlbs
}

// =====================================================

func getLayerNodes(layer, totalLayers uint32) (uint32, error) {
	if layer >= totalLayers {
		return 0, fmt.Errorf("layer (%d) should be smaller than totalLayers (%d)", layer, totalLayers)
	}

	return 1 << (totalLayers - layer - 1), nil
}

func getIndex(layerIndex, layer, totalLayers uint32) (uint32, error) {
	totalNodes, err := getLayerNodes(layer, totalLayers)
	if err != nil {
		return 0, err
	}

	if layerIndex >= totalNodes {
		return 0, fmt.Errorf(
			"layerIndex (%d) too big: there are only %d nodes in layer %d", layerIndex, totalNodes, layer,
		)
	}

	if layer == 0 {
		return 2 * layerIndex, nil
	}

	return (1<<(layer+1))*layerIndex + (1 << layer) - 1, nil
}

func readFile(filename string) ([]string, error) {
	file, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	hashes := []string{}
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		hashes = append(hashes, scanner.Text())
	}

	err = scanner.Err()
	if err != nil {
		return nil, err
	}

	return hashes, nil
}

func getPeaks(hashes []string) ([]string, error) {
	peaks := make([]string, TOTAL_LAYERS)
	for i := range peaks {
		peaks[i] = ZERO_HASH
	}

	for layer := uint32(0); layer < TOTAL_LAYERS; layer++ {
		totalNodes, err := getLayerNodes(layer, TOTAL_LAYERS)
		if err != nil {
			return nil, err
		}

		for i := uint32(0); i < totalNodes; i++ {
			index, err := getIndex(i, layer, TOTAL_LAYERS)
			if err != nil {
				return nil, err
			}

			if index >= uint32(len(hashes)) || hashes[index] == ZERO_HASH {
				if isRight(index) {
					leftSiblingIndex := sibling(index)
					peaks[layer] = hashes[leftSiblingIndex]

					parentIndex := parent(index)
					if parentIndex < uint32(len(hashes)) {
						hashes[parentIndex] = ZERO_HASH
					}
				}

				break
			}
		}
	}

	return peaks, nil
}

func writePeaks(filename string, peaks []string) error {
	file, err := os.Create(filename)
	if err != nil {
		return err
	}
	defer file.Close()

	for _, peak := range peaks {
		_, err := file.WriteString(peak + "\n")
		if err != nil {
			return err
		}
	}

	return nil
}

func main() {
	if len(os.Args) != 3 {
		log.Fatal("Arguments: <path>.hashtree <path>.peaks")
	}

	hashtreeFilename, peaksFilename := os.Args[1], os.Args[2]

	hashes, err := readFile(hashtreeFilename)
	if err != nil {
		log.Fatal(err)
	}

	peaks, err := getPeaks(hashes)
	if err != nil {
		log.Fatal(err)
	}

	err = writePeaks(peaksFilename, peaks)
	if err != nil {
		log.Fatal(err)
	}
}
