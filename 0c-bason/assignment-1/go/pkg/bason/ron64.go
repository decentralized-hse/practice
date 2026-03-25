package bason

import (
	"errors"
	"fmt"
)

// RON64 alphabet in ASCII order: 0-9A-Z_a-z~
const ron64Alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~"
const ron64Base = 64

var (
	ErrEmptyRon64   = errors.New("empty RON64 string")
	ErrInvalidRon64 = errors.New("invalid RON64 character")
	ErrRon64Overflow = errors.New("RON64 value overflow")
)

func buildDecodeTable(ch byte) int {
	if ch >= '0' && ch <= '9' {
		return int(ch - '0')
	}
	if ch >= 'A' && ch <= 'Z' {
		return 10 + int(ch-'A')
	}
	if ch == '_' {
		return 36
	}
	if ch >= 'a' && ch <= 'z' {
		return 37 + int(ch-'a')
	}
	if ch == '~' {
		return 63
	}
	return -1
}

// EncodeRon64 encodes a non-negative integer to a RON64 string.
// Numbers are encoded in big-endian digit order (most significant first).
func EncodeRon64(value uint64) string {
	if value == 0 {
		return "0"
	}
	var result []byte
	for value > 0 {
		result = append(result, ron64Alphabet[value%ron64Base])
		value /= ron64Base
	}
	// Reverse to get big-endian order
	for i, j := 0, len(result)-1; i < j; i, j = i+1, j-1 {
		result[i], result[j] = result[j], result[i]
	}
	return string(result)
}

// DecodeRon64 decodes a RON64 string to an integer.
// Returns an error on invalid characters or overflow.
func DecodeRon64(str string) (uint64, error) {
	if len(str) == 0 {
		return 0, ErrEmptyRon64
	}
	var result uint64
	for i := 0; i < len(str); i++ {
		digit := buildDecodeTable(str[i])
		if digit < 0 {
			return 0, fmt.Errorf("%w: %q", ErrInvalidRon64, str[i])
		}
		const maxU64 = ^uint64(0)
		if result > maxU64/ron64Base {
			return 0, ErrRon64Overflow
		}
		result = result*ron64Base + uint64(digit)
	}
	return result, nil
}
