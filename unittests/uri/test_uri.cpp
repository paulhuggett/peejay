//===- unittests/uri/test_uri.cpp -----------------------------------------===//
//*             _  *
//*  _   _ _ __(_) *
//* | | | | '__| | *
//* | |_| | |  | | *
//*  \__,_|_|  |_| *
//*                *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//

#include <algorithm>
#include <numeric>

#if __has_include(<version>)
#include <version>
#endif

// google test
#include <gmock/gmock.h>
#if URI_FUZZTEST
#include <fuzztest/fuzztest.h>
#endif

#include "uri/parts.hpp"
#include "uri/pctencode.hpp"
#include "uri/uri.hpp"

// to address
// ~~~~~~~~~~
#if defined(__cpp_lib_to_address)
template <typename T>
[[nodiscard]] constexpr auto to_address (T&& p) noexcept {
  return std::to_address (std::forward<T> (p));
}
#else
// True if std::pointer_traits<T>::to_address is available.
template <typename T, typename = void>
inline constexpr bool has_to_address = false;
template <typename T>
inline constexpr bool
  has_to_address<T, std::void_t<decltype (std::pointer_traits<T>::to_address (
                      std::declval<T const&> ()))>> = true;

template <typename T>
[[nodiscard]] constexpr T* to_address (T* const p) noexcept {
  static_assert (!std::is_function_v<T>, "T must not be a function type");
  return p;
}
template <typename T>
[[nodiscard]] constexpr auto to_address (T&& p) noexcept {
  using P = std::decay_t<T>;
  if constexpr (has_to_address<P>) {
    return std::pointer_traits<P>::to_address (std::forward<T> (p));
  } else {
    return to_address (p.operator->());
  }
}
#endif  // defined(__cpp_lib_to_address)

using testing::ElementsAre;
using namespace std::string_view_literals;

// The tests in this group were generated using the 'abnfgen' tool with the
// official URI ABNF. The tool was asked to produce 100 tests with the "attempt
// full coverage" (-c) mode enabled.

// NOLINTNEXTLINE
TEST (UriSplit, Empty) {
  std::optional<uri::parts> const x = uri::split_reference ("");
  ASSERT_TRUE (x);
  EXPECT_FALSE (x->scheme);
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, EmptyPathComponents) {
  std::optional<uri::parts> const x = uri::split_reference ("/foo///bar");
  ASSERT_TRUE (x);
  EXPECT_FALSE (x->scheme);
  EXPECT_FALSE (x->authority);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("foo", "", "", "bar"));
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}

