#include <optional>
#include <string>

/// @returns new root hash if put completed successfully, otherwise returns
/// std::nullopt
std::optional<std::string> GitPut(const std::string& root_hash,
                                  const std::string& file_path,
                                  const std::string& file_content);
