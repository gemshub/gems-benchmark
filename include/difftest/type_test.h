#pragma once

#include <type_traits>

namespace difftest {

namespace detail {

// Needed for some older versions of GCC
template<typename...>
struct voider { using type = void; };

// std::void_t will be part of C++17, but until then define it ourselves:
template<typename... T>
using void_t = typename voider<T...>::type;

template<typename T, typename U = void>
struct is_mappish_impl : std::false_type { };

template<typename T>
struct is_mappish_impl<T, void_t<typename T::key_type,
                                 typename T::mapped_type,
                                 decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])>>
                                                                                    : std::true_type { };


template<typename T, typename U = void>
struct is_container_impl : std::false_type { };

template<typename T>
struct is_container_impl<T, void_t<typename T::value_type,
        typename T::mapped_type,
        decltype(std::declval<T>().size()),
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end()),
        decltype(std::declval<T>().cbegin()),
        decltype(std::declval<T>().cend())>>
                                     : std::true_type { };
}


template<typename T>
struct is_mappish : detail::is_mappish_impl<T>::type { };


//template<typename T>
//struct is_container : detail::is_container_impl<T>::type { };

// https://stackoverflow.com/questions/12042824/how-to-write-a-type-trait-is-container-or-is-vector
template<typename T>
struct has_const_iterator
{
private:
    typedef char                      yes;
    typedef struct { char array[2]; } no;

    template<typename C> static yes test(typename C::const_iterator*);
    template<typename C> static no  test(...);
public:
    static const bool value = sizeof(test<T>(0)) == sizeof(yes);
    typedef T type;
};

template <typename T>
struct has_begin_end
{
    template<typename C> static char (&f(typename std::enable_if<
      std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::begin)),
      typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

    template<typename C> static char (&f(...))[2];

    template<typename C> static char (&g(typename std::enable_if<
      std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::end)),
      typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

    template<typename C> static char (&g(...))[2];

    static bool const beg_value = sizeof(f<T>(0)) == 1;
    static bool const end_value = sizeof(g<T>(0)) == 1;
};

template<typename T>
 struct is_container : std::integral_constant<bool, has_const_iterator<T>::value && has_begin_end<T>::beg_value && has_begin_end<T>::end_value>
 { };

// https://stackoverflow.com/questions/12042824/how-to-write-a-type-trait-is-container-or-is-vector

} // namespace difftest
