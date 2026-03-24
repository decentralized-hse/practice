package bason

import "strings"

// JoinPath joins path segments with '/' separator.
func JoinPath(segments []string) string {
	if len(segments) == 0 {
		return ""
	}
	return strings.Join(segments, "/")
}

// SplitPath splits a path into segments (by '/').
func SplitPath(path string) []string {
	if path == "" {
		return nil
	}
	var segments []string
	for _, part := range strings.Split(path, "/") {
		if part != "" {
			segments = append(segments, part)
		}
	}
	return segments
}

// GetPathParent returns the parent path ("a/b/c" → "a/b").
// Returns empty string for top-level keys.
func GetPathParent(path string) string {
	if path == "" {
		return ""
	}
	i := strings.LastIndex(path, "/")
	if i < 0 {
		return ""
	}
	return path[:i]
}

// GetPathBasename returns the last segment ("a/b/c" → "c").
func GetPathBasename(path string) string {
	if path == "" {
		return ""
	}
	i := strings.LastIndex(path, "/")
	if i < 0 {
		return path
	}
	return path[i+1:]
}
