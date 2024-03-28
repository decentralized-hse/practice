package main

import (
	"os"
	"os/exec"
	"testing"
	"bytes"
)

const(
	inputBinFile = "input-tmp.bin"
	outputXmlFile = "output-tmp.xml"
	outputBinFile = "output-tmp.bin"
)


func FuzzSolution(f *testing.F) {
    i := []byte{208, 152, 208, 178, 208, 176, 208, 189, 32, 208, 152, 208, 178, 208, 176, 208, 189, 208, 190, 208, 178, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 105, 118, 97, 110, 111, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 208, 152, 208, 144, 45, 51, 52, 53, 1, 1, 0, 0, 0, 0, 0, 0, 103, 105, 116, 104, 117, 98, 46, 99, 111, 109, 47, 100, 101, 99, 101, 110, 116, 114, 97, 108, 105, 122, 101, 100, 45, 104, 115, 101, 47, 112, 114, 97, 99, 116, 105, 99, 101, 47, 116, 114, 101, 101, 47, 109, 97, 105, 110, 47, 48, 52, 45, 102, 111, 114, 109, 97, 116, 115, 0, 9, 0, 0, 0, 65}
	f.Add(i)
	f.Fuzz(func(t *testing.T, input_bytes []byte) {
		os.WriteFile(inputBinFile, input_bytes, 0666)
		cmd := exec.Command("./validator", inputBinFile)
		if err := cmd.Run(); err != nil {
			_, err := readBinaryFile(inputBinFile)
			if err == nil {
				t.Errorf("no error for invalid input")
			}
		} else {
			students, err := readBinaryFile(inputBinFile)
			if err != nil {
				if err == invalidUTFError {
					t.Skip()
				}
				t.Error(err)
			}
            if err := writeXMLFile(outputXmlFile, students); err != nil {
                t.Error(err)
			}
			students, err = readXMLFile(outputXmlFile)
			if err != nil {
				t.Error(err)
			}
			if err := writeBinaryFile(outputBinFile, students); err != nil {
                t.Error(err)
			}
			inputFileBytes, err := os.ReadFile(inputBinFile)
			if err != nil {
				t.Error(err)
			}
			transformFileBytes, err := os.ReadFile(outputBinFile)
			if err != nil {
				t.Error(err)
			}
            if !bytes.Equal(inputFileBytes, transformFileBytes) {
				t.Errorf("round trip transformation isn't byte equal")
			}
		}
	})
}