//===- include/peejay/portab.hpp --------------------------*- mode: C++ -*-===//
//*                   _        _      *
//*  _ __   ___  _ __| |_ __ _| |__   *
//* | '_ \ / _ \| '__| __/ _` | '_ \  *
//* | |_) | (_) | |  | || (_| | |_) | *
//* | .__/ \___/|_|   \__\__,_|_.__/  *
//* |_|                               *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
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
#define PEEJAY_CONSTEXPR_CXX20 constexpr
#else
#define PEEJAY_CONSTEXPR_CXX20 inline
#endif

#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#if defined(__cpp_concepts) && defined(__cpp_lib_concepts)
#define PEEJAY_HAVE_CONCEPTS (1)
#else
#define PEEJAY_HAVE_CONCEPTS (0)
#endif

#if PEEJAY_HAVE_CONCEPTS
// This macro can't be written using a constexpr template function.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PEEJAY_CXX20REQUIRES(x) requires x
#else
#define PEEJAY_CXX20REQUIRES(x)
#endif  // PEEJAY_HAVE_CONCEPTS

#if PEEJAY_CXX20 && defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
#define PEEJAY_HAVE_SPAN (1)
#else
#define PEEJAY_HAVE_SPAN (0)
#endif

#if PEEJAY_CXX20 && defined(__has_cpp_attribute)
#if __has_cpp_attribute(unlikely) >= 201803L
#define PEEJAY_UNLIKELY_ATTRIBUTE [[unlikely]]
#endif
#if __has_cpp_attribute(no_unique_address)
#define PEEJAY_NO_UNIQUE_ADDRESS_ATTRIBUTE [[no_unique_address]]
#endif
#endif  // PEEJAY_CXX20 && defined(__has_cpp_attribute)
#ifndef PEEJAY_UNLIKELY_ATTRIBUTE
#define PEEJAY_UNLIKELY_ATTRIBUTE
#endif
#ifndef PEEJAY_NO_UNIQUE_ADDRESS_ATTRIBUTE
#define PEEJAY_NO_UNIQUE_ADDRESS_ATTRIBUTE
#endif

#endif  // PEEJAY_PORTAB_HPP
