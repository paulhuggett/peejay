//===- unittests/uri/test_punycode.cpp ------------------------------------===//
//*                                        _       *
//*  _ __  _   _ _ __  _   _  ___ ___   __| | ___  *
//* | '_ \| | | | '_ \| | | |/ __/ _ \ / _` |/ _ \ *
//* | |_) | |_| | | | | |_| | (_| (_) | (_| |  __/ *
//* | .__/ \__,_|_| |_|\__, |\___\___/ \__,_|\___| *
//* |_|                |___/                       *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gmock/gmock.h>

#include "uri/punycode.hpp"

#if URI_FUZZTEST
#include "fuzztest/fuzztest.h"
#endif

using namespace std::string_view_literals;

constexpr auto decoded_success_result_index = std::size_t{1};

// NOLINTNEXTLINE
TEST (Punycode, Empty) {
  auto const orig = std::u32string{};
  auto const encoded = ""sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  static_assert (std::is_same_v<
                 uri::punycode::decode_success_result<std::ranges::iterator_t<std::remove_const_t<decltype (encoded)>>>,
                 std::variant_alternative_t<decoded_success_result_index, std::remove_const_t<decltype (decoded)>>>);

  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in, std::end (encoded));
}
// NOLINTNEXTLINE
TEST (Punycode, AsciiNoPlain) {
  auto const orig = std::u32string{'A', 'b'};
  auto const encoded = "Ab-"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  static_assert (
    std::is_same_v<
      uri::punycode::decode_success_result<
        std::ranges::iterator_t<std::remove_const_t<decltype (encoded)>>>,
      std::variant_alternative_t<decoded_success_result_index,
                                 std::remove_const_t<decltype (decoded)>>>);

  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, AsciiWithPlainAllowed) {
  auto const orig = std::u32string{'A', 'b'};
  auto const encoded = "Ab"sv;
  std::string actual;
  uri::punycode::encode (orig, true, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);
}

// NOLINTNEXTLINE
TEST (Punycode, Delimiter) {
  auto const orig = std::u32string{
    ',',     // U+002C COMMA
    '-',     // U+002D HYPHEN-MINUS
    0x1BC0,  // U+01BC0 BATAK LETTER A
  };
  auto const encoded = ",--9cr"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, DecodeDelimiterCaps) {
  constexpr auto batak_letter_a = char32_t{0x1BC0};
  auto const orig = std::u32string{',', '-', batak_letter_a};

  auto const encoded = ",--9CR"sv;
  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, ArabicEgyptian) {
  // Arabic (Egyptian)
  auto const arabic = std::u32string{
    0x0644, 0x064A, 0x0647, 0x0645, 0x0627, 0x0628, 0x062A, 0x0643, 0x0644,
    0x0645, 0x0648, 0x0634, 0x0639, 0x0631, 0x0628, 0x064A, 0x061F};
  auto const encoded_arabic = "egbpdaj6bu4bxfgehfvwxn"sv;
  std::string actual;
  uri::punycode::encode (arabic, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded_arabic);

  auto const decoded = uri::punycode::decode (encoded_arabic);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, arabic);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded_arabic));
}

// NOLINTNEXTLINE
TEST (Punycode, ChineseSimplified) {
  // Chinese (simplified)
  auto const chinese_simplified = std::u32string{
    0x4ED6, 0x4EEC, 0x4E3A, 0x4EC0, 0x4E48, 0x4E0D, 0x8BF4, 0x4E2D, 0x6587};
  auto const encoded_chinese_simplified = "ihqwcrb4cv8a8dqg056pqjye"sv;
  std::string actual;
  uri::punycode::encode (chinese_simplified, false,
                         std::back_inserter (actual));
  EXPECT_EQ (actual, encoded_chinese_simplified);

  auto const decoded = uri::punycode::decode (encoded_chinese_simplified);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str,
             chinese_simplified);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded_chinese_simplified));
}

