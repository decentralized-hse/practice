package bason

import (
	"math"
	"testing"
)

func TestEncodeRon64_Zero(t *testing.T) {
	if got := EncodeRon64(0); got != "0" {
		t.Errorf("EncodeRon64(0) = %q, want \"0\"", got)
	}
}

func TestEncodeRon64_SingleDigit(t *testing.T) {
	for _, tc := range []struct{ val uint64; want string }{
		{9, "9"}, {10, "A"}, {36, "_"}, {63, "~"},
	} {
		if got := EncodeRon64(tc.val); got != tc.want {
			t.Errorf("EncodeRon64(%d) = %q, want %q", tc.val, got, tc.want)
		}
	}
}

func TestEncodeRon64_MultiDigit(t *testing.T) {
	for _, tc := range []struct{ val uint64; want string }{
		{64, "10"}, {100, "1_"}, {4095, "~~"}, {4096, "100"},
	} {
		if got := EncodeRon64(tc.val); got != tc.want {
			t.Errorf("EncodeRon64(%d) = %q, want %q", tc.val, got, tc.want)
		}
	}
}

func TestDecodeRon64_Zero(t *testing.T) {
	got, err := DecodeRon64("0")
	if err != nil || got != 0 {
		t.Errorf("DecodeRon64(\"0\") = %d, %v; want 0, nil", got, err)
	}
}

func TestDecodeRon64_SingleDigit(t *testing.T) {
	for _, tc := range []struct{ s string; want uint64 }{
		{"9", 9}, {"A", 10}, {"_", 36}, {"~", 63},
	} {
		got, err := DecodeRon64(tc.s)
		if err != nil || got != tc.want {
			t.Errorf("DecodeRon64(%q) = %d, %v; want %d, nil", tc.s, got, err, tc.want)
		}
	}
}

func TestDecodeRon64_MultiDigit(t *testing.T) {
	for _, tc := range []struct{ s string; want uint64 }{
		{"10", 64}, {"1_", 100}, {"~~", 4095}, {"100", 4096},
	} {
		got, err := DecodeRon64(tc.s)
		if err != nil || got != tc.want {
			t.Errorf("DecodeRon64(%q) = %d, %v; want %d, nil", tc.s, got, err, tc.want)
		}
	}
}

func TestRon64_RoundTrip(t *testing.T) {
	for _, val := range []uint64{0, 1, 10, 63, 64, 100, 1000, 10000, 1000000} {
		enc := EncodeRon64(val)
		dec, err := DecodeRon64(enc)
		if err != nil || dec != val {
			t.Errorf("value %d: enc=%q dec=%d err=%v", val, enc, dec, err)
		}
	}
}

func TestDecodeRon64_InvalidCharacter(t *testing.T) {
	for _, s := range []string{"@", "!", " "} {
		_, err := DecodeRon64(s)
		if err == nil {
			t.Errorf("DecodeRon64(%q) wanted error", s)
		}
	}
}

func TestDecodeRon64_EmptyString(t *testing.T) {
	_, err := DecodeRon64("")
	if err == nil {
		t.Error("DecodeRon64(\"\") wanted error")
	}
}

func TestEncodeRon64_MinimalEncoding(t *testing.T) {
	for val := uint64(0); val <= 10000; val++ {
		enc := EncodeRon64(val)
		if val == 0 {
			if enc != "0" {
				t.Errorf("zero: got %q", enc)
			}
		} else if len(enc) > 0 && enc[0] == '0' {
			t.Errorf("value %d has leading zero: %q", val, enc)
		}
	}
	for _, val := range []uint64{100000, 1000000, math.MaxUint64} {
		enc := EncodeRon64(val)
		if len(enc) == 0 {
			t.Errorf("value %d: empty encoding", val)
		}
		if val != 0 && len(enc) > 0 && enc[0] == '0' {
			t.Errorf("value %d: leading zero in %q", val, enc)
		}
	}
}

func TestRon64_LexicographicOrder(t *testing.T) {
	for a := uint64(0); a < 63; a++ {
		for b := a + 1; b < 64; b++ {
			encA, encB := EncodeRon64(a), EncodeRon64(b)
			if encA >= encB {
				t.Errorf("order: %d (%q) vs %d (%q)", a, encA, b, encB)
			}
		}
	}
	for a := uint64(64); a < 200; a += 7 {
		for b := a + 1; b < 200; b += 7 {
			encA, encB := EncodeRon64(a), EncodeRon64(b)
			if encA >= encB {
				t.Errorf("order: %d (%q) vs %d (%q)", a, encA, b, encB)
			}
		}
	}
}