// NOLINTNEXTLINE
TEST (UriSplit, 0001) {
  auto const x =
    uri::split_reference ("C://[::A:eE5c]:2194/&///@//:_/%aB//.////#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "C");
  ASSERT_TRUE (x->authority);
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[::A:eE5c]");
  EXPECT_EQ (x->authority->port, "2194");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("&", "", "", "@", "", ":_", "%aB",
                                              "", ".", "", "", "", ""));
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
  EXPECT_EQ (static_cast<std::string> (x->path), "/&///@//:_/%aB//.////");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path).generic_string (),
             "/&/@/:_/%aB/./");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0002) {
  auto const x = uri::split_reference ("P-.:/?/?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "P-.");
  EXPECT_FALSE (x->authority);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
  EXPECT_EQ (x->query, "/?");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0003) {
  auto const x = uri::split_reference ("i+V:?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "i+V");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0004) {
  auto const x = uri::split_reference ("L:%Cf#%dD/?H");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "L");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("%Cf"));
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "%dD/?H");
  EXPECT_EQ (static_cast<std::string> (x->path), "%Cf");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path).generic_string (),
             "%Cf");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0005) {
  auto const x =
    uri::split_reference ("E07:/8=-~%bF//%36////'/%16N%78//)/%53/;?*!");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "E07");
  EXPECT_FALSE (x->authority);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments,
               ElementsAre ("8=-~%bF", "", "%36", "", "", "", "'", "%16N%78",
                            "", ")", "%53", ";"));
  EXPECT_EQ (x->query, "*!");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0006) {
  auto const x = uri::split_reference ("v:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "v");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0007) {
  auto const x = uri::split_reference ("YXa:/#B");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "YXa");
  EXPECT_FALSE (x->authority);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "B");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0008) {
  auto const x = uri::split_reference ("n:/,+?$#(+!)D");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "n");
  EXPECT_FALSE (x->authority);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (",+"));
  EXPECT_EQ (x->query, "$");
  EXPECT_EQ (x->fragment, "(+!)D");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0009) {
  auto const x = uri::split_reference ("m:/?cJ");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "m");
  EXPECT_FALSE (x->authority);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
  EXPECT_EQ (x->query, "cJ");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0010) {
  auto const x = uri::split_reference ("zR:d/M/kx/s/GTl///SgA/?#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "zR");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments,
               ElementsAre ("d", "M", "kx", "s", "GTl", "", "", "SgA", ""));
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0011) {
  auto const x = uri::split_reference ("t:W?p#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "t");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("W"));
  EXPECT_EQ (x->query, "p");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0012) {
  auto const x = uri::split_reference ("QrIq:/#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "QrIq");
  EXPECT_FALSE (x->authority);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0013) {
  auto const x = uri::split_reference ("OuU:/?bZK");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "OuU");
  EXPECT_FALSE (x->authority);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
  EXPECT_EQ (x->query, "bZK");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0014) {
  auto const x = uri::split_reference ("Fjfe:?h");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "Fjfe");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "h");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0015) {
  auto const x = uri::split_reference ("y:w/o/b/?lKTF");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "y");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("w", "o", "b", ""));
  EXPECT_EQ (x->query, "lKTF");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0016) {
  auto const x = uri::split_reference ("P://=:_%bb%Cf%2F-8;~@230.109.31.250#.");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "P");
  ASSERT_TRUE (x->authority);
  EXPECT_EQ (x->authority->userinfo, "=:_%bb%Cf%2F-8;~");
  EXPECT_EQ (x->authority->host, "230.109.31.250");
  EXPECT_FALSE (x->authority->port);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, ".");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0017) {
  auto const x = uri::split_reference ("N://@=i%bD%Cb&*%Ea)%CE//:%cA//#?//");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "N");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "");
  EXPECT_EQ (x->authority->host, "=i%bD%Cb&*%Ea)%CE");
  EXPECT_FALSE (x->authority->port);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("", ":%cA", "", ""));
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "?//");
  EXPECT_EQ (static_cast<std::string> (x->path), "//:%cA//");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path), "/:%cA/");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0018) {
  auto const x = uri::split_reference ("X:#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "X");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0019) {
  auto const x = uri::split_reference ("U:??");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "U");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "?");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0020) {
  auto const x = uri::split_reference ("G:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "G");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0021) {
  auto const x = uri::split_reference ("l6+:?#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "l6+");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0022) {
  auto const x = uri::split_reference ("T.-://:@[VD.~]:?/@#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "T.-");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, ":");
  EXPECT_EQ (x->authority->host, "[VD.~]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "/@");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0023) {
  auto const x = uri::split_reference ("rC://3.76.206.5:8966?/");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "rC");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "3.76.206.5");
  EXPECT_EQ (x->authority->port, "8966");
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "/");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0024) {
  auto const x = uri::split_reference ("oNP:///::");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "oNP");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "");
  EXPECT_FALSE (x->authority->port);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("::"));
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0025) {
  auto const x = uri::split_reference ("g0:?#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "g0");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0026) {
  auto const x = uri::split_reference ("Do1-:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "Do1-");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0027) {
  auto const x = uri::split_reference ("K:?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "K");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0028) {
  auto const x = uri::split_reference ("tc://@[::F]:/::@~?@/");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "tc");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "");
  EXPECT_EQ (x->authority->host, "[::F]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("::@~"));
  EXPECT_EQ (x->query, "@/");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0029) {
  auto const x = uri::split_reference ("N:#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "N");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0030) {
  auto const x = uri::split_reference ("o:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "o");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0031) {
  auto const x = uri::split_reference (R"(k-0+:???/)");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "k-0+");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, R"(??/)");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0032) {
  auto const x =
    uri::split_reference (R"(y://%DD@253.216.255.251//aa/??/://;)");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "y");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "%DD");
  EXPECT_EQ (x->authority->host, "253.216.255.251");
  EXPECT_FALSE (x->authority->port.has_value ());
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("", "aa", ""));
  EXPECT_EQ (x->query, "?/://;");
  EXPECT_FALSE (x->fragment);
  EXPECT_EQ (static_cast<std::string> (x->path), "//aa/");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path), "/aa/");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0033) {
  auto const x =
    uri::split_reference ("B://.@[AC::1:6DEb:14.97.229.249]:?/#??~(");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "B");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, ".");
  EXPECT_EQ (x->authority->host, "[AC::1:6DEb:14.97.229.249]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "/");
  EXPECT_EQ (x->fragment, "??~(");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0034) {
  auto const x = uri::split_reference ("p://@26.254.86.252://aa");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "p");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "");
  EXPECT_EQ (x->authority->host, "26.254.86.252");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("", "aa"));
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
  EXPECT_EQ (static_cast<std::string> (x->path), "//aa");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path), "/aa");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0035) {
  auto const x = uri::split_reference ("P+-n:#/%f0");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "P+-n");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "/%f0");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0036) {
  auto const x = uri::split_reference ("u:?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "u");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0037) {
  auto const x = uri::split_reference ("U://%Aa:@[::b:E:A:53.48.69.41]?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "U");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "%Aa:");
  EXPECT_EQ (x->authority->host, "[::b:E:A:53.48.69.41]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0038) {
  auto const x = uri::split_reference ("h.P+9:?:#?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "h.P+9");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, ":");
  EXPECT_EQ (x->fragment, "?");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0039) {
  auto const x = uri::split_reference ("x:??#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "x");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "?");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0040) {
  auto const x = uri::split_reference ("A:#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "A");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0041) {
  auto const x = uri::split_reference ("Lp.:?#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "Lp.");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0042) {
  auto const x = uri::split_reference ("d-:#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "d-");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0043) {
  auto const x = uri::split_reference ("h-.:?/?/#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "h-.");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "/?/");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0044) {
  auto const x = uri::split_reference ("d:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "d");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0045) {
  auto const x = uri::split_reference ("L:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "L");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0046) {
  auto const x = uri::split_reference ("Z5://@[9:BB:8:DAc:BbAA:E:a::]?#@$");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "Z5");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "");
  EXPECT_EQ (x->authority->host, "[9:BB:8:DAc:BbAA:E:a::]");
  EXPECT_FALSE (x->authority->port.has_value ());
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "@$");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0047) {
  auto const x =
    uri::split_reference ("C-://[::1E:BB:a:5c1:Dd:40.44.228.108]/;?#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "C-");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[::1E:BB:a:5c1:Dd:40.44.228.108]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (";"));
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0048) {
  auto const x =
    uri::split_reference ("z://[c:BC:b:A:Bd:D:dC1f:cedB]?/#/:/%FA");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "z");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[c:BC:b:A:Bd:D:dC1f:cedB]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "/");
  EXPECT_EQ (x->fragment, "/:/%FA");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0049) {
  auto const x = uri::split_reference ("x.2:#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "x.2");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0050) {
  auto const x = uri::split_reference ("p://@[::F:e:4b:eCBE:f:c]");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "p");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "");
  EXPECT_EQ (x->authority->host, "[::F:e:4b:eCBE:f:c]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0051) {
  auto const x = uri::split_reference ("tmi://[e:C:Aa:eD::FDfD:b:F]:?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "tmi");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[e:C:Aa:eD::FDfD:b:F]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0052) {
  auto const x = uri::split_reference ("G+:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "G+");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0053) {
  auto const x =
    uri::split_reference ("A://[vA5.+:=.p~=)=&_;-=7)(.;]:768295/+");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "A");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[vA5.+:=.p~=)=&_;-=7)(.;]");
  EXPECT_EQ (x->authority->port, "768295");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("+"));
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0054) {
  auto const x = uri::split_reference ("n+://[::]:9831#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "n+");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[::]");
  EXPECT_EQ (x->authority->port, "9831");
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0055) {
  auto const x = uri::split_reference ("v-2e.l:?:????:/");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "v-2e.l");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, ":????:/");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0056) {
  auto const x = uri::split_reference ("ka+://6.@[F::219.226.254.253]:900/'R#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "ka+");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "6.");
  EXPECT_EQ (x->authority->host, "[F::219.226.254.253]");
  EXPECT_EQ (x->authority->port, "900");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("'R"));
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0057) {
  auto const x = uri::split_reference ("P://[daf::B:7:e:b:D:F]:730");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "P");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[daf::B:7:e:b:D:F]");
  EXPECT_EQ (x->authority->port, "730");
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0058) {
  auto const x = uri::split_reference ("H://-!:_%Bd@[::]:7");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "H");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "-!:_%Bd");
  EXPECT_EQ (x->authority->host, "[::]");
  EXPECT_EQ (x->authority->port, "7");
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0059) {
  auto const x = uri::split_reference ("u+://;@[::dFC:d:6:d]://#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "u+");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, ";");
  EXPECT_EQ (x->authority->host, "[::dFC:d:6:d]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("", ""));
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
  EXPECT_EQ (static_cast<std::string> (x->path), "//");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path), "/");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0060) {
  auto const x = uri::split_reference ("D://[dCDa:c:e:B:F::D:a]:/%Dc");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "D");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[dCDa:c:e:B:F::D:a]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("%Dc"));
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0061) {
  auto const x = uri::split_reference ("mF2:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "mF2");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0062) {
  auto const x =
    uri::split_reference ("f.://[d1b:CF:AbBa::F:d:11.246.155.253]?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "f.");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[d1b:CF:AbBa::F:d:11.246.155.253]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0063) {
  auto const x = uri::split_reference ("f5++://@[7d::6:df:f:245.95.78.9]:??");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "f5++");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "");
  EXPECT_EQ (x->authority->host, "[7d::6:df:f:245.95.78.9]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "?");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0064) {
  auto const x = uri::split_reference ("c.l://[::bba:B:6:1.255.161.3]:#?/");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "c.l");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[::bba:B:6:1.255.161.3]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "?/");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0065) {
  auto const x = uri::split_reference ("T://[fdF::f2]");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "T");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[fdF::f2]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0066) {
  auto const x = uri::split_reference ("U-92.://[::A:C:c]/");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "U-92.");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[::A:C:c]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
  EXPECT_EQ (static_cast<std::string> (x->path), "/");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path), "/");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0067) {
  auto const x = uri::split_reference ("K:?#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "K");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0068) {
  auto const x = uri::split_reference ("l.://[c:CEa:cd1B:f:f:D::ef]?#%bC@/:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "l.");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[c:CEa:cd1B:f:f:D::ef]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "%bC@/:");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0069) {
  auto const x = uri::split_reference (
    R"(v+://@[::C:dEd:4:218.255.251.5]:8/@.;J??Q??%48/#)");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "v+");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "");
  EXPECT_EQ (x->authority->host, "[::C:dEd:4:218.255.251.5]");
  EXPECT_EQ (x->authority->port, "8");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("@.;J"));
  EXPECT_EQ (x->query, R"(?Q??%48/)");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0070) {
  auto const x = uri::split_reference ("I:?#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "I");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0071) {
  auto const x = uri::split_reference ("t.+://[::Ec:AcA:9a]:92/%8a/#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "t.+");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[::Ec:AcA:9a]");
  EXPECT_EQ (x->authority->port, "92");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("%8a", ""));
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
  EXPECT_EQ (static_cast<std::string> (x->path), "/%8a/");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path), "/%8a/");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0072) {
  auto const x = uri::split_reference ("N+:?~");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "N+");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "~");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0073) {
  auto const x = uri::split_reference ("B:?/.#?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "B");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "/.");
  EXPECT_EQ (x->fragment, "?");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0074) {
  auto const x = uri::split_reference ("u8K.://.(@[d::Baa:dE:D]#/");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "u8K.");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, ".(");
  EXPECT_EQ (x->authority->host, "[d::Baa:dE:D]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "/");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0075) {
  auto const x = uri::split_reference ("E+.://@[::F:ab79:B:fa:C]#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "E+.");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_EQ (x->authority->userinfo, "");
  EXPECT_EQ (x->authority->host, "[::F:ab79:B:fa:C]");
  EXPECT_FALSE (x->authority->port);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0076) {
  auto const x = uri::split_reference ("S+://[::BBc:d0:EA:3.67.149.137]:/?#/");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "S+");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[::BBc:d0:EA:3.67.149.137]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "/");

  EXPECT_EQ (static_cast<std::string> (x->path), "/");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path), "/");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0077) {
  auto const x =
    uri::split_reference (R"(Y://[4Bbc:bb::cDcd:5:c4:e:B1]:/%CA@/./??)");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "Y");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[4Bbc:bb::cDcd:5:c4:e:B1]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("%CA@", ".", ""));
  EXPECT_EQ (x->query, "?");
  EXPECT_FALSE (x->fragment);

  EXPECT_EQ (static_cast<std::string> (x->path), "/%CA@/./");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path), "/%CA@/./");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0078) {
  auto const x = uri::split_reference ("W.-://[CF::]://!?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "W.-");
  ASSERT_TRUE (x->authority.has_value ());
  EXPECT_FALSE (x->authority->userinfo);
  EXPECT_EQ (x->authority->host, "[CF::]");
  EXPECT_EQ (x->authority->port, "");
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("", "!"));
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);

  EXPECT_EQ (static_cast<std::string> (x->path), "//!");
  EXPECT_EQ (static_cast<std::filesystem::path> (x->path), "/!");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0079) {
  auto const x = uri::split_reference ("SF6:#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "SF6");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0080) {
  auto const x = uri::split_reference (R"(R:?????////???/////#??@?_:?)");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "R");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, R"(????////???/////)");
  EXPECT_EQ (x->fragment, "??@?_:?");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0081) {
  auto const x = uri::split_reference ("g:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "g");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0082) {
  auto const x = uri::split_reference (R"(D-ir+.PA:??#)");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "D-ir+.PA");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "?");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0083) {
  auto const x = uri::split_reference ("Z-.-:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "Z-.-");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0084) {
  auto const x = uri::split_reference ("y-:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "y-");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0085) {
  auto const x = uri::split_reference ("p:?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "p");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0086) {
  auto const x = uri::split_reference ("M:#*.");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "M");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "*.");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0087) {
  auto const x = uri::split_reference ("I:?%ab#/.");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "I");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "%ab");
  EXPECT_EQ (x->fragment, "/.");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0088) {
  auto const x = uri::split_reference ("v6:#:?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "v6");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, ":?");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0089) {
  auto const x = uri::split_reference ("D:#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "D");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0090) {
  auto const x = uri::split_reference ("e.:#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "e.");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0091) {
  auto const x = uri::split_reference ("L:?#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "L");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0092) {
  auto const x = uri::split_reference ("g-:#");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "g-");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0093) {
  auto const x = uri::split_reference ("H:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "H");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0094) {
  auto const x = uri::split_reference (R"(K:??)");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "K");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "?");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0095) {
  auto const x = uri::split_reference (R"(c-:?#)");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "c-");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_EQ (x->fragment, "");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0096) {
  auto const x = uri::split_reference ("Bw:?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "Bw");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0097) {
  auto const x = uri::split_reference ("hC:?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "hC");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0098) {
  auto const x = uri::split_reference ("q:?/#/");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "q");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "/");
  EXPECT_EQ (x->fragment, "/");
}
// NOLINTNEXTLINE
TEST (UriSplit, 0099) {
  auto const x = uri::split_reference ("L:");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "L");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_FALSE (x->query);
  EXPECT_FALSE (x->fragment);
}
// NOLINTNEXTLINE
TEST (UriSplit, 0100) {
  auto const x = uri::split_reference ("W-:?");
  ASSERT_TRUE (x);
  EXPECT_EQ (x->scheme, "W-");
  EXPECT_FALSE (x->authority);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_TRUE (x->path.segments.empty ());
  EXPECT_EQ (x->query, "");
  EXPECT_FALSE (x->fragment);
}