// NOLINTNEXTLINE
TEST (Punycode, ChineseTraditional) {
  // Chinese (traditional)
  auto const orig = std::u32string{0x4ED6, 0x5011, 0x7232, 0x4EC0, 0x9EBD,
                                   0x4E0D, 0x8AAA, 0x4E2D, 0x6587};
  auto const encoded = "ihqwctvzc91f659drss3x8bo0yb"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, Czech) {
  // Czech: Pro<ccaron>prost<ecaron>nemluv<iacute><ccaron>esky
  auto const orig = std::u32string{
    0x0050, 0x0072, 0x006F, 0x010D, 0x0070, 0x0072, 0x006F, 0x0073,
    0x0074, 0x011B, 0x006E, 0x0065, 0x006D, 0x006C, 0x0075, 0x0076,
    0x00ED, 0x010D, 0x0065, 0x0073, 0x006B, 0x0079};
  auto const encoded = "Proprostnemluvesky-uyb24dma41a"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, Hebrew) {
  auto const orig = std::u32string{
    0x05DC, 0x05DE, 0x05D4, 0x05D4, 0x05DD, 0x05E4, 0x05E9, 0x05D5,
    0x05D8, 0x05DC, 0x05D0, 0x05DE, 0x05D3, 0x05D1, 0x05E8, 0x05D9,
    0x05DD, 0x05E2, 0x05D1, 0x05E8, 0x05D9, 0x05EA};
  auto const encoded = "4dbcagdahymbxekheh6e0a7fei0b"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, HindiDevanagari) {
  auto const orig = std::u32string{
    0x092F, 0x0939, 0x0932, 0x094B, 0x0917, 0x0939, 0x093F, 0x0928,
    0x094D, 0x0926, 0x0940, 0x0915, 0x094D, 0x092F, 0x094B, 0x0902,
    0x0928, 0x0939, 0x0940, 0x0902, 0x092C, 0x094B, 0x0932, 0x0938,
    0x0915, 0x0924, 0x0947, 0x0939, 0x0948, 0x0902};
  auto const encoded = "i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, JapaneseKanjiAndHiragana) {
  auto const orig = std::u32string{
    0x306A, 0x305C, 0x307F, 0x3093, 0x306A, 0x65E5, 0x672C, 0x8A9E, 0x3092,
    0x8A71, 0x3057, 0x3066, 0x304F, 0x308C, 0x306A, 0x3044, 0x306E, 0x304B};
  auto const encoded = "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, KoreanHangulSyllables) {
  auto const orig = std::u32string{
    0xC138, 0xACC4, 0xC758, 0xBAA8, 0xB4E0, 0xC0AC, 0xB78C, 0xB4E4,
    0xC774, 0xD55C, 0xAD6D, 0xC5B4, 0xB97C, 0xC774, 0xD574, 0xD55C,
    0xB2E4, 0xBA74, 0xC5BC, 0xB9C8, 0xB098, 0xC88B, 0xC744, 0xAE4C};
  auto const encoded =
    "989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5jpsd879ccm6fea98c"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, RussianCyrillic) {
  auto const orig =
    std::u32string{0x043F, 0x043E, 0x0447, 0x0435, 0x043C, 0x0443, 0x0436,
                   0x0435, 0x043E, 0x043D, 0x0438, 0x043D, 0x0435, 0x0433,
                   0x043E, 0x0432, 0x043E, 0x0440, 0x044F, 0x0442, 0x043F,
                   0x043E, 0x0440, 0x0443, 0x0441, 0x0441, 0x043A, 0x0438};
  auto const encoded = "b1abfaaepdrnnbgefbadotcwatmq2g4l"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, Spanish) {
  // Porqu<eacute>nopuedensimplementehablarenEspa<ntilde>ol
  auto const orig = std::u32string{
    0x0050, 0x006F, 0x0072, 0x0071, 0x0075, 0x00E9, 0x006E, 0x006F,
    0x0070, 0x0075, 0x0065, 0x0064, 0x0065, 0x006E, 0x0073, 0x0069,
    0x006D, 0x0070, 0x006C, 0x0065, 0x006D, 0x0065, 0x006E, 0x0074,
    0x0065, 0x0068, 0x0061, 0x0062, 0x006C, 0x0061, 0x0072, 0x0065,
    0x006E, 0x0045, 0x0073, 0x0070, 0x0061, 0x00F1, 0x006F, 0x006C};
  auto const encoded = "PorqunopuedensimplementehablarenEspaol-fmd56a"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, Vietnamese) {
  // T<adotbelow>isaoh<odotbelow>kh<ocirc>ngth<ecirchookabove>ch
  // <ihookabove>n<oacute>iti<ecircacute>ngVi<ecircdotbelow>t
  auto const orig = std::u32string{
    0x0054, 0x1EA1, 0x0069, 0x0073, 0x0061, 0x006F, 0x0068, 0x1ECD,
    0x006B, 0x0068, 0x00F4, 0x006E, 0x0067, 0x0074, 0x0068, 0x1EC3,
    0x0063, 0x0068, 0x1EC9, 0x006E, 0x00F3, 0x0069, 0x0074, 0x0069,
    0x1EBF, 0x006E, 0x0067, 0x0056, 0x0069, 0x1EC7, 0x0074};
  auto const encoded = "TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, ExampleL) {
  // 3<nen>B<gumi><kinpachi><sensei>
  auto const orig = std::u32string{0x0033, 0x5E74, 0x0042, 0x7D44,
                                   0x91D1, 0x516B, 0x5148, 0x751F};
  auto const encoded = "3B-ww4c5e180e575a65lsy2b"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, ExampleM) {
  // <amuro><namie>-with-SUPER-MONKEYS
  auto const orig = std::u32string{
    0x5B89, 0x5BA4, 0x5948, 0x7F8E, 0x6075, 0x002D, 0x0077, 0x0069,
    0x0074, 0x0068, 0x002D, 0x0053, 0x0055, 0x0050, 0x0045, 0x0052,
    0x002D, 0x004D, 0x004F, 0x004E, 0x004B, 0x0045, 0x0059, 0x0053};
  auto const encoded = "-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, ExampleN) {
  // Hello-Another-Way-<sorezore><no><basho>
  auto const orig = std::u32string{
    0x0048, 0x0065, 0x006C, 0x006C, 0x006F, 0x002D, 0x0041, 0x006E, 0x006F,
    0x0074, 0x0068, 0x0065, 0x0072, 0x002D, 0x0057, 0x0061, 0x0079, 0x002D,
    0x305D, 0x308C, 0x305E, 0x308C, 0x306E, 0x5834, 0x6240};
  auto const encoded = "Hello-Another-Way--fc4qua05auwb3674vfr0b"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, ExampleO) {
  // <hitotsu><yane><no><shita>2
  auto const orig = std::u32string{0x3072, 0x3068, 0x3064, 0x5C4B,
                                   0x6839, 0x306E, 0x4E0B, 0x0032};
  auto const encoded = "2-u9tlzr9756bt3uc0v"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, ExampleP) {
  // Maji<de>Koi<suru>5<byou><mae>
  auto const orig =
    std::u32string{0x004D, 0x0061, 0x006A, 0x0069, 0x3067, 0x004B, 0x006F,
                   0x0069, 0x3059, 0x308B, 0x0035, 0x79D2, 0x524D};
  auto const encoded = "MajiKoi5-783gue6qz075azm5e"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, ExampleQ) {
  //  <pafii>de<runba>
  auto const orig = std::u32string{0x30D1, 0x30D5, 0x30A3, 0x30FC, 0x0064,
                                   0x0065, 0x30EB, 0x30F3, 0x30D0};
  auto const encoded = "de-jg4avhby1noc0d"sv;
  std::string actual;
  uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, ExampleR) {
  // <sono><supiido><de>
  auto const orig =
    std::u32string{0x305D, 0x306E, 0x30B9, 0x30D4, 0x30FC, 0x30C9, 0x3067};
  auto const encoded = "d9juau41awczczp"sv;
  std::string actual;
  auto const [_1, _2, has_non_basic] =
    uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);
  EXPECT_TRUE (has_non_basic);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, ExampleS) {
  // (S) -> $1.00 <-
  auto const orig =
    std::u32string{'-', '>', ' ', '$', '1', '.', '0', '0', ' ', '<', '-'};
  auto const encoded = "-> $1.00 <--"sv;
  std::string actual;
  auto const [_1, _2, has_non_basic] =
    uri::punycode::encode (orig, false, std::back_inserter (actual));
  EXPECT_EQ (actual, encoded);
  EXPECT_FALSE (has_non_basic);

  auto const decoded = uri::punycode::decode (encoded);
  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, orig);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

