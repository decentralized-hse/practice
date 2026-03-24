package bason

import (
	"reflect"
	"testing"
)

func TestJoinPath_Empty(t *testing.T) {
	if got := JoinPath(nil); got != "" {
		t.Errorf("JoinPath(nil) = %q, want \"\"", got)
	}
}

func TestJoinPath_Single(t *testing.T) {
	if got := JoinPath([]string{"foo"}); got != "foo" {
		t.Errorf("JoinPath([\"foo\"]) = %q, want \"foo\"", got)
	}
}

func TestJoinPath_Multiple(t *testing.T) {
	if got := JoinPath([]string{"a", "b", "c"}); got != "a/b/c" {
		t.Errorf("JoinPath = %q, want \"a/b/c\"", got)
	}
	if got := JoinPath([]string{"users", "0", "name"}); got != "users/0/name" {
		t.Errorf("JoinPath = %q, want \"users/0/name\"", got)
	}
}

func TestSplitPath_Empty(t *testing.T) {
	got := SplitPath("")
	if got != nil {
		t.Errorf("SplitPath(\"\") = %v, want nil", got)
	}
}

func TestSplitPath_Single(t *testing.T) {
	got := SplitPath("foo")
	if want := []string{"foo"}; !reflect.DeepEqual(got, want) {
		t.Errorf("SplitPath(\"foo\") = %v, want %v", got, want)
	}
}

func TestSplitPath_Multiple(t *testing.T) {
	got := SplitPath("a/b/c")
	if want := []string{"a", "b", "c"}; !reflect.DeepEqual(got, want) {
		t.Errorf("SplitPath = %v, want %v", got, want)
	}
}

func TestSplitPath_LeadingSlash(t *testing.T) {
	got := SplitPath("/a/b")
	if want := []string{"a", "b"}; !reflect.DeepEqual(got, want) {
		t.Errorf("SplitPath(\"/a/b\") = %v, want %v", got, want)
	}
}

func TestSplitPath_TrailingSlash(t *testing.T) {
	got := SplitPath("a/b/")
	if want := []string{"a", "b"}; !reflect.DeepEqual(got, want) {
		t.Errorf("SplitPath(\"a/b/\") = %v, want %v", got, want)
	}
}

func TestGetPathParent(t *testing.T) {
	for _, tc := range []struct{ path, want string }{
		{"", ""}, {"foo", ""}, {"a/b/c", "a/b"}, {"a/b", "a"},
	} {
		if got := GetPathParent(tc.path); got != tc.want {
			t.Errorf("GetPathParent(%q) = %q, want %q", tc.path, got, tc.want)
		}
	}
}

func TestGetPathBasename(t *testing.T) {
	for _, tc := range []struct{ path, want string }{
		{"", ""}, {"foo", "foo"}, {"a/b/c", "c"}, {"users/0/name", "name"},
	} {
		if got := GetPathBasename(tc.path); got != tc.want {
			t.Errorf("GetPathBasename(%q) = %q, want %q", tc.path, got, tc.want)
		}
	}
}

func TestPath_RoundTrip(t *testing.T) {
	segments := []string{"a", "b", "c", "d"}
	joined := JoinPath(segments)
	split := SplitPath(joined)
	if !reflect.DeepEqual(split, segments) {
		t.Errorf("round trip: got %v, want %v", split, segments)
	}
}
