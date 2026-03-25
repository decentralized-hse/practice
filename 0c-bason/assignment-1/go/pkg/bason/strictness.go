package bason

import (
	"unicode/utf8"
)

// Strictness bitmask bits
const (
	StrictnessShortestEncoding  uint16 = 0x001 // Bit 0
	StrictnessCanonicalNumber   uint16 = 0x002 // Bit 1
	StrictnessValidUTF8         uint16 = 0x004 // Bit 2
	StrictnessNoDuplicateKeys   uint16 = 0x008 // Bit 3
	StrictnessContiguousArray   uint16 = 0x010 // Bit 4
	StrictnessOrderedArray      uint16 = 0x020 // Bit 5
	StrictnessSortedObjectKeys  uint16 = 0x040 // Bit 6
	StrictnessCanonicalBoolean  uint16 = 0x080 // Bit 7
	StrictnessMinimalRon64      uint16 = 0x100 // Bit 8
	StrictnessCanonicalPath     uint16 = 0x200 // Bit 9
	StrictnessNoMixing          uint16 = 0x400 // Bit 10

	StrictnessPermissive uint16 = 0x000
	StrictnessStandard   uint16 = 0x1FF // Bits 0-8
	StrictnessStrict     uint16 = 0x7FF // Bits 0-10
)

func isValidUTF8(str string) bool {
	return utf8.ValidString(str)
}

func isCanonicalNumber(str string) bool {
	if len(str) == 0 {
		return false
	}
	i := 0
	if str[0] == '+' {
		return false
	}
	if str[0] == '-' {
		i++
		if i >= len(str) {
			return false
		}
	}
	if str[i] == '0' && i+1 < len(str) && isDigit(str[i+1]) {
		return false
	}
	for _, r := range str {
		if r == 'e' || r == 'E' {
			return false
		}
	}
	if len(str) > 0 && str[len(str)-1] == '.' {
		return false
	}
	return true
}

func isDigit(b byte) bool {
	return b >= '0' && b <= '9'
}

func isCanonicalBoolean(str string) bool {
	return str == "true" || str == "false" || str == ""
}

func isCanonicalPath(path string) bool {
	if path == "" {
		return true
	}
	if path[0] == '/' {
		return false
	}
	if path[len(path)-1] == '/' {
		return false
	}
	for i := 0; i+1 < len(path); i++ {
		if path[i] == '/' && path[i+1] == '/' {
			return false
		}
	}
	return true
}

func isMinimalRon64(str string) bool {
	if len(str) == 0 {
		return false
	}
	if len(str) > 1 && str[0] == '0' {
		return false
	}
	return true
}

func validateRecordEncoding(record Record) bool {
	if record.Type == TypeArray || record.Type == TypeObject {
		return true
	}
	return len(record.Key) <= 15 && len(record.Value) <= 15
}

// ValidateBason validates a record against a strictness bitmask.
// Returns true if the record conforms to all enabled rules.
func ValidateBason(record Record, strictness uint16) bool {
	if strictness&StrictnessShortestEncoding != 0 {
		if !validateRecordEncoding(record) {
			return false
		}
	}
	if strictness&StrictnessCanonicalNumber != 0 {
		if record.Type == TypeNumber {
			if !isCanonicalNumber(record.Value) {
				return false
			}
		}
	}
	if strictness&StrictnessValidUTF8 != 0 {
		if !isValidUTF8(record.Key) || !isValidUTF8(record.Value) {
			return false
		}
	}
	if strictness&StrictnessNoDuplicateKeys != 0 {
		if record.Type == TypeObject {
			seen := make(map[string]bool)
			for _, child := range record.Children {
				if seen[child.Key] {
					return false
				}
				seen[child.Key] = true
			}
		}
	}
	if strictness&StrictnessContiguousArray != 0 {
		if record.Type == TypeArray && len(record.Children) > 0 {
			for i, child := range record.Children {
				index, err := DecodeRon64(child.Key)
				if err != nil || index != uint64(i) {
					return false
				}
			}
		}
	}
	if strictness&StrictnessOrderedArray != 0 {
		if record.Type == TypeArray && len(record.Children) > 1 {
			for i := 0; i+1 < len(record.Children); i++ {
				if record.Children[i].Key >= record.Children[i+1].Key {
					return false
				}
			}
		}
	}
	if strictness&StrictnessSortedObjectKeys != 0 {
		if record.Type == TypeObject && len(record.Children) > 1 {
			for i := 0; i+1 < len(record.Children); i++ {
				if record.Children[i].Key >= record.Children[i+1].Key {
					return false
				}
			}
		}
	}
	if strictness&StrictnessCanonicalBoolean != 0 {
		if record.Type == TypeBoolean {
			if !isCanonicalBoolean(record.Value) {
				return false
			}
		}
	}
	if strictness&StrictnessMinimalRon64 != 0 {
		if record.Type == TypeArray {
			for _, child := range record.Children {
				if !isMinimalRon64(child.Key) {
					return false
				}
			}
		}
	}
	if strictness&StrictnessCanonicalPath != 0 {
		if !isCanonicalPath(record.Key) {
			return false
		}
	}
	if record.Type == TypeArray || record.Type == TypeObject {
		for _, child := range record.Children {
			if !ValidateBason(child, strictness) {
				return false
			}
		}
	}
	return true
}
