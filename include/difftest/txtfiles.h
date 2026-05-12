#pragma once

#include <string>
#include <vector>

namespace difftest {

/// Get all regular file names from the directory.
std::vector<std::string> files_into_directory(const std::string& directory_path, const std::string& sample = "", bool recursive = false);

/// Read whole ASCII file into string.
std::string read_ascii_file(const std::string& file_path);

/// Checks if the given file status or path corresponds to an existing file or directory.
bool path_exist(const std::string& path);

/// Checks if the given file status or path corresponds to an existing file or try change extension.
bool path_exist_try_other_ext(std::string& path, const std::string& other_ext);


} // namespace difftest
