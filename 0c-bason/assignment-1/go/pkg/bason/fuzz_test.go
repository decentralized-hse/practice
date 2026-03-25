package bason

import (
	"math"
	"strconv"
	"testing"
)

// recordsEqual checks semantic equality of two records (Object: key set and values; Number: numeric equality).
func recordsEqual(a, b Record) bool {
	if a.Type != b.Type || a.Key != b.Key {
		return false
	}
	if a.Type == TypeArray {
		if len(a.Children) != len(b.Children) {
			return false
		}
		for i := range a.Children {
			if !recordsEqual(a.Children[i], b.Children[i]) {
				return false
			}
		}
		return true
	}
	if a.Type == TypeObject {
		aMap := make(map[string]int)
		for i, c := range a.Children {
			aMap[c.Key] = i
		}
		bMap := make(map[string]int)
		for j, c := range b.Children {
			bMap[c.Key] = j
		}
		if len(aMap) != len(bMap) {
			return false
		}
		for key, i := range aMap {
			j, ok := bMap[key]
			if !ok || !recordsEqual(a.Children[i], b.Children[j]) {
				return false
			}
		}
		return true
	}
	if a.Type == TypeNumber {
		return numericValuesEqual(a.Value, b.Value)
	}
	return a.Value == b.Value
}

func numericValuesEqual(va, vb string) bool {
	if va == vb {
		return true
	}
	da, errA := strconv.ParseFloat(va, 64)
	db, errB := strconv.ParseFloat(vb, 64)
	if errA != nil || errB != nil {
		return false
	}
	eps := 1e-9 * (math.Abs(da) + math.Abs(db) + 1)
	return math.Abs(da-db) <= eps
}

// FuzzDecodeBason runs the decoder on arbitrary bytes to find panics or invalid state.
func FuzzDecodeBason(f *testing.F) {
	// Seed with valid short-form records
	f.Add([]byte("s\x44nameAlice"))
	f.Add([]byte("n\x21age25"))
	f.Add([]byte("b\x42activetrue"))
	enc, _ := EncodeBason(Record{Type: TypeString, Key: "k", Value: "v"})
	f.Add(enc)

	f.Fuzz(func(t *testing.T, data []byte) {
		if len(data) == 0 {
			return
		}
		_, _, _ = DecodeBason(data)
	})
}

// FuzzDecodeBasonAll fuzzes the "decode all records" path.
func FuzzDecodeBasonAll(f *testing.F) {
	enc, _ := EncodeBason(Record{Type: TypeString, Key: "a", Value: "1"})
	f.Add(enc)
	enc2, _ := EncodeBason(Record{Type: TypeNumber, Key: "b", Value: "2"})
	f.Add(append(enc, enc2...))

	f.Fuzz(func(t *testing.T, data []byte) {
		_, _ = DecodeBasonAll(data)
	})
}

// FuzzJsonRoundTrip fuzzes JSON -> BASON -> JSON round-trip with corpus seeds.
func FuzzJsonRoundTrip(f *testing.F) {
	f.Add(`{}`)
	f.Add(`[]`)
	f.Add(`{"a":1}`)
	f.Add(`[1,2,3]`)
	f.Add(`{"x":"y","n":null}`)

	f.Fuzz(func(t *testing.T, json string) {
		if len(json) == 0 || len(json) > 10000 {
			return
		}
		basonBytes, err := JsonToBason(json)
		if err != nil {
			return
		}
		if len(basonBytes) == 0 {
			return
		}
		json2, err := BasonToJson(basonBytes)
		if err != nil {
			t.Fatalf("BasonToJson failed: %v", err)
		}
		bason2, err := JsonToBason(json2)
		if err != nil {
			return
		}
		r1, _, err := DecodeBason(basonBytes)
		if err != nil {
			return
		}
		r2, _, err := DecodeBason(bason2)
		if err != nil {
			return
		}
		if !recordsEqual(r1, r2) {
			t.Errorf("round-trip records differ: json=%q -> json2=%q", json, json2)
		}
	})
}

// TestFuzzJsonBasonRoundTrip runs many iterations of random JSON round-trip (like C++ FUZZ_ITERATIONS).
func TestFuzzJsonBasonRoundTrip(t *testing.T) {
	iterations := 10_000
	if testing.Short() {
		iterations = 500
	}
	gen := newRandomJSON(12345)
	for i := 0; i < iterations; i++ {
		json := gen.generate(4, 6)
		basonBytes, err := JsonToBason(json)
		if err != nil || len(basonBytes) == 0 {
			continue
		}
		json2, err := BasonToJson(basonBytes)
		if err != nil {
			continue
		}
		bason2, err := JsonToBason(json2)
		if err != nil || len(bason2) == 0 {
			continue
		}
		r1, _, err := DecodeBason(basonBytes)
		if err != nil {
			continue
		}
		r2, _, err := DecodeBason(bason2)
		if err != nil {
			continue
		}
		if !recordsEqual(r1, r2) {
			t.Fatalf("round-trip failed at iteration %d: json=%q json2=%q", i, json, json2)
		}
	}
}
