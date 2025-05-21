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

#ifndef PEEJAY_DETAILS_TYPE_LIST_HPP
#define PEEJAY_DETAILS_TYPE_LIST_HPP

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

template <typename T> struct is_type_list : std::false_type {};

template <template <typename...> typename Seq, typename... Ts> struct is_type_list<Seq<Ts...>> : std::true_type {};

template <template <typename...> typename Seq> struct is_type_list<Seq<>> : std::true_type {};

}  // end namespace details

/// Test if a type is a type list
template <typename T> inline constexpr bool is_type_list = details::is_type_list<T>::value;
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

//  _         _                __  *
// (_)_ _  __| |_____ __  ___ / _| *
// | | ' \/ _` / -_) \ / / _ \  _| *
// |_|_||_\__,_\___/_\_\ \___/_|   *
//                                 *
constexpr inline auto npos = std::numeric_limits<std::size_t>::max();

namespace details {

template <typename Seq, typename T> struct index_of;

template <typename T, template <typename...> typename Seq>
struct index_of<Seq<>, T> : std::integral_constant<std::size_t, npos> {};

template <typename T, template <typename...> typename Seq, typename... Ts>
struct index_of<Seq<T, Ts...>, T> : std::integral_constant<std::size_t, 0> {};

template <typename T, typename TOther, template <typename...> typename Seq, typename... Ts>
struct index_of<Seq<TOther, Ts...>, T>
    : std::integral_constant<std::size_t,
                             (index_of<Seq<Ts...>, T>::value != npos) ? 1 + (index_of<Seq<Ts...>, T>::value) : npos> {};

}  // end namespace details

/// Gets the index of first occurrence of a type within a type list or npos if not found
template <sequence Seq, typename T>
struct index_of : std::integral_constant<std::size_t, details::index_of<Seq, T>::value> {};

template <sequence Seq, typename T> inline constexpr bool index_of_v = index_of<Seq, T>::value;

static_assert(index_of<type_list<double, char, bool, double>, double>::value == 0, "type_list index_of<> is broken");
static_assert(index_of<type_list<double, char, bool, double>, char>::value == 1, "type_list index_of<> is broken");
static_assert(index_of<type_list<double, char, bool>, long>::value == npos, "type_list index_of<> is broken");

//                 *
// _ __  __ ___ __ *
//| '  \/ _` \ \ / *
//|_|_|_\__,_/_\_\ *
//                 *
namespace details {

template <std::integral ResultType, typename Seq, typename M> struct max;

template <std::integral ResultType, template <typename...> typename Seq, typename M> struct max<ResultType, Seq<>, M> {
  using type = M;
};

template <std::integral ResultType, template <typename...> typename Seq, typename... Ts, typename T, typename M>
struct max<ResultType, Seq<T, Ts...>, M> {
  using type = typename std::conditional<(T::value > M::value), typename max<ResultType, Seq<Ts...>, T>::type,
                                         typename max<ResultType, Seq<Ts...>, M>::type>::type;
};

}  // end namespace details

/// Gets the maximum value from a list of std::integral_constant-like types.
template <std::integral ResultType, sequence Seq>
using max = typename details::max<ResultType, Seq, std::integral_constant<ResultType, 0>>::type;

template <std::integral ResultType, sequence Seq> inline constexpr auto max_v = max<ResultType, Seq>::type::value;

static_assert(
    std::is_same_v<
        max<std::size_t, type_list<std::integral_constant<std::size_t, 5>, std::integral_constant<std::size_t, 4>,
                                   std::integral_constant<std::size_t, 3>>>,
        std::integral_constant<std::size_t, 5>>,
    "type_list max<> is broken");
static_assert(std::is_same_v<max<int, type_list<std::integral_constant<int, -2>, std::integral_constant<int, 4>,
                                                std::integral_constant<int, 1>>>,
                             std::integral_constant<int, 4>>,
              "type_list max<> is broken");

//  _                     __                *
// | |_ _ _ __ _ _ _  ___/ _|___ _ _ _ __   *
// |  _| '_/ _` | ' \(_-<  _/ _ \ '_| '  \  *
//  \__|_| \__,_|_||_/__/_| \___/_| |_|_|_| *
//                                          *
namespace details {

template <typename List, template <typename...> typename Fun> struct transform;

template <template <typename...> typename Seq, typename... Ts, template <typename...> typename Fun>
struct transform<Seq<Ts...>, Fun> {
  using type = Seq<Fun<Ts>...>;
};

}  // end namespace details

/// Applies a unary "function" to the members of a sequence, yielding a new sequence.
template <sequence Seq, template <typename...> typename Fun>
using transform = typename details::transform<Seq, Fun>::type;

/// A unary function type which returns the size of a type.
template <typename T> struct type_sizeof : std::integral_constant<std::size_t, sizeof(T)> {};
/// A unary function type which returns the alignment of a type.
template <typename T> struct type_alignof : std::integral_constant<std::size_t, alignof(T)> {};

static_assert(
    std::is_same_v<transform<type_list<double, int>, type_sizeof>, type_list<type_sizeof<double>, type_sizeof<int>>>,
    "type_list transform<> is broken");

//       _ _        __  *
//  __ _| | |  ___ / _| *
// / _` | | | / _ \  _| *
// \__,_|_|_| \___/_|   *
//                      *
namespace details {

template <typename Seq> struct all_of;
/// An empty list always yields false.
template <template <typename...> typename Seq> struct all_of<Seq<>> : std::bool_constant<false> {};
/// Recursively check whether the head of the list is true.
template <template <typename...> typename Seq, typename... Ts>
struct all_of<Seq<Ts...>> : std::bool_constant<(Ts::value && ...)> {};

}  // end namespace details

/// Checks if a list of std::bool_constant-like types are all true.
template <sequence Seq> using all_of = typename details::all_of<Seq>;
template <sequence Seq> inline constexpr bool all_of_v = all_of<Seq>::value;

static_assert(all_of_v<type_list<std::bool_constant<true>, std::bool_constant<true>>>);
static_assert(!all_of_v<type_list<std::bool_constant<true>, std::bool_constant<false>>>);
static_assert(!all_of_v<type_list<std::bool_constant<false>, std::bool_constant<false>>>);
static_assert(!all_of_v<type_list<>>);

}  // end namespace peejay::type_list

#endif  // PEEJAY_DETAILS_TYPE_LIST_HPP
