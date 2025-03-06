//===- lib/uri/parts.cpp --------------------------------------------------===//
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
#include "uri/parts.hpp"

#include <concepts>
#include <iterator>

#include "uri/icubaby.hpp"
#include "uri/pctdecode.hpp"
#include "uri/pctencode.hpp"

namespace uri::details {

std::size_t pct_encoded_size (std::string_view const str,
                              pctencode_set const encodeset) {
  if (!uri::needs_pctencode (str, encodeset)) {
    return std::size_t{0};
  }
  ro_sink_container<char> sink;
  uri::pctencode (std::begin (str), std::end (str), std::back_inserter (sink),
                  encodeset);
  return sink.size ();
}

std::size_t pct_decoded_size (std::string_view const str) {
  if (!uri::needs_pctdecode (str.begin (), str.end ())) {
    return std::size_t{0};
  }
  return uri::pctdecode (str).size ();
}

}  // end namespace uri::details
