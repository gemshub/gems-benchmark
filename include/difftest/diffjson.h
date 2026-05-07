#pragma once

#include <nlohmann/json.hpp>
#include "difftest/compare.h"

namespace difftest {

/// JsonFile class used to reading json file structure
class JsonFile
{
public:

    explicit JsonFile(const std::string& path, const std::string& template_diff_json = ""):
        file_path(path)
    {
        if(!template_diff_json.empty()) {
            updateTemplateDiff(template_diff_json);
        }
    }

    /// Destructor
    virtual ~JsonFile()
    {}

    /// Copy assignment
    JsonFile &operator =( const JsonFile &other) = default;
    /// Move assignment
    JsonFile &operator =( JsonFile &&other) = default;

    /// Update file location
    void updatePath(const std::string& path)
    {
        file_path = path;
        json_data.clear();
    }

    /// Update structured document containing data to compare.
    virtual void updateTemplateDiff(const Comparator& template_diff);

    /// Update structured document containing data to compare.
    virtual void updateTemplateDiff(const std::string& template_diff_json);

    /// Checks if the given path corresponds to an existing file.
    virtual bool exist();

    /// Load file to internal structure
    virtual bool load_all();

    /// Compare with template
    virtual bool compare_to(const JsonFile& templ, const Comparator& comp);

    /// Get json data
    virtual std::string json_string() const
    {
        return json_data.dump(4);
    }

protected:

    /// File location
    std::string file_path;

    /// Default numbers compare method
    FloatCompareMethod float_comp;

    /// Internal structure of file data
    nlohmann::json json_data;

    /// List/map of data keys (keywords) for which advanced comparison should be done.
    std::map<std::string,FloatCompareMethod> compare_map;
    /// List of data keys (keywords) for which comparison should be ignored.
    std::set<std::string> ignored_keys;

    /// Work copy of comparator
    Comparator json_comp;


    std::string diff_json(const nlohmann::json &lval, const nlohmann::json &rval);
    std::string diff_array(const nlohmann::json &lval, const nlohmann::json &rval);
    std::string diff_object(const nlohmann::json &lval, const nlohmann::json &rval);

    void update_compare_method(const nlohmann::json &val_diff_json);
    void update_compare_map(const nlohmann::json &data);

};

} // namespace difftest
