#include <regex>
#include "difftest/detail.h"

namespace difftest {

//  Function that can be used to split text using regexp.
std::vector<std::string> regexp_split(const std::string& str, std::string rgx_str)
{
    std::vector<std::string> lst;

    if(!str.empty())  {
        std::regex rgx(rgx_str);
        std::sregex_token_iterator iter(str.begin(), str.end(), rgx, -1);
        std::sregex_token_iterator end;

        while(iter != end) {
            lst.push_back(*iter);
            trim(lst.back());
            ++iter;
        }
    }
    return lst;
    // ????  in "" use as 1 string
}

bool regexp_test(const std::string& str, std::string rgx_str)
{
    std::regex rx(rgx_str);
    return std::regex_match(str , rx);
}

//  Function that can be used to extract tokens using regexp.
std::vector<std::string> regexp_extract(const std::string& str, std::string rgx_str)
{
    std::vector<std::string> lst;
    std::regex rgx(rgx_str);
    std::sregex_token_iterator iter(str.begin(), str.end(), rgx, 0);
    std::sregex_token_iterator end;

    while(iter != end) {
        lst.push_back(*iter);
        trim(lst.back());
        ++iter;
    }
    return lst;
}

//  Function that can be used to replace text using regex.
std::string regexp_replace(const std::string& instr, const std::string& rgx_str, const std::string& replacement )
{
    std::regex re(rgx_str);
    std::string output_str = std::regex_replace(instr, re, replacement);
    return output_str;
}

void replaceall(std::string &str, const std::string &old_part, const std::string &new_part)
{
    size_t posb=0, pos = str.find( old_part ); //rfind( old_part );
    while(pos != std::string::npos) {
        std::string res(str.substr(0, pos));
        res += new_part;
        res += str.substr(pos + old_part.length());
        str = res;
        posb = pos + new_part.length();
        pos = str.find(old_part, posb);
    }
}

} // namespace difftest
