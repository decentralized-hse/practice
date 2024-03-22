package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"fuzz/kv"
	"testing"

	"github.com/stretchr/testify/assert"
)


func FuzzWKT(f *testing.F) {
	i := []byte{208, 152, 208, 178, 208, 176, 208, 189, 32, 208, 152, 208, 178, 208, 176, 208, 189, 208, 190, 208, 178, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 105, 118, 97, 110, 111, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 208, 152, 208, 144, 45, 51, 52, 53, 1, 1, 0, 0, 0, 0, 0, 0, 103, 105, 116, 104, 117, 98, 46, 99, 111, 109, 47, 100, 101, 99, 101, 110, 116, 114, 97, 108, 105, 122, 101, 100, 45, 104, 115, 101, 47, 112, 114, 97, 99, 116, 105, 99, 101, 47, 116, 114, 101, 101, 47, 109, 97, 105, 110, 47, 48, 52, 45, 102, 111, 114, 109, 97, 116, 115, 0, 9, 0, 0, 0, 65}
	f.Add(i)

	f.Fuzz(func(t *testing.T, input []byte) {
		students, err := kv.ReadBinary(input)
		if err != nil {
			t.Skip()
		}

		kvBytes, err := kv.WriteKeyValue(students)
		if err != nil {
			t.Errorf("failed to write to kv, why?")
		}

		students1, err := kv.ReadKeyValue(kvBytes)
		if err != nil {
			t.Errorf("failed to read kv, why?")
		}
		assert.Equal(t, students, students1)

		output, err := kv.WriteBinary(students1)
		if err != nil {
			t.Errorf("failed to write, why?")
			// t.Skip()
		}

		if bytes.Compare(input, output) != 0 {
			print(input)
			print(output)
			t.Errorf("Before: %v, after: %v", []uint8(input), []uint8(output))
		}
	})
}
