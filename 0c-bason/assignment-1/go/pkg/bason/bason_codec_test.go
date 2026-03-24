package bason

import (
	"bytes"
	"strings"
	"testing"
)

func TestEncodeDecodeString(t *testing.T) {
	record := Record{Type: TypeString, Key: "name", Value: "Alice"}
	encoded, err := EncodeBason(record)
	if err != nil {
		t.Fatal(err)
	}
	decoded, size, err := DecodeBason(encoded)
	if err != nil {
		t.Fatal(err)
	}
	if decoded.Type != TypeString || decoded.Key != "name" || decoded.Value != "Alice" {
		t.Errorf("decoded = %+v", decoded)
	}
	if size != len(encoded) {
		t.Errorf("size = %d, encoded len = %d", size, len(encoded))
	}
}

func TestEncodeDecodeNumber(t *testing.T) {
	record := Record{Type: TypeNumber, Key: "age", Value: "25"}
	encoded, err := EncodeBason(record)
	if err != nil {
		t.Fatal(err)
	}
	decoded, _, err := DecodeBason(encoded)
	if err != nil {
		t.Fatal(err)
	}
	if decoded.Type != TypeNumber || decoded.Key != "age" || decoded.Value != "25" {
		t.Errorf("decoded = %+v", decoded)
	}
}

func TestEncodeDecodeBoolean(t *testing.T) {
	record := Record{Type: TypeBoolean, Key: "active", Value: "true"}
	encoded, err := EncodeBason(record)
	if err != nil {
		t.Fatal(err)
	}
	decoded, _, err := DecodeBason(encoded)
	if err != nil {
		t.Fatal(err)
	}
	if decoded.Type != TypeBoolean || decoded.Value != "true" {
		t.Errorf("decoded = %+v", decoded)
	}
}

func TestEncodeDecodeBooleanNull(t *testing.T) {
	record := Record{Type: TypeBoolean, Key: "value", Value: ""}
	encoded, err := EncodeBason(record)
	if err != nil {
		t.Fatal(err)
	}
	decoded, _, err := DecodeBason(encoded)
	if err != nil {
		t.Fatal(err)
	}
	if decoded.Type != TypeBoolean || decoded.Value != "" {
		t.Errorf("decoded = %+v", decoded)
	}
}

func TestShortFormUsed(t *testing.T) {
	record := Record{Type: TypeString, Key: "name", Value: "Alice"}
	encoded, err := EncodeBason(record)
	if err != nil {
		t.Fatal(err)
	}
	if len(encoded) != 11 {
		t.Errorf("encoded len = %d, want 11", len(encoded))
	}
	if encoded[0] != 's' {
		t.Errorf("encoded[0] = %q, want 's'", encoded[0])
	}
}

func TestLongFormUsed(t *testing.T) {
	record := Record{Type: TypeString, Key: "key", Value: strings.Repeat("x", 100)}
	encoded, err := EncodeBason(record)
	if err != nil {
		t.Fatal(err)
	}
	if len(encoded) != 109 {
		t.Errorf("encoded len = %d, want 109", len(encoded))
	}
	if encoded[0] != 'S' {
		t.Errorf("encoded[0] = %q, want 'S'", encoded[0])
	}
}

func TestEncodeDecodeArray(t *testing.T) {
	record := Record{
		Type: TypeArray, Key: "scores",
		Children: []Record{
			{Type: TypeNumber, Key: "0", Value: "95"},
			{Type: TypeNumber, Key: "1", Value: "87"},
		},
	}
	encoded, err := EncodeBason(record)
	if err != nil {
		t.Fatal(err)
	}
	decoded, _, err := DecodeBason(encoded)
	if err != nil {
		t.Fatal(err)
	}
	if decoded.Type != TypeArray || decoded.Key != "scores" {
		t.Errorf("decoded = %+v", decoded)
	}
	if len(decoded.Children) != 2 {
		t.Fatalf("len(Children) = %d", len(decoded.Children))
	}
	if decoded.Children[0].Value != "95" || decoded.Children[1].Value != "87" {
		t.Errorf("children values = %q, %q", decoded.Children[0].Value, decoded.Children[1].Value)
	}
}

func TestEncodeDecodeObject(t *testing.T) {
	record := Record{
		Type: TypeObject, Key: "",
		Children: []Record{
			{Type: TypeString, Key: "name", Value: "Alice"},
			{Type: TypeNumber, Key: "age", Value: "25"},
		},
	}
	encoded, err := EncodeBason(record)
	if err != nil {
		t.Fatal(err)
	}
	decoded, _, err := DecodeBason(encoded)
	if err != nil {
		t.Fatal(err)
	}
	if decoded.Type != TypeObject || len(decoded.Children) != 2 {
		t.Errorf("decoded = %+v", decoded)
	}
	if decoded.Children[0].Key != "name" || decoded.Children[0].Value != "Alice" {
		t.Errorf("child0 = %+v", decoded.Children[0])
	}
	if decoded.Children[1].Key != "age" || decoded.Children[1].Value != "25" {
		t.Errorf("child1 = %+v", decoded.Children[1])
	}
}

func TestDecodeAll(t *testing.T) {
	enc1, _ := EncodeBason(Record{Type: TypeString, Key: "a", Value: "1"})
	enc2, _ := EncodeBason(Record{Type: TypeString, Key: "b", Value: "2"})
	combined := append(enc1, enc2...)
	records, err := DecodeBasonAll(combined)
	if err != nil {
		t.Fatal(err)
	}
	if len(records) != 2 {
		t.Fatalf("len(records) = %d", len(records))
	}
	if records[0].Key != "a" || records[1].Key != "b" {
		t.Errorf("keys = %q, %q", records[0].Key, records[1].Key)
	}
}

func TestRoundTrip(t *testing.T) {
	original := Record{
		Type: TypeObject, Key: "",
		Children: []Record{
			{Type: TypeString, Key: "name", Value: "Bob"},
			{Type: TypeNumber, Key: "score", Value: "100"},
		},
	}
	encoded, err := EncodeBason(original)
	if err != nil {
		t.Fatal(err)
	}
	decoded, _, err := DecodeBason(encoded)
	if err != nil {
		t.Fatal(err)
	}
	reencoded, err := EncodeBason(decoded)
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(encoded, reencoded) {
		t.Error("round trip produced different bytes")
	}
}
