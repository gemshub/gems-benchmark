#pragma once

#include "difftest/diffjson.h"

namespace difftest {

/// KeyValueJsonFile class used to reading key-value file to internal json structure
class KeyValueJsonFile: public JsonFile
{
public:

    explicit KeyValueJsonFile(const std::string& path, const std::string& template_diff_kv = ""):
        JsonFile(path), key_regexp("<([^>]*)>"), comment_regexp("#([^\\\n]*)\\\n")
    {
        if(!template_diff_kv.empty()) {
            updateTemplateDiff(template_diff_kv);
        }
    }

    /// Destructor
    virtual ~KeyValueJsonFile()
    {}

    /// Copy assignment
    KeyValueJsonFile &operator =(const KeyValueJsonFile &other) = default;
    /// Move assignment
    KeyValueJsonFile &operator =(KeyValueJsonFile &&other) = default;

    /// Update structured document containing data to compare.
    void updateTemplateDiff(const std::string& template_diff_json) override;

    /// Checks if the given path corresponds to an existing file.
    bool exist() override;

    /// Load file to internal structure
    bool load_all() override;

    // addition -------------------------------------------------------

    /// Set up regular expressions for key
    void setKeyRegularExpression(const std::string& key)
    {
        key_regexp = key;
    }

    /// Get regular expressions for key
    const std::string& keyRegularExpression() const
    {
        return key_regexp;
    }

    /// Set up regular expressions for comment
    void setCommentRegularExpression(const std::string& key)
    {
        comment_regexp = key;
    }

    /// Get regular expressions for comment
    const std::string& commentRegularExpression() const
    {
        return comment_regexp;
    }

private:

    /// Regular expression for key
    std::string key_regexp;

    /// Regular expression for comments or empty
    std::string comment_regexp;

    std::map<std::string, std::string> load_key_value_pairs(const std::string &kv_file_data);
    bool set_to_json(const std::map<std::string,std::string>& file_data);
    nlohmann::json parse_json(std::string &value) const;

};

} // namespace difftest
