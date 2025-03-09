//===- unittests/uri/test_parts.cpp ---------------------------------------===//
//*                   _        *
//*  _ __   __ _ _ __| |_ ___  *
//* | '_ \ / _` | '__| __/ __| *
//* | |_) | (_| | |  | |_\__ \ *
//* | .__/ \__,_|_|   \__|___/ *
//* |_|                        *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//

#include <gmock/gmock.h>

#include "peejay/uri/parts.hpp"

#if URI_FUZZTEST
#include <fuzztest/fuzztest.h>
#endif

using namespace std::string_view_literals;

// NOLINTNEXTLINE
TEST(Parts, PunyEncodedSizeNoNonAscii) {
  EXPECT_EQ(uri::details::puny_encoded_size(u8"a.b"sv | icubaby::views::transcode<char8_t, char32_t>), std::size_t{0});
}

// NOLINTNEXTLINE
TEST(Parts, PunyDecodedSizeNoNonAscii) {
  std::variant<std::error_code, std::size_t> const puny_decoded_result = uri::details::puny_decoded_size("a.b"sv);
  ASSERT_TRUE(std::holds_alternative<std::size_t>(puny_decoded_result));
  EXPECT_EQ(std::get<std::size_t>(puny_decoded_result), std::size_t{0});
}

// NOLINTNEXTLINE
TEST(Parts, PunyEncodedThreeParts) {
  std::string output;
  uri::details::puny_encoded(u8"aaa.bbb.ccc"sv | icubaby::views::transcode<char8_t, char32_t>,
                             std::back_inserter(output));
  EXPECT_EQ(output, "aaa.bbb.ccc");
}

// NOLINTNEXTLINE
TEST(Parts, PunyEncodedMuchenDe) {
  std::string output;
  std::u32string const input{'M',
                             static_cast<char32_t>(0x00FC),  // U+00FC LATIN SMALL LETTER U WITH DIAERESIS
                             'n',
                             'c',
                             'h',
                             'e',
                             'n',
                             '.',
                             'd',
                             'e'};
  uri::details::puny_encoded(input, std::back_inserter(output));
  EXPECT_EQ(output, "xn--Mnchen-3ya.de");
}

// NOLINTNEXTLINE
TEST(Parts, PunyEncodedMuchenDotGrinningFace) {
  std::string output;
  // M<U+00FC LATIN SMALL LETTER U WITH DIAERESIS>nchen.<U+03C0 greek small
  // letter pi>
  auto const latin_small_letter_u_with_diaeresis = char32_t{0x00FC};
  auto const greek_small_letter_pi = char32_t{0x03C0};
  std::u32string const input{
      'M', latin_small_letter_u_with_diaeresis, 'n', 'c', 'h', 'e', 'n', '.', greek_small_letter_pi};
  uri::details::puny_encoded(input, std::back_inserter(output));
  EXPECT_EQ(output, "xn--Mnchen-3ya.xn--1xa");
}

// NOLINTNEXTLINE
TEST(Parts, PunyEncodedSizeMuchenDe) {
  auto const latin_small_letter_u_with_diaeresis = char32_t{0x00FC};
  std::u32string const input{'M', latin_small_letter_u_with_diaeresis, 'n', 'c', 'h', 'e', 'n', '.', 'd', 'e'};
  EXPECT_EQ(uri::details::puny_encoded_size(input), std::size_t{17});
}

// NOLINTNEXTLINE
TEST(Parts, PunyEncodedMunchenGrinningFace) {
  auto const latin_small_letter_u_with_diaeresis = char32_t{0x00FC};
  auto const grinning_face = char32_t{0x1F600};
  std::u32string const input32{'M', latin_small_letter_u_with_diaeresis, 'n', 'c', 'h', 'e', 'n', '.', grinning_face};
  std::string input;
  std::ranges::copy(input32 | icubaby::views::transcode<char32_t, char8_t> |
                        std::views::transform([](char8_t const v) { return static_cast<char>(v); }),
                    std::back_inserter(input));
  uri::parts p;
  using auth = struct uri::parts::authority;
  p.authority = auth{std::nullopt, std::bit_cast<char const*>(input.data()), std::nullopt};
  std::vector<char> store;
  uri::parts const encoded_parts = uri::encode(store, p);
  EXPECT_EQ(encoded_parts.authority->host, "xn--Mnchen-3ya.xn--e28h");
}