// NOLINTNEXTLINE
TEST (Punycode, BadInput) {
  auto const decoded = uri::punycode::decode ("eg{|}");
  ASSERT_EQ (decoded.index (), 0U);
  EXPECT_EQ (std::get<0U> (decoded),
             make_error_code (uri::punycode::decode_error_code::bad_input));
}

// NOLINTNEXTLINE
TEST (Punycode, BadInputInPlainAsciiPart) {
  auto const decoded = uri::punycode::decode ("\x80-eg");
  ASSERT_EQ (decoded.index (), 0U);
  EXPECT_EQ (std::get<0U> (decoded),
             make_error_code (uri::punycode::decode_error_code::bad_input));
}

#if URI_FUZZTEST
static void EncodeNeverCrashes (std::u32string const& s) {
  std::string actual;
  uri::punycode::encode (s, false, std::back_inserter (actual));
}
// NOLINTNEXTLINE
FUZZ_TEST (Punycode, EncodeNeverCrashes);

static void DecodeNeverCrashes (std::string const& s) {
  uri::punycode::decode (s);
}
// NOLINTNEXTLINE
FUZZ_TEST (Punycode, DecodeNeverCrashes);
#endif  // URI_FUZZTEST

#if URI_FUZZTEST
namespace {

inline constexpr auto max_code_point = char32_t{0x10FFFF};
auto UnicodeCodePoint () {
  return fuzztest::InRange<char32_t> (0x0000, max_code_point);
}
auto U32String () {
  return fuzztest::ContainerOf<std::u32string> (UnicodeCodePoint ());
}
void EncodeDecodeRoundTrip (std::u32string const& s) {
  std::string encoded;
  uri::punycode::encode (s, false, std::back_inserter (encoded));

  auto const decoded = uri::punycode::decode (encoded);

  ASSERT_EQ (decoded.index (), decoded_success_result_index);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).str, s);
  EXPECT_EQ (std::get<decoded_success_result_index> (decoded).in,
             std::end (encoded));
}

}  // end anonymous namespace

