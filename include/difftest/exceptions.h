#pragma once

#include <exception>
#include <stdexcept>
#include <string>

namespace jsonio14 {

/// @brief General exception structure into project.
class jarango_exception: public std::exception
{

public:

    /// Returns the explanatory string.
    const char* what() const noexcept override
    {
        return m.what();
    }

    /// The id of the exception.
    const int id;

    ///  @brief Create any error exception.
    ///
    ///  @param atitle    the title of the exception
    ///  @param aid       the id of the exception
    ///  @param what_arg  the explanatory string
    ///  @return jarango_exception object.
    static jarango_exception create(const std::string& atitle, int aid, const std::string& what_arg)
    {
        std::string w = header(atitle, aid) +  ": " + what_arg;
        return jarango_exception( aid, w.c_str());
    }

protected:

    jarango_exception(int aid, const char* what_arg) : id(aid), m(what_arg) {}

    static std::string header(const std::string& aname, int aid)
    {
        return "jsonio." + aname + "." + std::to_string(aid);
    }

private:

    /// An exception object as storage for error messages.
    std::runtime_error m;
};


/// Throw  jarango_exception.
[[ noreturn ]] inline void JARANGO_THROW( const std::string& title, int id, const std::string& message )
{
    throw jarango_exception::create(title, id, message);
}

/// Throw by condition jsonio_exception.
inline void JARANGO_THROW_IF(bool error, const std::string& title, int id, const std::string& message )
{
    if(error) throw jarango_exception::create(title, id, message);
}

} // namespace jsonio14