// NOLINTNEXTLINE
TEST(Parts, PunyDecoded) {
  std::u32string output;
  auto out = std::back_inserter(output);
  auto const input = std::string_view{"aaa.bbb.ccc"};

  auto const puny_decoded_result = uri::details::puny_decoded(input, out);

  using result_type = uri::details::puny_decoded_result<decltype(out)>;
  ASSERT_TRUE(std::holds_alternative<result_type>(puny_decoded_result));
  auto const& result = std::get<result_type>(puny_decoded_result);
  EXPECT_FALSE(result.any_encoded);
  EXPECT_THAT(output, testing::ElementsAre(char32_t{'a'}, char32_t{'a'}, char32_t{'a'}, char32_t{'.'}, char32_t{'b'},
                                           char32_t{'b'}, char32_t{'b'}, char32_t{'.'}, char32_t{'c'}, char32_t{'c'},
                                           char32_t{'c'}));
}

// NOLINTNEXTLINE
TEST(Parts, PunyDecodedMuchenDe) {
  std::vector<char8_t> output;
  auto out = std::back_inserter(output);
  auto const input = std::string_view{"xn--Mnchen-3ya.de"};
  auto const puny_decoded_result = uri::details::puny_decoded(input, out);

  using result_type = uri::details::puny_decoded_result<decltype(out)>;
  ASSERT_TRUE(std::holds_alternative<result_type>(puny_decoded_result));
  auto const& result = std::get<result_type>(puny_decoded_result);
  EXPECT_TRUE(result.any_encoded);

  // latin-small-letter-u-with-diaresis is U+00FC (C3 BC)
  EXPECT_THAT(output,
              testing::ElementsAre('M', static_cast<char8_t>(0xC3), static_cast<char8_t>(0xBC), 'n', char8_t{'c'},
                                   char8_t{'h'}, char8_t{'e'}, char8_t{'n'}, char8_t{'.'}, char8_t{'d'}, char8_t{'e'}));
}

// NOLINTNEXTLINE
TEST(Parts, AllSetButNothingToEncode) {
  uri::parts input;
  using auth = struct uri::parts::authority;
  input.scheme = "https"sv;
  input.authority = auth{"user"sv, "host"sv, "1234"sv};
  input.path.absolute = true;
  input.path.segments = std::vector<std::string_view>{"a"sv, "b"sv};
  input.query = "query"sv;
  input.fragment = "fragment"sv;
  ASSERT_TRUE(input.valid());

  std::vector<char> store;
  uri::parts const output = uri::encode(store, input);

  EXPECT_TRUE(output.valid());
  EXPECT_EQ(store.size(), 0U) << "Nothing to encode so store size should be 0";
  EXPECT_EQ(output.scheme, input.scheme);
  ASSERT_TRUE(output.authority.has_value());
  EXPECT_EQ(output.authority->userinfo, input.authority->userinfo);
  EXPECT_EQ(output.authority->host, input.authority->host);
  EXPECT_EQ(output.authority->port, input.authority->port);
  EXPECT_EQ(output.path.absolute, input.path.absolute);
  EXPECT_THAT(output.path.segments, testing::ContainerEq(input.path.segments));
  ASSERT_TRUE(output.query.has_value());
  EXPECT_EQ(*output.query, *input.query);
  ASSERT_TRUE(output.fragment.has_value());
  EXPECT_EQ(*output.fragment, *input.fragment);
}

