#include <sstream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include "difftest/txtfiles.h"

namespace difftest {

std::string make_path(const std::string& dir, const std::string& name, const std::string& ext)
{
    std::string Path(dir);
    if(dir != "") {
        Path += "/";
    }
    Path += name + "." + ext;
    return Path;
}

/// Creates the directory path.
bool create_directory(const std::string& path)
{
    fs::path ps(path);
    return fs::create_directories(ps);
}

/// Copies a directory, the subdirectories are also copied, with their content, recursively.
/// \param source - 	path to the source directory
///        target - 	path to the target directory
void copy_directory(const std::string& source, const std::string& target)
{
    fs::path from(source), to(target);
    fs::copy(from, to, fs::copy_options::recursive);
}

/// Copies a single file.
/// \param source - 	path to the source file
///        target - 	path to the target file
void copy_file(const std::string& source, const std::string& target)
{
    fs::path from(source), to(target);
    fs::copy_file(from, to, fs::copy_options::overwrite_existing);
}


// Get all  regular file names from the directory.
std::vector<std::string> files_into_directory(const std::string& directory_path, const std::string& sample)
{
    std::vector<std::string> file_names;

    if(!directory_path.empty()) {
        fs::path ps(directory_path);
        if(fs::exists(ps)) {
            for(auto& p: fs::directory_iterator(ps)) {
                if(fs::is_regular_file(p.path())) {
                    std::string file = p.path().string();
                    //cout << "file = " << file << endl;
                    if(sample.empty() || file.find(sample) != std::string::npos) {
                        file_names.push_back(file);
                    }
                }
            }
        }
    }
    return file_names;
}

// Read whole ASCII file into string.
std::string read_ascii_file(const std::string& file_path)
{
    std::ifstream t(file_path);
    /// JARANGO_THROW_IF( !t.good(), "filesystem", 4, "file open error...  " + file_path );
    std::stringstream buffer;
    buffer << t.rdbuf();

    auto retstr = buffer.str();
    // skip over optional BOM http://unicode.org/faq/utf_bom.html
    if(retstr.size() >= 3 && static_cast<uint8_t>(retstr[0]) == 0xef &&
        static_cast<uint8_t>(retstr[1]) == 0xbb &&
        static_cast<uint8_t>(retstr[2]) == 0xbf) {
        // found UTF-8 BOM. simply skip over it
        retstr = retstr.substr(3);
    }
    return retstr;
}

bool path_exist(const std::string& path)
{
    fs::path ps(path);
    return fs::exists(ps);
}


bool path_exist_try_other_ext(std::string& path, const std::string& other_ext)
{
    fs::path ps(path);
    bool exist_f= false;
    if(!other_ext.empty()) {
        ps.replace_extension(other_ext);
        exist_f = fs::exists(ps);
        if(exist_f) {
            path = ps.string();
        }
    } else {
        exist_f = fs::exists(ps);
        if(!exist_f) {
            auto all_files = files_into_directory(ps.parent_path().string(), ps.stem().string());
            if(all_files.size() >0) {
                exist_f = fs::exists(all_files[0]);
                path = all_files[0];
            }
        }
    }
    return exist_f;
}

} // namespace difftest
