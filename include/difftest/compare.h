#pragma once

#include <string>
#include <sstream>
#include <set>
#include <cmath>
#include <limits>
#include <iomanip>

#include "difftest/type_test.h"

namespace difftest {

// https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison

template<typename T>
bool approximatelyEqual( const T& a, const T& b, const T& epsilon = std::numeric_limits<T>::epsilon() )
{
    return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

template<typename T>
bool essentiallyEqual( const T& a, const T& b, const T& epsilon = std::numeric_limits<T>::epsilon() )
{
    return fabs(a - b) <= ( (fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

template<typename T>
bool definitelyGreaterThan( const T& a, const T& b, const T& epsilon = std::numeric_limits<T>::epsilon() )
{
    return (a - b) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

template<typename T>
bool definitelyLessThan( const T& a, const T& b, const T& epsilon = std::numeric_limits<T>::epsilon() )
{
    return (b - a) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}


/// Class to compare float-type numbers
struct FloatCompareMethod
{

    /// Possible compare methods for all float-type numbers
    enum FloatMethod {
        /// Indicate difference if the absolute difference of compared numbers n1 > n2
        /// is greater than a product of n1 by the indicated "eps" value (i.e. not "essentially equal")
        RelDiffMore,
        /// Indicate difference if the absolute difference of compared numbers n1 < n2
        /// is greater than a product of n1 by the indicated "eps" value (i.e. not "approximately equal")
        AprDiffMore,
        /// Indicate difference if the absolute difference is greater than a  "eps" value (i.e. not " absolute tolerance")
        AbsDiffMore,
        /// Indicate difference if the number of identical significant digits is less than given as value
        SigDigLess,
        /// Indicate difference if the number of identical significant digits of log is less than given as value
        LogDigLess
    };

    static FloatCompareMethod::FloatMethod get_method(const std::string &method_name);

    /// Method comparing the float-type numbers
    FloatMethod  method = RelDiffMore;
    /// Epsilon/difference value comparing the float-type numbers
    double  epsilon = std::numeric_limits<double>::epsilon();

    explicit FloatCompareMethod(FloatMethod amethod = RelDiffMore,
                                double anepsilon = std::numeric_limits<double>::epsilon()):
        method(amethod), epsilon(anepsilon)
    {}

    /// Compare all fundamental floating types (along with their aliases)
    template <class T,
             std::enable_if_t<std::is_floating_point<T>::value, int> = 0 >
    bool diff(const T& lval, const T& rval)
    {
        bool ret_diff = false;

        switch(method) {
        case FloatCompareMethod::RelDiffMore:
            ret_diff = !essentiallyEqual(lval, rval, epsilon);
            break;
        case FloatCompareMethod::AprDiffMore:
            ret_diff = !approximatelyEqual(lval, rval, epsilon);
            break;
        case FloatCompareMethod::AbsDiffMore:
            ret_diff = !(fabs(lval-rval) < epsilon);
            break;
        case FloatCompareMethod::SigDigLess:
            ret_diff = diff_sign_dig_less(lval, rval);
            break;
        case FloatCompareMethod::LogDigLess:
            if(lval > 0. && rval> 0.) {
                ret_diff =  diff_sign_dig_less(log(lval), log(rval));
            } else {
                ret_diff = diff_sign_dig_less(lval, rval);
            }
            break;

        }
        return ret_diff;
    }

    template <class T,
             std::enable_if_t<std::is_floating_point<T>::value, int> = 0 >
    bool diff_sign_dig_less(const T& lval, const T& rval )
    {
        int n = static_cast<int>( epsilon );
        auto ld = dtond(lval, n);
        auto rd = dtond(rval, n);
        return  !essentiallyEqual(ld, rd);

    }

    /// Converting double to n significant digits precision (2<n<14)
    /// If no second parameter given, n=7 as float is assumed
    static double dtond(const double d, const int n = 7)
    {
        if(d == 0.0 || n > 13 || n < 2) {
            return d;
        }
        double f = pow(10.0, n - ceil(log10(fabs(d))));
        return round(d * f) / f;
    }

};


/// Comparator class used to compare two values, strings,
/// vector-like objects (std::list, std::vector, std::set, etc)
/// or map-like objects (std::map, std::unordered_map, etc).
///
/// Any string split by space symbols and compare as string array.
/// If both strings contain numbers, then we compare two double values.
class Comparator final
{

public:

    static std::string templ_name;
    static std::string source_name;

    /// Possible compare methods
    enum CompareMethod {
        /// Output all differences between template and source
        Difference,
        /// Output all differences source consist in template
        ConsistIn
    };

    explicit Comparator(CompareMethod method=Difference):
        compare_method(method)
    { }

    /// Copy assignment
    Comparator &operator =(const Comparator &other)
    {
        compare_method = other.compare_method;
        float_method = other.float_method;
        return *this;
    }

    /// Set up method comparing the float-type numbers
    void setEpsilon(double epsilon, FloatCompareMethod::FloatMethod method = FloatCompareMethod::RelDiffMore)
    {
        float_method.epsilon = epsilon;
        float_method.method = method;
    }

    /// Set up method comparing the float-type numbers
    void setEpsilon(const FloatCompareMethod& other)
    {
        float_method = other;
    }
    /// Get method comparing the float-type numbers
    FloatCompareMethod floatMethod() const
    {
        return float_method;
    }

    /// Set up compare template and source method
    void setMethod(CompareMethod method)
    {
        compare_method = method;
    }
    /// Get compare template and source method
    CompareMethod method() const
    {
        return compare_method;
    }

    /// Return last generated difference
    std::string getDifference() const
    {
        return oss.str();
    }

    /// Compare templ and source data
    template <class T >
    bool compare(const T& templ, const T& source)
    {
        oss.str("");
        oss.clear();
        return !diff(templ, source, oss);
    }

    /// Print difference of values
    template <class T >
    std::string value_diff_string(const T& lval, const T& rval) const
    {
        std::ostringstream oss;
        oss << std::setprecision(15) << templ_name << " = " << lval << " " << source_name << " = " << rval;
        return oss.str();
    }

    /// Print difference of sizes
    std::string size_diff_string(size_t lval_size, size_t rval_size) const;

    /// Print difference of sizes
    std::string keys_diff_string(bool is_left, std::set<std::string> keys) const;

protected:

    /// Current compare template and source method
    CompareMethod  compare_method;

    /// Method comparing the float-type numbers
    FloatCompareMethod float_method;

    /// Generate difference stream
    std::ostringstream oss;

    /// Compare all fundamental integral types, along with all their aliases
    template <class T,
             std::enable_if_t<std::is_integral<T>::value, int> = 0 >
    bool diff( const T& lval, const T& rval, std::ostream& out   )
    {
        auto ret_diff = lval != rval;
        if( ret_diff )
            out <<  value_diff_string( lval, rval );
        return ret_diff;
    }

    /// Compare all fundamental floating types (along with their aliases)
    template <class T,
             std::enable_if_t<std::is_floating_point<T>::value, int> = 0 >
    bool diff( const T& lval, const T& rval, std::ostream& out   )
    {
        bool ret_diff = float_method.diff(lval, rval);
        if( ret_diff )
            out << value_diff_string( lval, rval );
        return ret_diff;
    }

    /// Compare two vectors
    template <class T,
             std::enable_if_t<is_container<T>{}&!is_mappish<T>{}, int> = 0 >
    bool diff(const T& lval, const T& rval, std::ostream& out)
    {
        int ii = 0;
        bool is_different = false;
        std::ostringstream iss;

        // If the number of values is different for this field in compared files, the comparison fails
        //    and the numbers of values is reported for both files.
        if(lval.size() > rval.size() ||
            (lval.size() < rval.size() && compare_method == Difference)) {
            out <<  size_diff_string(lval.size(), rval.size());
            return true;
        }

        auto lval_it = lval.begin();
        auto rval_it = rval.begin();
        while(lval_it != lval.end() && rval_it != rval.end()) {
            iss.str("");
            iss.clear();
            auto ret_diff = diff(*lval_it, *rval_it, iss);
            if(ret_diff) {
                is_different = true;
                out <<  ii << ": " << iss.str() << "\n";
                // Think about recursion arrays => use yaml?
            }
            ++lval_it;
            ++rval_it;
            ++ii;
        }
        return is_different;
    }

    /// Compare two maps
    template <class T,
             std::enable_if_t<is_container<T>{}&is_mappish<T>{}, int> = 0 >
    bool diff(const T& lval, const T& rval, std::ostream& out)
    {
        std::set<std::string> not_used_left;
        bool is_different = false;
        std::ostringstream iss;

        auto lval_it = lval.begin();
        auto rval_it = rval.begin();

        while(lval_it != lval.end()) {
            auto key =  lval_it->first;
            rval_it = rval.find( key );

            if(rval_it != rval.end()) {
                iss.str("");
                iss.clear();
                auto ret_diff = diff(lval_it->second, rval_it->second, iss);
                if(ret_diff) {
                    is_different = true;
                    out <<  key << ": " << iss.str() << " \n";
                    // Think about recursion arrays => use yaml?
                }
            } else {
                not_used_left.insert(key);
            }
            ++lval_it;
        }

        if(!not_used_left.empty()) {
            is_different = true;
            oss << keys_diff_string( true, not_used_left );
        }

        std::set<std::string> not_used_right;
        if(method() == Comparator::Difference) {
            auto rval_it = rval.begin();
            while(rval_it != rval.end())  {
                lval_it = lval.find( rval_it->first );
                if(lval_it == lval.end()) {
                    not_used_right.insert(rval_it->first);
                }
                ++rval_it;
            }

            if(!not_used_right.empty()) {
                is_different = true;
                oss << keys_diff_string( false, not_used_right );
            }
        }

        return is_different;
    }

    bool diff(const std::string& lval, const std::string& rval, std::ostream& out);
    bool diff_simple(const std::string &lval, const std::string &rval, std::ostream &out);

};

} // namespace difftest
