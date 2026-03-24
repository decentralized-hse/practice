package bason

import (
	"testing"
)

func TestFlatten_SingleLeaf(t *testing.T) {
	record := Record{Type: TypeString, Key: "name", Value: "Alice"}
	flat := FlattenBason(record)
	if len(flat) != 1 {
		t.Fatalf("len(flat) = %d", len(flat))
	}
	if flat[0].Key != "name" || flat[0].Value != "Alice" {
		t.Errorf("flat[0] = %+v", flat[0])
	}
}

func TestFlatten_SimpleObject(t *testing.T) {
	record := Record{
		Type: TypeObject, Key: "",
		Children: []Record{
			{Type: TypeString, Key: "name", Value: "Alice"},
			{Type: TypeNumber, Key: "age", Value: "25"},
		},
	}
	flat := FlattenBason(record)
	if len(flat) != 2 {
		t.Fatalf("len(flat) = %d", len(flat))
	}
	if flat[0].Key != "name" || flat[0].Value != "Alice" {
		t.Errorf("flat[0] = %+v", flat[0])
	}
	if flat[1].Key != "age" || flat[1].Value != "25" {
		t.Errorf("flat[1] = %+v", flat[1])
	}
}

func TestFlatten_NestedObject(t *testing.T) {
	root := Record{
		Type: TypeObject, Key: "",
		Children: []Record{{
			Type: TypeObject, Key: "user",
			Children: []Record{
				{Type: TypeString, Key: "name", Value: "Alice"},
				{Type: TypeNumber, Key: "age", Value: "25"},
			},
		}},
	}
	flat := FlattenBason(root)
	if len(flat) != 2 {
		t.Fatalf("len(flat) = %d", len(flat))
	}
	if flat[0].Key != "user/name" || flat[0].Value != "Alice" {
		t.Errorf("flat[0] = %+v", flat[0])
	}
	if flat[1].Key != "user/age" || flat[1].Value != "25" {
		t.Errorf("flat[1] = %+v", flat[1])
	}
}

func TestFlatten_Array(t *testing.T) {
	record := Record{
		Type: TypeArray, Key: "scores",
		Children: []Record{
			{Type: TypeNumber, Key: "0", Value: "95"},
			{Type: TypeNumber, Key: "1", Value: "87"},
		},
	}
	flat := FlattenBason(record)
	if len(flat) != 2 {
		t.Fatalf("len(flat) = %d", len(flat))
	}
	if flat[0].Key != "scores/0" || flat[1].Key != "scores/1" {
		t.Errorf("keys = %q, %q", flat[0].Key, flat[1].Key)
	}
}

func TestUnflatten_SingleLeaf(t *testing.T) {
	flat := []Record{{Type: TypeString, Key: "name", Value: "Alice"}}
	nested := UnflattenBason(flat)
	if len(nested.Children) != 1 {
		t.Fatalf("len(Children) = %d", len(nested.Children))
	}
	if nested.Children[0].Key != "name" || nested.Children[0].Value != "Alice" {
		t.Errorf("nested.Children[0] = %+v", nested.Children[0])
	}
}

func TestUnflatten_MultipleLeaves(t *testing.T) {
	flat := []Record{
		{Type: TypeString, Key: "name", Value: "Alice"},
		{Type: TypeNumber, Key: "age", Value: "25"},
	}
	nested := UnflattenBason(flat)
	if len(nested.Children) != 2 {
		t.Fatalf("len(Children) = %d", len(nested.Children))
	}
	// Keys are sorted (age < name)
	if nested.Children[0].Key != "age" || nested.Children[1].Key != "name" {
		t.Errorf("children keys = %q, %q", nested.Children[0].Key, nested.Children[1].Key)
	}
}

func TestUnflatten_NestedPaths(t *testing.T) {
	flat := []Record{
		{Type: TypeString, Key: "user/name", Value: "Alice"},
		{Type: TypeNumber, Key: "user/age", Value: "25"},
	}
	nested := UnflattenBason(flat)
	if len(nested.Children) != 1 {
		t.Fatalf("len(Children) = %d", len(nested.Children))
	}
	if nested.Children[0].Key != "user" {
		t.Errorf("child key = %q", nested.Children[0].Key)
	}
	if len(nested.Children[0].Children) != 2 {
		t.Errorf("user children = %d", len(nested.Children[0].Children))
	}
}

func TestUnflatten_ArrayIndices(t *testing.T) {
	flat := []Record{
		{Type: TypeNumber, Key: "scores/0", Value: "95"},
		{Type: TypeNumber, Key: "scores/1", Value: "87"},
	}
	nested := UnflattenBason(flat)
	if len(nested.Children) != 1 {
		t.Fatalf("len(Children) = %d", len(nested.Children))
	}
	if nested.Children[0].Key != "scores" || nested.Children[0].Type != TypeArray {
		t.Errorf("child = %+v", nested.Children[0])
	}
	if len(nested.Children[0].Children) != 2 {
		t.Errorf("scores children = %d", len(nested.Children[0].Children))
	}
}

func TestFlattenUnflatten_RoundTrip(t *testing.T) {
	original := Record{
		Type: TypeObject, Key: "",
		Children: []Record{{
			Type: TypeObject, Key: "user",
			Children: []Record{
				{Type: TypeString, Key: "name", Value: "Alice"},
				{Type: TypeNumber, Key: "age", Value: "25"},
			},
		}},
	}
	flat := FlattenBason(original)
	nested := UnflattenBason(flat)
	if len(nested.Children) != 1 {
		t.Fatalf("len(Children) = %d", len(nested.Children))
	}
	if nested.Children[0].Key != "user" {
		t.Errorf("child key = %q", nested.Children[0].Key)
	}
	if len(nested.Children[0].Children) != 2 {
		t.Errorf("user children = %d", len(nested.Children[0].Children))
	}
}