#if URI_FUZZTEST
static void UriSplitNeverCrashes (std::string const& input) {
  uri::split (input);
}
// NOLINTNEXTLINE
FUZZ_TEST (UriSplitFuzz, UriSplitNeverCrashes);
#endif  // URI_FUZZTEST

// NOLINTNEXTLINE
TEST (RemoveDotSegments, LeadingDotDotSlash) {
  auto x = uri::split_reference ("../bar");
  ASSERT_TRUE (x);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("..", "bar"));
  x->path.remove_dot_segments ();
  EXPECT_THAT (x->path.segments, ElementsAre ("bar"));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, LeadingDotSlash) {
  auto x = uri::split_reference ("./bar");
  ASSERT_TRUE (x);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (".", "bar"));
  x->path.remove_dot_segments ();
  EXPECT_THAT (x->path.segments, ElementsAre ("bar"));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, LeadingDotDotSlashDotSlash) {
  auto x = uri::split_reference (".././bar");
  ASSERT_TRUE (x);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("..", ".", "bar"));
  x->path.remove_dot_segments ();
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("bar"));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, MidDot) {
  auto x = uri::split_reference ("/foo/./bar");
  ASSERT_TRUE (x);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("foo", ".", "bar"));
  x->path.remove_dot_segments ();
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("foo", "bar"));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, LonelySlashDot) {
  auto x = uri::split_reference ("/.");
  ASSERT_TRUE (x);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("."));

  x->path.remove_dot_segments ();

  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, TrailingDotSlash) {
  auto x = uri::split_reference ("/bar/./");
  ASSERT_TRUE (x);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("bar", ".", ""));

  x->path.remove_dot_segments ();

  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("bar", ""));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, MidSlashDotDot) {
  auto x = uri::split_reference ("/foo/../bar");
  ASSERT_TRUE (x);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("foo", "..", "bar"));

  x->path.remove_dot_segments ();

  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("bar"));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, TrailingDotDotSlash) {
  auto x = uri::split_reference ("/bar/../");
  ASSERT_TRUE (x);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("bar", "..", ""));

  x->path.remove_dot_segments ();

  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, LonelySlashDotDot) {
  auto x = uri::split_reference ("/..");
  ASSERT_TRUE (x);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (".."));

  x->path.remove_dot_segments ();

  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));

  auto const x2 = uri::split_reference ("/");
  EXPECT_EQ (x, x2);
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, TrailingSlashDotDot) {
  auto x = uri::split_reference ("/bar/..");
  ASSERT_TRUE (x);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("bar", ".."));

  x->path.remove_dot_segments ();

  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));

  auto const x2 = uri::split_reference ("/");
  EXPECT_EQ (x, x2);
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, TwoDirectoriesTrailingSlashDotDot) {
  auto x = uri::split_reference ("/foo/bar/..");
  ASSERT_TRUE (x);
  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("foo", "bar", ".."));

  x->path.remove_dot_segments ();

  EXPECT_TRUE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("foo", ""));

  auto const x2 = uri::split_reference ("/foo/");
  EXPECT_EQ (x, x2);
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, LonelyDot) {
  auto x = uri::split_reference (".");
  ASSERT_TRUE (x);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("."));

  x->path.remove_dot_segments ();

  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, LonelyDotDot) {
  auto x = uri::split_reference ("..");
  ASSERT_TRUE (x);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (".."));

  x->path.remove_dot_segments ();

  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
}
// NOLINTNEXTLINE
TEST (RemoveDotSegments, LonelyDotDotSlashDot) {
  auto x = uri::split_reference ("../.");
  ASSERT_TRUE (x);
  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre ("..", "."));

  x->path.remove_dot_segments ();

  EXPECT_FALSE (x->path.absolute);
  EXPECT_THAT (x->path.segments, ElementsAre (""));
}

