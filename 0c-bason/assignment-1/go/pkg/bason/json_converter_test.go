package bason

import (
	"testing"
)

func TestJsonConverter_StringValue(t *testing.T) {
	json := `{"name": "Alice"}`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `{"name":"Alice"}` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_NumberValue(t *testing.T) {
	json := `{"age": 25}`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `{"age":25}` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_BooleanValues(t *testing.T) {
	json := `{"active": true, "deleted": false}`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `{"active":true,"deleted":false}` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_NullValue(t *testing.T) {
	json := `{"value": null}`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `{"value":null}` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_Array(t *testing.T) {
	json := `[1, 2, 3]`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `[1,2,3]` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_NestedObject(t *testing.T) {
	json := `{"user": {"name": "Alice", "age": 25}}`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `{"user":{"name":"Alice","age":25}}` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_ArrayOfObjects(t *testing.T) {
	json := `[{"x": 1}, {"x": 2}]`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `[{"x":1},{"x":2}]` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_EscapeSequences(t *testing.T) {
	json := `{"text": "line1\nline2\ttab"}`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `{"text":"line1\nline2\ttab"}` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_EmptyObject(t *testing.T) {
	json := `{}`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `{}` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_EmptyArray(t *testing.T) {
	json := `[]`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	if result != `[]` {
		t.Errorf("got %q", result)
	}
}

func TestJsonConverter_ComplexExample(t *testing.T) {
	json := `{
        "name": "Alice",
        "age": 25,
        "scores": [95, 87, 92],
        "active": true,
        "address": {
            "city": "NYC",
            "zip": "10001"
        }
    }`
	basonBytes, err := JsonToBason(json)
	if err != nil {
		t.Fatal(err)
	}
	result, err := BasonToJson(basonBytes)
	if err != nil {
		t.Fatal(err)
	}
	bason2, err := JsonToBason(result)
	if err != nil {
		t.Fatal(err)
	}
	if len(bason2) == 0 {
		t.Error("re-parse produced empty bason")
	}
}
