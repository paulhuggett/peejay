#ifndef PEEJAY_PORTAB_HPP
#define PEEJAY_PORTAB_HPP

#if __cplusplus >= 202002L
#define PEEJAY_CXX20 (1)
#elif defined(_MSVC_LANG) && _MSVC_LANG >= 202002L
// MSVC does not set the value of __cplusplus correctly unless the
// /Zc:__cplusplus is supplied. We have to detect C++20 using its
// compiler-specific macros instead.
#define PEEJAY_CXX20 (1)
#else
#define PEEJAY_CXX20 (0)
#endif

#if PEEJAY_CXX20
#define PEEJAY_CXX20REQUIRES(x) requires x
#else
#define PEEJAY_CXX20REQUIRES(x)
#endif  // PEEJAY_CXX20

#endif  // PEEJAY_PORTAB_HPP
