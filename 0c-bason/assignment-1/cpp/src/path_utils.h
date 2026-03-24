#pragma once

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

// Join path segments with '/' separator
std::string JoinPath(const std::vector<std::string>& segments);

// Split a path into segments
std::vector<std::string> SplitPath(const std::string& path);

// Return the parent path ("a/b/c" → "a/b")
// Empty string for top-level keys
std::string GetPathParent(const std::string& path);

// Return the last segment ("a/b/c" → "c")
std::string GetPathBasename(const std::string& path);

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