// NOLINTNEXTLINE
TEST(Parts, EncodeDecode) {
  uri::parts original;
  using auth = struct uri::parts::authority;
  original.scheme = "https"sv;
  original.authority = auth{"user"sv, "M\xC3\xBCnchen.de"sv, "1234"sv};
  original.path.absolute = true;
  original.path.segments = std::vector<std::string_view>{"~\xC2\xA1"sv};
  original.query = "a%b"sv;
  original.fragment = "c%d"sv;

  std::vector<char> encode_store;
  uri::parts const encoded = uri::encode(encode_store, original);
  EXPECT_TRUE(encoded.valid());

  std::vector<char> decode_store;
  std::variant<std::error_code, uri::parts> const decode_result = uri::decode(decode_store, encoded);
  ASSERT_TRUE(std::holds_alternative<uri::parts>(decode_result));

  auto const& decoded = std::get<uri::parts>(decode_result);
  EXPECT_EQ(decoded.scheme, original.scheme);
  ASSERT_TRUE(decoded.authority.has_value());
  EXPECT_EQ(decoded.authority->userinfo, original.authority->userinfo);
  EXPECT_EQ(decoded.authority->host, original.authority->host);
  EXPECT_EQ(decoded.authority->port, original.authority->port);
  EXPECT_EQ(decoded.path.absolute, original.path.absolute);
  EXPECT_THAT(decoded.path.segments, testing::ContainerEq(original.path.segments));
  ASSERT_TRUE(decoded.query.has_value());
  EXPECT_EQ(*decoded.query, *original.query);
  ASSERT_TRUE(decoded.fragment.has_value());
  EXPECT_EQ(*decoded.fragment, *original.fragment);
}

// NOLINTNEXTLINE
TEST(Parts, EncodeDecodePunycodeTLD) {
  uri::parts original;
  using auth = struct uri::parts::authority;
  original.scheme = "http"sv;
  // CYRILLIC SMALL LETTER IO
  // CYRILLIC SMALL LETTER ZHE
  // CYRILLIC SMALL LETTER I
  // CYRILLIC SMALL LETTER KA
  //
  // CYRILLIC SMALL LETTER ER
  // CYRILLIC SMALL LETTER EF
  original.authority = auth{std::nullopt, "ёжик.рф"sv, std::nullopt};

  std::vector<char> encode_store;
  uri::parts const encoded = uri::encode(encode_store, original);
  EXPECT_TRUE(encoded.valid());

  std::vector<char> decode_store;
  std::variant<std::error_code, uri::parts> const decode_result = uri::decode(decode_store, encoded);
  ASSERT_TRUE(std::holds_alternative<uri::parts>(decode_result));

  auto const& decoded = std::get<uri::parts>(decode_result);
  EXPECT_EQ(decoded.scheme, original.scheme);
  ASSERT_TRUE(decoded.authority.has_value());
  EXPECT_EQ(decoded.authority->userinfo, original.authority->userinfo);
  EXPECT_EQ(decoded.authority->host, original.authority->host);
  EXPECT_EQ(decoded.authority->port, original.authority->port);
  EXPECT_EQ(decoded.path.absolute, original.path.absolute);
  ASSERT_FALSE(decoded.query.has_value());
  ASSERT_FALSE(decoded.fragment.has_value());
}

// NOLINTNEXTLINE
TEST(Parts, DecodeBadPunycodeTLD) {
  uri::parts encoded;
  using auth = struct uri::parts::authority;
  encoded.scheme = "http"sv;
  encoded.authority = auth{std::nullopt, "xn--ё"sv, std::nullopt};

  std::vector<char> decode_store;
  std::variant<std::error_code, uri::parts> const decode_result = uri::decode(decode_store, encoded);
  ASSERT_TRUE(std::holds_alternative<std::error_code>(decode_result));

  EXPECT_EQ(std::get<std::error_code>(decode_result), make_error_code(uri::punycode::decode_error_code::bad_input));
}

using opt_authority = std::optional<struct uri::parts::authority>;

struct parts_without_authority {
  std::optional<std::string> scheme;
  struct uri::parts::path path;
  std::optional<std::string> query;
  std::optional<std::string> fragment;