// NOLINTNEXTLINE
TEST (UriFileSystemPath, Root) {
  struct uri::parts::path p;
  p.absolute = true;
  EXPECT_EQ (static_cast<std::filesystem::path> (p),
             std::filesystem::path ("/"));
}
// NOLINTNEXTLINE
TEST (UriFileSystemPath, RootFoo) {
  struct uri::parts::path p;
  p.absolute = true;
  p.segments.emplace_back ("foo");
  EXPECT_EQ (static_cast<std::filesystem::path> (p),
             std::filesystem::path ("/foo"));
}
// NOLINTNEXTLINE
TEST (UriFileSystemPath, AbsoluteTwoSegments) {
  struct uri::parts::path p;
  p.absolute = true;
  p.segments.emplace_back ("foo");
  p.segments.emplace_back ("bar");
  EXPECT_EQ (static_cast<std::filesystem::path> (p),
             std::filesystem::path ("/foo/bar"));
}
// NOLINTNEXTLINE
TEST (UriFileSystemPath, AbsoluteTwoSegmentsDirectory) {
  struct uri::parts::path p;
  p.absolute = true;
  p.segments.emplace_back ("foo");
  p.segments.emplace_back ("bar");
  p.segments.emplace_back ();
  EXPECT_EQ (static_cast<std::filesystem::path> (p),
             std::filesystem::path ("/foo/bar/"));
}
// NOLINTNEXTLINE
TEST (UriFileSystemPath, RelativeTwoSegments) {
  struct uri::parts::path p;
  p.segments.emplace_back ("foo");
  p.segments.emplace_back ("bar");
  EXPECT_EQ (static_cast<std::filesystem::path> (p),
             std::filesystem::path ("foo/bar"));
}
// NOLINTNEXTLINE
TEST (UriFileSystemPath, RelativeTwoSegmentsDirectory) {
  struct uri::parts::path p;
  p.segments.emplace_back ("foo");
  p.segments.emplace_back ("bar");
  p.segments.emplace_back ();
  EXPECT_EQ (static_cast<std::filesystem::path> (p),
             std::filesystem::path ("foo/bar/"));
}