// NOLINTNEXTLINE
FUZZ_TEST (Punycode, EncodeDecodeRoundTrip).WithDomains (U32String ());
#endif  // URI_FUZZTEST

namespace {

constexpr std::string_view::const_iterator ascii_part_end (std::string_view const encoded) {
  auto const r = uri::find_last (encoded, '-');
  return r.empty () ? encoded.begin () : r.begin ();
}

void DecodeEncodeRoundTrip (std::string_view original) {
  // A single leading delimiter can be safely ignored.
  if (std::ranges::count (original, '-') == 1 && original.starts_with ('-')) {
    original.remove_prefix (1);
  }
  auto const res = uri::punycode::decode (original);
  if (res.index () == decoded_success_result_index) {
    std::string encoded;
    uri::punycode::encode (std::get<decoded_success_result_index> (res).str,
                           false, std::back_inserter (encoded));

    auto const ascii_end = ascii_part_end (original);
    assert (ascii_end >= original.begin () && ascii_end <= original.end ());
    ASSERT_EQ (original.size (), encoded.size ());
    EXPECT_TRUE (
      std::equal (std::begin (original), ascii_end, std::begin (encoded)));
    EXPECT_TRUE ((std::equal (
      ascii_end, std::end (original),
      std::begin (encoded) + std::distance (std::begin (original), ascii_end),
      [] (char a, char b) { return std::tolower (a) == std::tolower (b); })));
  }
}

}  // end anonymous namespace

#if URI_FUZZTEST
// NOLINTNEXTLINE
FUZZ_TEST (Punycode, DecodeEncodeRoundTrip);
#endif  // URI_FUZZTEST

// NOLINTNEXTLINE
TEST (Punycode, DecodeEncodeRoundTripEmpty) {
  DecodeEncodeRoundTrip ("");
}
// NOLINTNEXTLINE
TEST (Punycode, DecodeEncodeRoundTripDash) {
  DecodeEncodeRoundTrip ("-");
}
// NOLINTNEXTLINE
TEST (Punycode, DecodeEncodeRoundTripLeadingDelimiter) {
  DecodeEncodeRoundTrip ("-Ssu");
}
// NOLINTNEXTLINE
TEST (Punycode, DecodeEncodeRoundTripTrailingDelimiter) {
  DecodeEncodeRoundTrip ("hello-");
}