  [[nodiscard]] uri::parts as_parts(opt_authority&& auth) const {
    // assert (!auth || auth->host.data () != nullptr);
    return uri::parts{scheme, std::move(auth), path, query, fragment};
  }
};

#if URI_FUZZTEST

static void EncodeAndComposeValidAlwaysAgree(parts_without_authority const& base, opt_authority&& auth) {
  std::vector<char> store;
  if (uri::parts const p = uri::encode(store, base.as_parts(std::move(auth))); p.valid()) {
    std::string const str = uri::compose(p);
    EXPECT_TRUE(uri::split(str).has_value());
  }
}
// NOLINTNEXTLINE
FUZZ_TEST(Parts, EncodeAndComposeValidAlwaysAgree);

#endif  // URI_FUZZTEST

static void EncodeDecodeRoundTrip(parts_without_authority const& base, opt_authority&& auth) {
  if (auth && (auth->host.starts_with("xn--") || auth->host.find(".xn--") != std::string_view::npos)) {
    return;
  }
  if (uri::parts const original = base.as_parts(std::move(auth)); original.valid()) {
    std::vector<char> encode_store;
    uri::parts const encoded = uri::encode(encode_store, original);
    EXPECT_TRUE(encoded.valid());

    std::vector<char> decode_store;
    std::variant<std::error_code, uri::parts> const decode_result = uri::decode(decode_store, encoded);
    ASSERT_TRUE(std::holds_alternative<uri::parts>(decode_result));

    auto const& decoded = std::get<uri::parts>(decode_result);
    EXPECT_EQ(decoded.scheme, original.scheme);
    ASSERT_EQ(decoded.authority.has_value(), original.authority.has_value());
    if (decoded.authority && original.authority) {
      EXPECT_EQ(decoded.authority->userinfo, original.authority->userinfo);
      EXPECT_EQ(decoded.authority->host, original.authority->host);
      EXPECT_EQ(decoded.authority->port, original.authority->port);
    }
    EXPECT_EQ(decoded.path.absolute, original.path.absolute);
    EXPECT_THAT(decoded.path.segments, testing::ContainerEq(original.path.segments));
    EXPECT_EQ(decoded.query, original.query);
    EXPECT_EQ(decoded.fragment, original.fragment);
  }
}

#if URI_FUZZTEST
// NOLINTNEXTLINE
FUZZ_TEST(Parts, EncodeDecodeRoundTrip);
#endif  // URI_FUZZTEST

TEST(Parts, EncodeDecodeRoundTripRegression2) {
  using auth = struct uri::parts::authority;
  EncodeDecodeRoundTrip({"A", {false, {}}, std::nullopt, std::nullopt}, auth{std::nullopt, ".xn--", std::nullopt});
}

TEST(Parts, EncodeDecodeRoundTripRegression3) {
  using auth = struct uri::parts::authority;
  EncodeDecodeRoundTrip({"U", {true, {"ffffffffffffffffffffffffffff", "k%fff"}}, "", std::nullopt},
                        auth{"", "b", std::nullopt});
}

TEST(Parts, EncodeDecodeRoundTripManyPathElements) {
  using auth = struct uri::parts::authority;
  std::vector<std::string_view> elements{
      "el0",  "el%1",  "el2",  "el%3",  "el4",  "el%5",  "el6",  "el%7",  "el8",  "el%9",  "el10", "el%11",
      "el12", "el%13", "el14", "el%15", "el16", "el%17", "el18", "el%19", "el20", "el%21", "el22", "el%23",
      "el24", "el%25", "el26", "el%27", "el28", "el%29", "el30", "el%31", "el32", "el%33", "el34", "el%35",
      "el36", "el%37", "el38", "el%39", "el40", "el%41", "el42", "el%43", "el44", "el%45", "el46", "el%47"};
  EncodeDecodeRoundTrip({std::nullopt, {true, std::move(elements)}, std::nullopt, std::nullopt},
                        auth{std::nullopt, "host", std::nullopt});
}