class Join : public testing::Test {
protected:
  static constexpr std::string_view base_ = "http://a/b/c/d;p?q";
};

// uri::join() test cases from RFC 3986 5.4.1. Normal Examples.
// NOLINTNEXTLINE
TEST_F (Join, Normal) {
  EXPECT_EQ (uri::split ("g:h"), uri::join (base_, "g:h"));
  EXPECT_EQ (uri::split ("http://a/b/c/g"), uri::join (base_, "g"));
  EXPECT_EQ (uri::split ("http://a/b/c/g"), uri::join (base_, "./g"));
  EXPECT_EQ (uri::split ("http://a/b/c/g/"), uri::join (base_, "g/"));
  EXPECT_EQ (uri::split ("http://a/g"), uri::join (base_, "/g"));
  EXPECT_EQ (uri::split ("http://g"), uri::join (base_, "//g"));
  EXPECT_EQ (uri::split ("http://a/b/c/d;p?y"), uri::join (base_, "?y"));
  EXPECT_EQ (uri::split ("http://a/b/c/g?y"), uri::join (base_, "g?y"));
  EXPECT_EQ (uri::split ("http://a/b/c/d;p?q#s"), uri::join (base_, "#s"));
  EXPECT_EQ (uri::split ("http://a/b/c/d;p?q#s"), uri::join (base_, "#s"));
  EXPECT_EQ (uri::split ("http://a/b/c/g#s"), uri::join (base_, "g#s"));
  EXPECT_EQ (uri::split ("http://a/b/c/g?y#s"), uri::join (base_, "g?y#s"));
  EXPECT_EQ (uri::split ("http://a/b/c/;x"), uri::join (base_, ";x"));
  EXPECT_EQ (uri::split ("http://a/b/c/g;x"), uri::join (base_, "g;x"));
  EXPECT_EQ (uri::split ("http://a/b/c/g;x?y#s"), uri::join (base_, "g;x?y#s"));
  EXPECT_EQ (uri::split ("http://a/b/c/d;p?q"), uri::join (base_, ""));
  EXPECT_EQ (uri::split ("http://a/b/c/"), uri::join (base_, "."));
  EXPECT_EQ (uri::split ("http://a/b/c/"), uri::join (base_, "./"));
  EXPECT_EQ (uri::split ("http://a/b/"), uri::join (base_, ".."));
  EXPECT_EQ (uri::split ("http://a/b/"), uri::join (base_, "../"));
  EXPECT_EQ (uri::split ("http://a/b/g"), uri::join (base_, "../g"));
  EXPECT_EQ (uri::split ("http://a/"), uri::join (base_, "../.."sv));
  EXPECT_EQ (uri::split ("http://a/"), uri::join (base_, "../../"));
  EXPECT_EQ (uri::split ("http://a/g"), uri::join (base_, "../../g"));
  EXPECT_EQ (uri::split ("file://user@a/g"), uri::join ("file://user@a", "../../g"));
}

