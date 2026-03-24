#include "path_utils.h"

#include <sstream>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

std::string JoinPath(const std::vector<std::string>& segments)
{
    if (segments.empty()) {
        return "";
    }

    std::string result;
    for (size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) {
            result += '/';
        }
        result += segments[i];
    }
    return result;
}

std::vector<std::string> SplitPath(const std::string& path)
{
    std::vector<std::string> segments;
    
    if (path.empty()) {
        return segments;
    }

    std::istringstream stream(path);
    std::string segment;
    
    while (std::getline(stream, segment, '/')) {
        if (!segment.empty()) {  // Skip empty segments
            segments.push_back(segment);
        }
    }

    return segments;
}

std::string GetPathParent(const std::string& path)
{
    if (path.empty()) {
        return "";
    }

    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return "";  // No parent
    }

    return path.substr(0, lastSlash);
}

std::string GetPathBasename(const std::string& path)
{
    if (path.empty()) {
        return "";
    }

    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return path;  // The whole path is the basename
    }

    return path.substr(lastSlash + 1);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
