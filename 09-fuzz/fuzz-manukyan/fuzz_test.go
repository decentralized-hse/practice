package main

import (
	"encoding/json"
	"strings"
	"testing"
)

func FuzzJSONConversion(f *testing.F) {
	testCases := []string{
		`{"name": "John Doe", "login": "jdoe", "group": "CS101", "practice": [0, 1, 0, 1, 0, 1, 0, 1], "project": {"repository": "https://example.com/repo.git", "mark": 5}, "mark": 4.5}`,
		`{"name": "", "login": "", "group": "", "practice": [0, 0, 0, 0, 0, 0, 0, 0], "project": {"repository": "", "mark": 0}, "mark": 0}`,
		`{"name": 123, "login": true}`,
	}

	for _, tc := range testCases {
		f.Add(tc)
	}

	f.Fuzz(func(t *testing.T, rawJSON string) {
		var student Student

		err := json.Unmarshal([]byte(rawJSON), &student)
		if err != nil {
			t.Logf("Unmarshal failed as expected with error: %v", err)
			return
		}

		// Re-marshal and compare with the original JSON
		marshaledJSON, err := json.Marshal(student)
		if err != nil {
			switch {
			case strings.Contains(err.Error(), "должена иметь значение либо 0, либо 1"):
				t.Logf("Expected validation error for practice field: %v", err)

			case strings.Contains(err.Error(), "Поле Group должно быть валидной utf8 строкой"):
				t.Logf("Expected validation error for group field: %v", err)

			case strings.Contains(err.Error(), "Поле Repo должно быть валидной utf8 строкой"):
				t.Logf("Expected validation error for Repo field: %v", err)

			case strings.Contains(err.Error(), "Поле Name должно быть валидной utf8 строкой"):
				t.Logf("Expected validation error for Name field: %v", err)

			default:
				t.Errorf("Unexpected error during marshaling: %v", err)
			}
			return
		}

		var unmarshaledAgain Student
		err = json.Unmarshal(marshaledJSON, &unmarshaledAgain)
		if err != nil {
			t.Errorf("Re-unmarshal failed: %v", err)
			return
		}

		// Validate the whole struct
		if student != unmarshaledAgain {
			t.Errorf("Mismatch after re-unmarshal: got %+v, want %+v", unmarshaledAgain, student)
		}
	})
}