// NOLINTNEXTLINE
TEST_F (Join, Abnormal) {
  // Check that we are careful in handling cases where there are more ".."
  // segments in a relative-path reference than there are hierarchical levels in
  // the base URI's path.
  EXPECT_EQ (uri::split ("http://a/g"), uri::join (base_, "../../../g"));
  EXPECT_EQ (uri::split ("http://a/g"), uri::join (base_, "../../../../g"));

  // Check we correctly remove the dot-segments "." and ".." when they are
  // complete components of a path, but not when they are only part of a
  // segment.
  EXPECT_EQ (uri::split ("http://a/g"), uri::join (base_, "/./g"));
  EXPECT_EQ (uri::split ("http://a/g"), uri::join (base_, "/../g"));
  EXPECT_EQ (uri::split ("http://a/b/c/g."), uri::join (base_, "g."));
  EXPECT_EQ (uri::split ("http://a/b/c/.g"), uri::join (base_, ".g"));
  EXPECT_EQ (uri::split ("http://a/b/c/g.."), uri::join (base_, "g.."));
  EXPECT_EQ (uri::split ("http://a/b/c/..g"), uri::join (base_, "..g"));

  // Verify cases where the relative reference uses unnecessary or nonsensical
  // forms of the "." and ".." complete path segments.
  EXPECT_EQ (uri::split ("http://a/b/g"), uri::join (base_, "./../g"));
  EXPECT_EQ (uri::split ("http://a/b/c/g/"), uri::join (base_, "./g/."));
  EXPECT_EQ (uri::split ("http://a/b/c/g/h"), uri::join (base_, "g/./h"));
  EXPECT_EQ (uri::split ("http://a/b/c/h"), uri::join (base_, "g/../h"));
  EXPECT_EQ (uri::split ("http://a/b/c/g;x=1/y"),
             uri::join (base_, "g;x=1/./y"));
  EXPECT_EQ (uri::split ("http://a/b/c/y"), uri::join (base_, "g;x=1/../y"));

  // Check that we correctly separate the reference's query and/or fragment
  // components from the path component before merging it with the base path and
  // removing dot-segments.
  EXPECT_EQ (uri::split ("http://a/b/c/g?y/./x"), uri::join (base_, "g?y/./x"));
  EXPECT_EQ (uri::split ("http://a/b/c/g?y/../x"),
             uri::join (base_, "g?y/../x"));
  EXPECT_EQ (uri::split ("http://a/b/c/g#s/./x"), uri::join (base_, "g#s/./x"));
  EXPECT_EQ (uri::split ("http://a/b/c/g#s/../x"),
             uri::join (base_, "g#s/../x"));

  // Verify the behviour when the scheme name is present in a relative reference
  // if it is the same as the base URI scheme.
  EXPECT_EQ (uri::split ("http:g"), uri::join (base_, "http:g"));
}

