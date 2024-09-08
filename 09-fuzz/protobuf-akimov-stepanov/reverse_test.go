package main

import (
	"reflect"
	"testing"
	"unicode/utf8"
)

func FuzzReverse(f *testing.F) {
	testcases := []string{""}
	for _, tc := range testcases {
		f.Add([]byte(tc))
	}
	f.Fuzz(func(t *testing.T, orig []byte) {
		l := len(orig)
		if l != 128 {
			return
		}
		if !utf8.Valid(orig[0:32]) {
			return
		}
		if !utf8.Valid(orig[32:48]) {
			return
		}
		if !utf8.Valid(orig[48:56]) {
			return
		}
		if !utf8.Valid(orig[64:123]) {
			return
		}

		proto := BinToProto(orig)
		new_bin := ProtoToBin(proto)
		if !reflect.DeepEqual(orig, new_bin) {
			t.Errorf("Before: %q, after: %q", orig, new_bin)
		}
	})
}
