#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

namespace difftest {

const double DOUBLE_INFPLUS = 1.79769313486230000E+308;
const double DOUBLE_INFMINUS = -1.79769313486230000E+308;
const double DOUBLE_NAN = 1.797693000000000E+308;
const std::string DOUBLE_INFPLUS_STR = std::to_string(DOUBLE_INFPLUS);
const std::string DOUBLE_INFMINUS_STR = std::to_string(DOUBLE_INFMINUS);
const std::string DOUBLE_NAN_STR = std::to_string(DOUBLE_NAN);

///  Function that can be used to split text using regexp.
std::vector<std::string> regexp_split(const std::string& str, std::string rgx_str = "\\s+");
///  Function that can be used to extract tokens using regexp.
std::vector<std::string> regexp_extract(const std::string& str, std::string rgx_str);
///  Function that can be used to replace text using regex.
std::string regexp_replace(const std::string& instr, const std::string& rgx_str, const std::string& replacement);
///  Returns true whether the string matches the regular expression.
bool regexp_test(const std::string& str, std::string rgx_str);
void replaceall(std::string& str, const std::string& old_part, const std::string& new_part);


/// Read value from string.
template <typename T>
bool is(T& x, const std::string& s)
{
  std::istringstream iss(s);
  return iss >> x && !iss.ignore();
}

/// Replace all characters to character in string (in place).
inline void replace_all(std::string &s, const std::string &characters, char to_character)
{
    std::replace_if(s.begin(), s.end(), [=](char ch) {
        return characters.find_first_of(ch)!=std::string::npos;
    }, to_character);
}

template < class T>
inline bool in_range( const T& x, const T& lower, const T& upper)
{
    return (x >= lower && x <= upper);
}


/// Trim all whitespace characters from start (in place).
inline void ltrim(std::string &s )
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

/// Trim all whitespace characters from end (in place).
inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

/// Trim all whitespace characters from both ends (in place).
inline void trim(std::string &s )
{
    ltrim(s);
    rtrim(s);
}

/// Trim characters from start (in place).
inline void ltrim(std::string &s, const std::string &characters )
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [=](char ch) {
        return characters.find_first_of(ch)==std::string::npos;
    }));
}

/// Trim characters from end (in place).
inline void rtrim(std::string &s, const std::string &characters)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [=](char ch) {
        return characters.find_first_of(ch)==std::string::npos;
    }).base(), s.end());
}

/// Trim characters from both ends (in place).
inline void trim(std::string &s, const std::string &characters )
{
    ltrim(s, characters);
    rtrim(s, characters);
}

} // namespace difftest
