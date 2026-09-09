
#include <iostream>
#include "difftest/diffjson.h"
#include "difftest/txtfiles.h"
#include "difftest/detail.h"

namespace difftest {



void JsonFile::updateTemplateDiff(const Comparator &template_diff)
{
    json_comp = template_diff;
    float_comp = template_diff.floatMethod();
}

void JsonFile::updateTemplateDiff( const std::string &template_diff_json )
{
    auto json_data = read_ascii_file(template_diff_json);
    auto data = nlohmann::json::parse(json_data);

    update_compare_method(data);
    update_compare_map(data);
}


void JsonFile::update_compare_method(const nlohmann::json &data)
{
    if(data["eps_number"].is_object()) {
        if( data["eps_number"]["eps"].is_number() ) {
            float_comp.epsilon = data["eps_number"]["eps"];
        }
        if( data["eps_number"]["compare"].is_string() ) {
            float_comp.method = FloatCompareMethod::get_method( data["eps_number"]["compare"] );
        }
    }

}

void JsonFile::update_compare_map(const nlohmann::json &data)
{
    compare_map.clear();
    ignored_keys.clear();

    auto datafields = data["datafields"];
    for(auto& fld: datafields)  {
        if( fld["ignored"].is_boolean() && fld["ignored"] ) {
            ignored_keys.insert( fld["datakey"].get<std::string>() );
        }

        compare_map[ fld["datakey"] ] = FloatCompareMethod(); // default

        if( fld["eps"].is_number() ) {
            compare_map[ fld["datakey"] ].epsilon = fld["eps"];
        }
        if( fld["compare"].is_string() ) {
            compare_map[ fld["datakey"] ].method = FloatCompareMethod::get_method(fld["compare"]);
        }
    }
}

bool JsonFile::exist()
{
    return path_exist_try_other_ext(file_path, ".json");
}


bool JsonFile::load_all()
{
    json_data.clear();

    if(!exist()) {
        std::cerr <<  "Error: Trying read not existing file  " <<  file_path << "\n";
        return false;
    }

    auto fdata = read_ascii_file(file_path);
    replaceall( fdata, "-inf", DOUBLE_INFMINUS_STR);
    replaceall( fdata, "inf", DOUBLE_INFPLUS_STR);
    replaceall( fdata, "nan", DOUBLE_NAN_STR);
    json_data = nlohmann::json::parse(fdata);

    return true;
}

bool JsonFile::compare_to(const JsonFile &templ, const Comparator &comp, std::ostream& out)
{
    json_comp = comp;
    std::string diff_string =  diff_json(templ.json_data, json_data);

    out <<  "\n-------------------------------------------------------------\n";
    out <<  "Template file (" << Comparator::templ_name << ") : " <<  templ.file_path << "\n";
    out <<  "Source file (" << Comparator::source_name << ") : " <<  file_path << "\n";

    if(!diff_string.empty()) {
        out <<  "Difference -------------------------------------------------\n";
        out <<  diff_string << "\n\n";
        return false;
    }
    else {
        out <<  "No Difference ------------------------------------------------\n";
        return true;
    }
}

std::string JsonFile::diff_json(const nlohmann::json &lval, const nlohmann::json &rval)
{
    std::string oss;

    if(lval.is_number() &&  rval.is_number())  {
        bool ret_diff = !json_comp.compare(lval.get<double>(), rval.get<double>());
        if(ret_diff) {
            oss = json_comp.getDifference();
        }
    }
    else if(lval.is_primitive() &&  rval.is_primitive()) {
        bool ret_diff = !json_comp.compare(lval.get<std::string>(), rval.get<std::string>());
        if(ret_diff) {
            oss =  json_comp.getDifference();
        }
    }
    else if(lval.is_array() &&  rval.is_array()) {
        oss = diff_array(lval, rval);
    }
    else if(lval.is_object() &&  rval.is_object()) {
        oss = diff_object(lval, rval);
    }
    else  {
        oss = "Different object types";
    }
    return oss;
}

std::string JsonFile::diff_array(const nlohmann::json &lval, const nlohmann::json &rval)
{
    int ii = 0;
    std::ostringstream iss;

    // If the arrays differ in size, report the size mismatch instead of diffing elementwise.
    if(lval.size() > rval.size() ||
        ( lval.size() < rval.size() && json_comp.method() == Comparator::Difference )) {
        return  json_comp.size_diff_string(lval.size(), rval.size());
    }

    auto lval_it = lval.begin();
    auto rval_it = rval.begin();
    while(lval_it != lval.end() && rval_it != rval.end()) {
        auto ret_diff = diff_json(*lval_it, *rval_it);
        if(!ret_diff.empty()) {
            iss <<  ii << ": " << ret_diff << std::endl;
        }
        ++lval_it;
        ++rval_it;
        ++ii;
    }
    return iss.str();
}

std::string JsonFile::diff_object(const nlohmann::json &lval, const nlohmann::json &rval)
{
    std::set<std::string> not_used_left;
    std::ostringstream oss;

    auto lval_it = lval.begin();
    auto rval_it = rval.begin();
    while(lval_it != lval.end()) {
        auto key =  lval_it.key();
        auto ignored_it =  ignored_keys.find(key);
        if(ignored_it !=  ignored_keys.end()) {
            ++lval_it;
            continue;
        }

        rval_it = rval.find( key );
        if(rval_it != rval.end()) {
            auto compf_it =  compare_map.find(key);
            if(compf_it != compare_map.end()) {
                json_comp.setEpsilon(compf_it->second);
            }
            else {
                json_comp.setEpsilon(float_comp);
            }

            auto ret_diff= diff_json(lval_it.value(), rval_it.value());
            if(!ret_diff.empty()) {
                oss <<  key << ": " <<  ret_diff << " \n";
            }
        }
        else {
            not_used_left.insert(key);
        }
        ++lval_it;
    }

    if(!not_used_left.empty()) {
        oss << json_comp.keys_diff_string(true, not_used_left);
    }

    std::set<std::string> not_used_right;
    if(json_comp.method() == Comparator::Difference) {
        auto rval_it = rval.begin();
        while(rval_it != rval.end()) {
            lval_it = lval.find(rval_it.key());
            if(lval_it == lval.end()) {
                not_used_right.insert(rval_it.key());
            }
            ++rval_it;
        }

        if(!not_used_right.empty()) {
            oss << json_comp.keys_diff_string( false, not_used_right );
        }
    }
    return oss.str();
}

} // namespace difftest