// NOLINTNEXTLINE
TEST (UriCompose, Empty) {
  uri::parts p;
  EXPECT_EQ (uri::compose (p), "");
}
// NOLINTNEXTLINE
TEST (UriCompose, Scheme) {
  uri::parts p;
  p.scheme = "file";
  EXPECT_EQ (uri::compose (p), "file:");
}
// NOLINTNEXTLINE
TEST (UriCompose, Authority) {
  uri::parts p;
  p.authority.emplace ();
  p.authority->userinfo = "username";
  p.authority->host = "host";
  p.authority->port = "123";
  auto const expected = "//username@host:123"sv;
  EXPECT_EQ (uri::compose (p), expected);
  EXPECT_EQ (uri::split_reference (expected), p);
}
// NOLINTNEXTLINE
TEST (UriCompose, AbsolutePath) {
  uri::parts p;
  p.path.absolute = true;
  p.path.segments.emplace_back ("a");
  p.path.segments.emplace_back ("b");
  p.path.segments.emplace_back ();
  auto const expected = "/a/b/"sv;
  EXPECT_EQ (uri::compose (p), expected);
  EXPECT_EQ (uri::split_reference (expected), p);
}
// NOLINTNEXTLINE
TEST (UriCompose, RelativePath) {
  uri::parts p;
  p.path.segments.emplace_back ("a");
  p.path.segments.emplace_back ("b");
  p.path.segments.emplace_back ();
  auto const expected = "a/b/"sv;
  EXPECT_EQ (uri::compose (p), expected);
  EXPECT_EQ (uri::split_reference (expected), p);
}
// NOLINTNEXTLINE
TEST (UriCompose, Query) {
  uri::parts p;
  p.query = "query";
  auto const expected = "?query"sv;
  EXPECT_EQ (uri::compose (p), expected);
  EXPECT_EQ (uri::split_reference (expected), p);
}
// NOLINTNEXTLINE
TEST (UriCompose, Fragment) {
  uri::parts p;
  p.fragment = "fragment";
  auto const expected = "#fragment"sv;
  EXPECT_EQ (uri::compose (p), expected);
  EXPECT_EQ (uri::split_reference (expected), p);
}
// NOLINTNEXTLINE
TEST (UriCompose, EmptyStrings) {
  uri::parts p;
  p.authority.emplace ();
  p.authority->userinfo = "";
  p.authority->host = "foo.com";
  p.authority->port = "";
  p.path.segments.emplace_back ("segment");
  p.query = "";
  p.fragment = "";
  std::string const& c = uri::compose (p);
  auto const& p2 = uri::split_reference (c);
  ASSERT_TRUE (p2);
  EXPECT_EQ (p, *p2);
}

