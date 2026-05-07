
#include <iostream>
#include "difftest/diffkeyvalue.h"
#include "difftest/txtfiles.h"
#include "difftest/detail.h"

namespace difftest {

void KeyValueFile::updateTemplateDiff(const std::string &template_diff_json)
{
    auto json_data = read_ascii_file(template_diff_json);

    compare_map.clear();
    ignored_keys.clear();

    auto data = nlohmann::json::parse(json_data);

    if( data["key_pattern"].is_string() ) {
        key_regexp = data["key_pattern"];
    }
    if( data["comment_pattern"].is_string() ) {
        comment_regexp = data["comment_pattern"];
    }

    if( data["eps_number"].is_object() ) {
        if( data["eps_number"]["eps"].is_number() ) {
            float_comp.epsilon = data["eps_number"]["eps"];
        }
        if( data["eps_number"]["compare"].is_string() ) {
            float_comp.method = FloatCompareMethod::get_method( data["eps_number"]["compare"] );
        }
    }

    auto datafields = data["datafields"];
    for(auto& fld: datafields) {
        //std::cout << fld["datakey"]  << " : " << fld["compare"]  << " : " << fld["eps"]  << " : " << std::endl;
        if( fld["ignored"].is_boolean() && fld["ignored"] ) {
            ignored_keys.insert( fld["datakey"].get<std::string>() );
        }

        compare_map[ fld["datakey"] ] = FloatCompareMethod(); // default

        if( fld["eps"].is_number() ) {
            compare_map[ fld["datakey"] ].epsilon = fld["eps"];
        }
        if( fld["compare"].is_string() ) {
            compare_map[ fld["datakey"] ].method = FloatCompareMethod::get_method( fld["compare"] );
        }
    }

    //for( const auto& el: compare_map )
    //  std::cout << el.first << " " << el.second.method << " " << el.second.epsilon << std::endl;
    //for( const auto& el: ignored_keys )
    //  std::cout << el << std::endl;
}

bool KeyValueFile::exist() const
{
    return path_exist(file_path);
}

bool KeyValueFile::load_all()
{
    file_data.clear();

    if(!exist()) {
        std::cerr <<  "Error: Trying read not existing file  " <<  file_path << "\n";
        return false;
    }

    auto fdata = read_ascii_file(file_path);

    // remove comments
    if(!comment_regexp.empty()) {
        fdata = regexp_replace(fdata, comment_regexp, " ");
    }

    auto headers = regexp_extract(fdata, key_regexp);
    auto datas = regexp_split(fdata, key_regexp);

    auto value = datas.begin()+1;
    for(const auto& key: headers) {
        // std::cout << key << "\n" <<  *value << std::endl;
        if(value < datas.end()) {
            file_data[key] = *value;
            value++;
        }
    }
    return true;
}

bool KeyValueFile::compare_to(const KeyValueFile &templ, Comparator &comp)
{
    std::string diff_string;
    if(compare_map.empty() && ignored_keys.empty()) {
        auto the_same = comp.compare(templ.file_data, file_data);
        if(!the_same) {
            diff_string = comp.getDifference();
        }
    }
    else  {
        diff_string = diff_structured(templ.file_data, file_data, comp);
    }

    if(!diff_string.empty()) {
        std::cout <<  "Template file (" << Comparator::templ_name << ") : " <<  templ.file_path << "\n";
        std::cout <<  "Source file (" << Comparator::source_name << ") : " <<  file_path << "\n";
        std::cout <<  "Difference-------------------------------------------------\n";
        std::cout <<  diff_string << "\n\n";
        return false;
    }
    return true;
}

nlohmann::json KeyValueFile::parse_json(std::string& value) const
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

std::string KeyValueFile::json_string() const
{
    nlohmann::json jobj;
    for(const auto& elem: file_data) {
        auto lval_list = regexp_split(elem.second);
        if(lval_list.size() == 1)  {
            jobj[elem.first] = parse_json(lval_list[0]);
        }
        else  {
            jobj[elem.first] = nlohmann::json::array();
            for(auto& item: lval_list) {
                jobj[elem.first].push_back(parse_json(item));
            }
        }
    }
    return jobj.dump(4);
}


// Compare two maps
std::string KeyValueFile::diff_structured(const std::map<std::string,std::string>& lval,
                                          const std::map<std::string,std::string>& rval, Comparator &comp)
{
    FloatCompareMethod def_comp( comp.floatMethod() );
    std::set<std::string> not_used_left;
    std::ostringstream oss;

    auto lval_it = lval.begin();
    auto rval_it = rval.begin();
    while(lval_it != lval.end()) {
        auto key =  lval_it->first;
        auto ignored_it = ignored_keys.find(key);
        if(ignored_it != ignored_keys.end()) {
            ++lval_it;
            continue;
        }

        rval_it = rval.find(key);
        if(rval_it != rval.end()) {
            auto compf_it =  compare_map.find(key);
            if(compf_it != compare_map.end()) {
                comp.setEpsilon( compf_it->second );
            }
            else {
                comp.setEpsilon( float_comp );
            }

            auto ret_diff = !comp.compare(lval_it->second, rval_it->second);
            if(ret_diff)  {
                oss <<  key << ": " <<  comp.getDifference() << " \n";
                // Think about recursion arrays => use yaml?
            }
        }
        else {
            not_used_left.insert(key);
        }
        ++lval_it;
    }

    if(!not_used_left.empty()) {
        oss << comp.keys_diff_string(true, not_used_left);
    }

    std::set<std::string> not_used_right;
    if(comp.method() == Comparator::Difference) {
        auto rval_it = rval.begin();
        while(rval_it != rval.end()) {
            lval_it = lval.find(rval_it->first);
            if(lval_it == lval.end()) {
                not_used_right.insert(rval_it->first);
            }
            ++rval_it;
        }

        if(!not_used_right.empty()) {
            oss << comp.keys_diff_string(false, not_used_right);
        }
    }

    comp.setEpsilon(def_comp);
    return oss.str();
}

} // namespace difftest
