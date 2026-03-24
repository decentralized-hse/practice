#include "strictness.h"
#include "ron64.h"

#include <set>
#include <cctype>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

namespace {

bool IsValidUtf8(const std::string& str)
{
    size_t i = 0;
    while (i < str.size()) {
        uint8_t byte = str[i];
        
        if ((byte & 0x80) == 0) {
            // 1-byte character (ASCII)
            ++i;
        } else if ((byte & 0xE0) == 0xC0) {
            // 2-byte character
            if (i + 1 >= str.size() || (str[i + 1] & 0xC0) != 0x80) {
                return false;
            }
            i += 2;
        } else if ((byte & 0xF0) == 0xE0) {
            // 3-byte character
            if (i + 2 >= str.size() || 
                (str[i + 1] & 0xC0) != 0x80 || 
                (str[i + 2] & 0xC0) != 0x80) {
                return false;
            }
            i += 3;
        } else if ((byte & 0xF8) == 0xF0) {
            // 4-byte character
            if (i + 3 >= str.size() || 
                (str[i + 1] & 0xC0) != 0x80 || 
                (str[i + 2] & 0xC0) != 0x80 ||
                (str[i + 3] & 0xC0) != 0x80) {
                return false;
            }
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

bool IsCanonicalNumber(const std::string& str)
{
    if (str.empty()) {
        return false;
    }

    size_t i = 0;
    
    // Check for leading '+'
    if (str[0] == '+') {
        return false;
    }

    // Handle negative sign
    if (str[0] == '-') {
        ++i;
        if (i >= str.size()) {
            return false;
        }
    }

    // Check for leading zeros (except "0" itself)
    if (str[i] == '0' && i + 1 < str.size() && std::isdigit(str[i + 1])) {
        return false;
    }

    // No scientific notation
    for (char ch : str) {
        if (ch == 'e' || ch == 'E') {
            return false;
        }
    }

    // No trailing decimal point
    if (!str.empty() && str.back() == '.') {
        return false;
    }

    return true;
}

bool IsCanonicalBoolean(const std::string& str)
{
    return str == "true" || str == "false" || str.empty();
}

bool IsCanonicalPath(const std::string& path)
{
    if (path.empty()) {
        return true;
    }

    // No leading slash
    if (path[0] == '/') {
        return false;
    }

    // No trailing slash
    if (path.back() == '/') {
        return false;
    }

    // No consecutive slashes
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        if (path[i] == '/' && path[i + 1] == '/') {
            return false;
        }
    }

    return true;
}

bool IsMinimalRon64(const std::string& str)
{
    if (str.empty()) {
        return false;
    }
    
    // No leading zeros (except "0" itself)
    if (str.size() > 1 && str[0] == '0') {
        return false;
    }

    return true;
}

bool ValidateRecordEncoding(const TBasonRecord& record)
{
    // Check if short form should have been used
    size_t valueLen = 0;
    if (record.Type == EBasonType::Array || record.Type == EBasonType::Object) {
        // For containers, we'd need to encode children to check
        // For simplicity, we skip this check in validation
        return true;
    } else {
        valueLen = record.Value.size();
    }

    return record.Key.size() <= 15 && valueLen <= 15;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

bool ValidateBason(const TBasonRecord& record, uint16_t strictness)
{
    // Bit 0: Shortest encoding
    if (strictness & NStrictness::SHORTEST_ENCODING) {
        if (!ValidateRecordEncoding(record)) {
            return false;
        }
    }

    // Bit 1: Canonical number format
    if (strictness & NStrictness::CANONICAL_NUMBER) {
        if (record.Type == EBasonType::Number) {
            if (!IsCanonicalNumber(record.Value)) {
                return false;
            }
        }
    }

    // Bit 2: Valid UTF-8
    if (strictness & NStrictness::VALID_UTF8) {
        if (!IsValidUtf8(record.Key) || !IsValidUtf8(record.Value)) {
            return false;
        }
    }

    // Bit 3: No duplicate keys (for objects)
    if (strictness & NStrictness::NO_DUPLICATE_KEYS) {
        if (record.Type == EBasonType::Object) {
            std::set<std::string> keys;
            for (const auto& child : record.Children) {
                if (!keys.insert(child.Key).second) {
                    return false;  // Duplicate found
                }
            }
        }
    }

    // Bit 4: Contiguous array indices
    if (strictness & NStrictness::CONTIGUOUS_ARRAY) {
        if (record.Type == EBasonType::Array && !record.Children.empty()) {
            for (size_t i = 0; i < record.Children.size(); ++i) {
                try {
                    uint64_t index = DecodeRon64(record.Children[i].Key);
                    if (index != i) {
                        return false;
                    }
                } catch (...) {
                    return false;
                }
            }
        }
    }

    // Bit 5: Ordered array indices
    if (strictness & NStrictness::ORDERED_ARRAY) {
        if (record.Type == EBasonType::Array && record.Children.size() > 1) {
            for (size_t i = 0; i + 1 < record.Children.size(); ++i) {
                if (record.Children[i].Key >= record.Children[i + 1].Key) {
                    return false;
                }
            }
        }
    }

    // Bit 6: Sorted object keys
    if (strictness & NStrictness::SORTED_OBJECT_KEYS) {
        if (record.Type == EBasonType::Object && record.Children.size() > 1) {
            for (size_t i = 0; i + 1 < record.Children.size(); ++i) {
                if (record.Children[i].Key >= record.Children[i + 1].Key) {
                    return false;
                }
            }
        }
    }

    // Bit 7: Canonical boolean text
    if (strictness & NStrictness::CANONICAL_BOOLEAN) {
        if (record.Type == EBasonType::Boolean) {
            if (!IsCanonicalBoolean(record.Value)) {
                return false;
            }
        }
    }

    // Bit 8: Minimal RON64
    if (strictness & NStrictness::MINIMAL_RON64) {
        if (record.Type == EBasonType::Array) {
            for (const auto& child : record.Children) {
                if (!IsMinimalRon64(child.Key)) {
                    return false;
                }
            }
        }
    }

    // Bit 9: Canonical path format
    if (strictness & NStrictness::CANONICAL_PATH) {
        if (!IsCanonicalPath(record.Key)) {
            return false;
        }
    }

    // Recursive validation for containers
    if (record.Type == EBasonType::Array || record.Type == EBasonType::Object) {
        for (const auto& child : record.Children) {
            if (!ValidateBason(child, strictness)) {
                return false;
            }
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
