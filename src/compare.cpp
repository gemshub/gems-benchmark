#include "difftest/compare.h"
#include "difftest/detail.h"

namespace difftest {

std::string Comparator::templ_name = "left";
std::string Comparator::source_name = "right";

std::string Comparator::size_diff_string(size_t lval_size, size_t rval_size) const
{
    std::ostringstream oss;
    oss <<  "The number of values is different: " <<
        templ_name << " = " << lval_size << " " << source_name << " = " << rval_size;
    return oss.str();
}

std::string Comparator::keys_diff_string(bool is_left, std::set<std::string> keys) const
{
    std::ostringstream oss;
    oss << "Present only in ";
    if(is_left) {
        oss << templ_name << ":";
    } else {
        oss << source_name << ":";
    }

    for(const auto& el: keys) {
        oss << " " << el;
    }
    oss << "\n";
    return oss.str();
}

bool Comparator::diff_simple(const std::string &lval, const std::string &rval, std::ostream &out)
{
    bool ret = false;

    if(lval != rval) {
        double ldbl, rdbl;
        ret = true;

        if(is(ldbl, lval) && is(rdbl, rval)) {
            std::ostringstream iss;
            ret = diff(ldbl, rdbl, iss );
        }
        if(ret) {
            out <<  value_diff_string(lval, rval);
        }
    }
    return ret;
}

bool Comparator::diff(const std::string &lval, const std::string &rval, std::ostream &out)
{
    if(  lval == rval  )
        return false;

    auto lval_list = regexp_split(lval);
    auto rval_list = regexp_split(rval);

    if(lval_list.size() == 1 || rval_list.size() == 1) {
        return diff_simple(lval_list[0], rval_list[0], out);
    } else {
        // output string as array
        return diff( lval_list, rval_list, out );
    }
}

FloatCompareMethod::FloatMethod FloatCompareMethod::get_method(const std::string &method_name)
{
    FloatCompareMethod::FloatMethod method = FloatCompareMethod::RelDiffMore;

    if(method_name == "sig_dig_less") {
        method = FloatCompareMethod::SigDigLess;
    }
    else if(method_name == "rel_def_more") {
        method = FloatCompareMethod::RelDiffMore;
    }
    else if(method_name == "apr_def_more") {
        method = FloatCompareMethod::AprDiffMore;
    }
    else if(method_name == "abs_def_more") {
        method = FloatCompareMethod::AbsDiffMore;
    }
    else if(method_name == "log_dig_less") {
        method = FloatCompareMethod::LogDigLess;
    }

    return method;
}

} // namespace difftest