#if URI_FUZZTEST
static void SplitComposeEqual (std::string const& s) {
  if (auto const& p = uri::split (s)) {
    std::string const& s2 = uri::compose (*p);
    EXPECT_EQ (s2, s);
  }
}
// NOLINTNEXTLINE
FUZZ_TEST (UriCompose, SplitComposeEqual);
#endif  // URI_FUZZTEST

// NOLINTNEXTLINE
TEST (PartsValid, SchemeEmpty) {
  uri::parts p;
  EXPECT_FALSE (p.valid ()) << "Scheme must have at least one leading ALPHA";
}

// NOLINTNEXTLINE
TEST (PartsValid, SchemeSimple) {
  uri::parts p;
  p.scheme = "scheme";
  EXPECT_TRUE (p.valid ());
}

// NOLINTNEXTLINE
TEST (PartsValid, SchemeLeadingDigit) {
  uri::parts p;
  p.scheme = "123";
  EXPECT_FALSE (p.valid ()) << "Scheme must have at least one leading ALPHA";
}

// NOLINTNEXTLINE
TEST (PartsValid, SchemeMixedCharacters) {
  uri::parts p;
  p.scheme = "a+123";
  EXPECT_TRUE (p.valid ())
    << R"!(scheme must be ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ))!";
}

// NOLINTNEXTLINE
TEST (PartsValid, AuthorityUserinfo) {
  using parts_authority = struct uri::parts::authority;
  uri::parts p;
  p.scheme = "scheme";
  p.authority = parts_authority{"userinfo"sv, "host"sv, std::nullopt};
  EXPECT_TRUE (p.valid ());
  EXPECT_EQ (p.valid (), uri::split (uri::compose (p)).has_value ());
}
