#pragma once

#include <memory>
#include <iostream>
#include "difftest/diffjson.h"

namespace difftest {

/// Implementation of extracting data from a string of arguments
/// and organizing comparisons of files or directories.
class ComparisonPerformer
{
public:

    /// These are common commands used in various situations
    enum MainCommands {
        Help,
        CopmpareFiles,
        CompareDirectories
    };

    /// These are compare file types
    enum MainTypes {
        fJSON,
        fKeyValue,
        fUndef
    };

    /// Constructor
    explicit  ComparisonPerformer(int argc, char* argv[]);
    /// Destructor
    virtual ~ComparisonPerformer()
    {}

    /// Execute command
    virtual int execute_command();

protected:

    /// Command to be execute
    MainCommands command;
    /// Recursively compare any subdirectories found
    bool use_recursion = false;

    /// Template file name in a directory to compare
    std::string file_name_templ;

    /// Template file or a directory to compare
    std::string templ_path;

    /// Source file or a directory to compare
    std::string source_path;

    /// Structured document containing data to compare.
    std::string template_diff_json;

    /// Comparator used to compare two values
    Comparator compare_method;

    /// Loaded template  file format
    MainTypes templ_type = fUndef;
    /// Loaded template  - a json  or key-value format file
    std::shared_ptr<JsonFile> templ_file;

    /// Loaded source file format
    MainTypes source_type = fUndef;
    /// Loaded source  - a json  or key-value format file
    std::shared_ptr<JsonFile> source_file;

    virtual void show_usage(const std::string& name);
    virtual bool compare_files(const std::string& ftempl, const std::string& fsource, std::ostream& out=std::cout);
    virtual bool compare_dirs(const std::string& ftempl, const std::string& fsource);
    virtual int extract_args(int argc, char *argv[]);
    virtual void set_path(MainTypes type, const char *path);
    MainTypes file_type(const std::string &file, MainTypes def_type);
    bool update_file(MainTypes ftype, const std::string &path, std::shared_ptr<JsonFile> &file);
};

} // namespace difftest
