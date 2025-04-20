//===- include/peejay/details/type_list.hpp ---------------*- mode: C++ -*-===//
//*  _                      _ _     _    *
//* | |_ _   _ _ __   ___  | (_)___| |_  *
//* | __| | | | '_ \ / _ \ | | / __| __| *
//* | |_| |_| | |_) |  __/ | | \__ \ |_  *
//*  \__|\__, | .__/ \___| |_|_|___/\__| *
//*      |___/|_|                        *
//===----------------------------------------------------------------------===//
// Copyright © 2025 Paul Bowen-Huggett
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// “Software”), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT
//===----------------------------------------------------------------------===//
// This implementation is based on the ViennaTypeListLibrary
// https://github.com/hlavacs/ViennaTypeListLibrary
//
// MIT License
//
// Copyright (c) 2021 hlavacs
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef PEEJAY_DETAILS__TYPE_LIST_HPP
#define PEEJAY_DETAILS__TYPE_LIST_HPP

#include <type_traits>
#include <variant>

namespace peejay::type_list {

template <typename... Ts> struct type_list {
  static constexpr std::size_t size = sizeof...(Ts);
};

//  _      _                    _ _    _
// (_)___ | |_ _  _ _ __  ___  | (_)__| |_
// | (_-< |  _| || | '_ \/ -_) | | (_-<  _|
// |_/__/  \__|\_, | .__/\___| |_|_/__/\__|
//             |__/|_|
namespace details {

template <typename T> struct is_type_list_impl : std::false_type {};

template <template <typename...> typename Seq, typename... Ts> struct is_type_list_impl<Seq<Ts...>> : std::true_type {};

template <template <typename...> typename Seq> struct is_type_list_impl<Seq<>> : std::true_type {};

}  // end namespace details

/// Test if a type is a type list
template <typename T> inline constexpr bool is_type_list = details::is_type_list_impl<T>::value;
template <typename T>
concept sequence = is_type_list<T>;

static_assert(!is_type_list<int>, "type_list is_type_list<> is broken");
static_assert(is_type_list<type_list<double, char, bool, double>>, "type_list is_type_list<> is broken");
static_assert(is_type_list<type_list<>>, "type_list is_type_list<> is broken");

//                      _
//  __ ___ _ _  __ __ _| |_
// / _/ _ \ ' \/ _/ _` |  _|
// \__\___/_||_\__\__,_|\__|
namespace details {

template <typename... Seq> struct concat;
// Both input lists are empty: result is empty.
template <template <typename...> typename Seq1, template <typename...> typename Seq2> struct concat<Seq1<>, Seq2<>> {
  using type = Seq1<>;
};
// The second list is empty: result is the first list,
template <template <typename...> typename Seq1, template <typename...> typename Seq2, typename... Ts>
struct concat<Seq1<Ts...>, Seq2<>> {
  using type = Seq1<Ts...>;
};
// Recursively move a type from the head of the second to the tail of the first list.
template <template <typename...> typename Seq1, typename... Ts1, template <typename...> typename Seq2, typename T,
          typename... Ts2>
struct concat<Seq1<Ts1...>, Seq2<T, Ts2...>> {
  using type = typename concat<Seq1<Ts1..., T>, Seq2<Ts2...>>::type;
};

}  // end namespace details

template <sequence... Seq> using concat = typename details::concat<Seq...>::type;

static_assert(std::is_same_v<concat<type_list<double, int>, type_list<>>, type_list<double, int>>,
              "type_list concat<> is broken");
static_assert(std::is_same_v<concat<type_list<double, int>, type_list<char>>, type_list<double, int, char>>,
              "type_list concat<> is broken");
static_assert(
    std::is_same_v<concat<type_list<double, int>, type_list<char, float>>, type_list<double, int, char, float>>,
    "type_list concat<> is broken");

//  _                      _          _
// | |_ ___  __ ____ _ _ _(_)__ _ _ _| |_
// |  _/ _ \ \ V / _` | '_| / _` | ' \  _|
//  \__\___/  \_/\__,_|_| |_\__,_|_||_\__|
namespace details {

template <typename Seq> struct to_variant;
template <template <typename...> typename Seq, typename... Ts> struct to_variant<Seq<Ts...>> {
  using type = std::variant<Ts...>;
};

}  // end namespace details

/// A variant type whose alternatie types are taken from the type list \p Seq
template <sequence Seq> using to_variant = typename details::to_variant<Seq>::type;

static_assert(std::is_same_v<to_variant<type_list<double, int, char>>, std::variant<double, int, char>>,
              "type_list to_variant<> is broken");
static_assert(
    std::is_same_v<to_variant<concat<type_list<double, int>, type_list<long>>>, std::variant<double, int, long>>,
    "type_list to_variant<> is broken");

//  _              _
// | |_  __ _ ___ | |_ _  _ _ __  ___
// | ' \/ _` (_-< |  _| || | '_ \/ -_)
// |_||_\__,_/__/  \__|\_, | .__/\___|
//                     |__/|_|
namespace details {

template <typename Seq, typename T> struct has_type;
/// An empty list always yields false.
template <template <typename...> typename Seq, typename T> struct has_type<Seq<>, T> {
  static constexpr bool value = false;
};
/// Recursively check whether the head of the list matches.
template <template <typename...> typename Seq, typename... Ts, typename T> struct has_type<Seq<Ts...>, T> {
  static constexpr bool value = (std::is_same_v<T, Ts> || ...);
};

}  // end namespace details

template <sequence Seq, typename T> struct has_type {
  static constexpr bool value = details::has_type<Seq, T>::value;
};
template <sequence Seq, typename T> inline constexpr bool has_type_v = has_type<Seq, T>::value;

static_assert(has_type<type_list<double, int, char, double>, char>::value, "typelist has_type<> is broken");
static_assert(!has_type<type_list<double, int, char, double>, float>::value, "typelist has_type<> is broken");

}  // end namespace peejay::type_list

#endif  // PEEJAY_DETAILS__TYPE_LIST_HPP
