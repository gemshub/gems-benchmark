#include <iostream>
#include "difftest/diffkvasjson.h"
#include "difftest/txtfiles.h"
#include "difftest/detail.h"

namespace difftest {


void KeyValueJsonFile::updateTemplateDiff(const std::string &template_diff_kv)
{
    auto json_data = read_ascii_file(template_diff_kv);
    auto data = nlohmann::json::parse(json_data);

    if( data["key_pattern"].is_string() ) {
        key_regexp = data["key_pattern"];
    }
    if( data["comment_pattern"].is_string() ) {
        comment_regexp = data["comment_pattern"];
    }

    update_compare_method(data);
    update_compare_map(data);
}

bool KeyValueJsonFile::exist()
{
    return path_exist_try_other_ext(file_path, "");
}

bool KeyValueJsonFile::load_all()
{
    json_data.clear();

    if(!exist()) {
        std::cerr <<  "Error: Trying read not existing file  " <<  file_path << "\n";
        return false;
    }

    auto fdata = read_ascii_file(file_path);
    // Internal structure of file data
    auto file_data = load_key_value_pairs(fdata);
    return set_to_json(file_data);
}


std::map<std::string,std::string> KeyValueJsonFile::load_key_value_pairs(const std::string& kv_file_data)
{
    std::map<std::string,std::string> file_data;
    auto fdata = kv_file_data;

    // remove comments
    if(!comment_regexp.empty()) {
        fdata = regexp_replace(fdata, comment_regexp, " ");
    }

    auto headers = regexp_extract(fdata, key_regexp);
    auto datas = regexp_split(fdata, key_regexp);

    auto value = datas.begin()+1;
    for(auto key: headers) {
        trim(key, "<>");
        //std::cout << key << "\n" <<  *value << std::endl;
        if(value < datas.end()) {
            file_data[key] = *value;
            value++;
        }
    }
    return file_data;
}


bool  KeyValueJsonFile::set_to_json(const std::map<std::string,std::string>& file_data)
{
    json_data.clear();
    for(const auto& elem: file_data) {
        auto lval_list = regexp_split(elem.second);
        if(lval_list.size() == 1) {
            json_data[elem.first] = parse_json(lval_list[0]);
        }
        else {
            json_data[elem.first] = nlohmann::json::array();
            for(auto& item:lval_list) {
                json_data[elem.first].push_back(parse_json(item));
            }
        }
    }

    return true;
}

nlohmann::json KeyValueJsonFile::parse_json(std::string& value) const
{
    nlohmann::json jsonval;
    long ival = 0;
    double dval=0.;

    trim(value);

    if(value == "true") {
        jsonval = true;
    }
    else if(value == "false") {
        jsonval = false;
    }
    else if(is<long>(ival, value)) {
        jsonval = ival;
    }
    else if(is<double>(dval, value)) {
        jsonval = dval;
    }
    else {
        trim(value, "'\"");
        jsonval =  value;
    }
    return jsonval;
}

} // namespace jsoniodiff
