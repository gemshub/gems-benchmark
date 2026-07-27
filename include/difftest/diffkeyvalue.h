#pragma once

#include <nlohmann/json.hpp>
#include "difftest/compare.h"

namespace difftest {

/// KeyValueFile class used to reading key-value file structure
class KeyValueFile
{
public:

    explicit KeyValueFile(const std::string& path, const std::string& template_diff_json = ""):
        file_path(path), key_regexp("<([^>]*)>"), comment_regexp("#([^\\\n]*)\\\n")
    {
        if(!template_diff_json.empty()) {
            updateTemplateDiff( template_diff_json );
        }
    }

    /// Destructor
    virtual ~KeyValueFile()
    {}

    /// Copy constructor
    KeyValueFile(const KeyValueFile &obj) = default;
    /// Move constructor
    KeyValueFile(KeyValueFile &&obj) = default;
    /// Copy assignment
    KeyValueFile &operator =(const KeyValueFile &other) = default;
    /// Move assignment
    KeyValueFile &operator =(KeyValueFile &&other) = default;

    /// Update file location
    void updatePath(const std::string& path)
    {
        file_path = path;
        file_data.clear();
    }

    /// Update structured document containing data to compare.
    virtual void updateTemplateDiff(const std::string& template_diff_json);

    /// Checks if the given path corresponds to an existing file.
    bool exist() const;

    /// Load file to internal structure
    virtual bool load_all();

    /// Compare with template
    virtual bool compare_to(const KeyValueFile& templ, Comparator& comp, std::ostream& out);

    /// Get json data
    virtual std::string json_string() const;


    // addition -------------------------------------------------------

    /// Set up regular expressions for key
    void setKeyRegularExpression( const std::string& key )
    {
        key_regexp = key;
    }

    /// Get regular expressions for key
    const std::string& keyRegularExpression() const
    {
        return key_regexp;
    }

    /// Set up regular expressions for comment
    void setCommentRegularExpression( const std::string& key )
    {
        comment_regexp = key;
    }

    /// Get regular expressions for comment
    const std::string& commentRegularExpression() const
    {
        return comment_regexp;
    }


private:

    /// File location
    std::string file_path;

    /// Regular expression for key
    std::string key_regexp;

    // Regular expression for value or empty
    //std::string value_regexp;

    /// Regular expression for comments or empty
    std::string comment_regexp;

    /// Default numbers compare method
    FloatCompareMethod float_comp;

    /// Internal structure of file data
    std::map<std::string,std::string> file_data;

    /// List/map of data keys (keywords) for which advanced comparison should be done.
    std::map<std::string,FloatCompareMethod> compare_map;
    /// List of data keys (keywords) for which comparison should be ignored.
    std::set<std::string> ignored_keys;

    std::string diff_structured(const std::map<std::string, std::string> &lval,
                                const std::map<std::string, std::string> &rval, Comparator &comp);

    nlohmann::json parse_json(std::string &value) const;

};

} // namespace difftest
