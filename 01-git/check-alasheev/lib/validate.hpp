#pragma once

#include <optional>
#include <stdexcept>
#include <string>

class ValidationError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

void validateDirectory(const std::string& path, const std::string& hash);
