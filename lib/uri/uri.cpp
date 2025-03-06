//===- lib/uri/uri.cpp ----------------------------------------------------===//
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
#include "uri/uri.hpp"

#include <cassert>
#include <sstream>

#include "uri/pctencode.hpp"
#include "uri/rule.hpp"

using namespace uri;

namespace {

inline auto single_code_point (rule const& r, code_point const cp) {
  assert (static_cast<std::underlying_type_t<code_point>> (cp) <=
          static_cast<std::underlying_type_t<code_point>> (
            std::numeric_limits<char>::max ()));
  return r.single_char (static_cast<char> (cp));
}

inline auto code_point_range (code_point const first, code_point const last) {
  assert (static_cast<std::underlying_type_t<code_point>> (first) <=
          static_cast<std::underlying_type_t<code_point>> (
            std::numeric_limits<char>::max ()));
  assert (static_cast<std::underlying_type_t<code_point>> (last) <=
          static_cast<std::underlying_type_t<code_point>> (
            std::numeric_limits<char>::max ()));
  return [f = std::tolower (static_cast<int> (first)),
          l = std::tolower (static_cast<int> (last))] (rule const& r) {
    return r.single_char ([=] (char const c) {
      auto const cl = std::tolower (static_cast<int> (c));
      return cl >= f && cl <= l;
    });
  };
}

inline auto commercial_at (rule const& r) {
  return single_code_point (r, code_point::commercial_at);
}
inline auto colon (rule const& r) {
  return single_code_point (r, code_point::colon);
}
inline auto hash (rule const& r) {
  return single_code_point (r, code_point::number_sign);
}
inline auto plus (rule const& r) {
  return single_code_point (r, code_point::plus_sign);
}
inline auto minus (rule const& r) {
  return single_code_point (r, code_point::hyphen_minus);
}
inline auto solidus (rule const& r) {
  return single_code_point (r, code_point::solidus);
}
inline auto question_mark (rule const& r) {
  return single_code_point (r, code_point::question_mark);
}
inline auto full_stop (rule const& r) {
  return single_code_point (r, code_point::full_stop);
}
inline auto left_square_bracket (rule const& r) {
  return single_code_point (r, code_point::left_square_bracket);
}
inline auto right_square_bracket (rule const& r) {
  return single_code_point (r, code_point::right_square_bracket);
}
inline auto percent_sign (rule const& r) {
  return single_code_point (r, code_point::percent_sign);
}
inline auto digit_one (rule const& r) {
  return single_code_point (r, code_point::digit_one);
}
inline auto digit_two (rule const& r) {
  return single_code_point (r, code_point::digit_two);
}
inline auto digit_five (rule const& r) {
  return single_code_point (r, code_point::digit_five);
}
inline auto latin_small_letter_v (rule const& r) {
  return single_code_point (r, code_point::latin_small_letter_v);
}

auto single_colon (rule const& r) {
  return r.concat (colon)
    .concat ([] (rule const& r1) -> rule::matched_result {
      if (auto const& sv = r1.tail ()) {
        if (sv->empty () || sv->front () != ':') {
          return std::make_tuple (sv->substr (0, 0),
                                  rule::acceptor_container{});
        }
      }
      return {};
    })
    .matched ("single-colon", r);
}

// sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//               / "*" / "+" / "," / ";" / "="
auto sub_delims (rule const& r) {
  return r.single_char ([] (char const c) {
    auto const cp = static_cast<code_point> (c);
    return cp == code_point::exclamation_mark ||
           cp == code_point::dollar_sign || cp == code_point::ampersand ||
           cp == code_point::apostrophe || cp == code_point::left_parenthesis ||
           cp == code_point::right_parenthesis || cp == code_point::asterisk ||
           cp == code_point::plus_sign || cp == code_point::comma ||
           cp == code_point::semi_colon || cp == code_point::equals_sign;
  });
}

// unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
auto unreserved (rule const& r) {
  return r.single_char ([] (char const c) {
    auto const cp = static_cast<code_point> (c);
    return static_cast<bool> (std::isalnum (static_cast<int> (c))) ||
           cp == code_point::hyphen_minus || cp == code_point::full_stop ||
           cp == code_point::low_line || cp == code_point::tilde;
  });
}

// pct-encoded   = "%" HEXDIG HEXDIG
auto pct_encoded (rule const& r) {
  return r.concat (percent_sign)
    .concat (hexdig)
    .concat (hexdig)
    .matched ("pct-encoded", r);
}

// pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
auto pchar (rule const& r) {
  return r
    .alternative (unreserved, pct_encoded, sub_delims, colon, commercial_at)
    .matched ("pchar", r);
}

// userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
auto userinfo (rule const& r) {
  return r
    .star ([] (rule const& r2) {
      return r2.alternative (unreserved, pct_encoded, sub_delims, colon)
        .matched ("userinfo/*", r2);
    })
    .matched ("userinfo", r);
}
constexpr auto userinfofn = userinfo;

// scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
auto scheme (rule const& r) {
  return r.concat (alpha)
    .star ([] (rule const& r2) {
      return r2.alternative (alpha, digit, plus, minus, full_stop)
        .matched ("scheme/*", r2);
    })
    .matched ("scheme", r);
}
constexpr auto schemefn = scheme;

// reg-name      = *( unreserved / pct-encoded / sub-delims )
auto reg_name (rule const& r) {
  return r
    .star ([] (rule const& r1) {
      return r1.alternative (unreserved, pct_encoded, sub_delims)
        .matched ("reg-name/*", r1);
    })
    .matched ("reg-name", r);
}

// dec-octet     = DIGIT                 ; 0-9
//               / %x31-39 DIGIT         ; 10-99
//               / "1" 2DIGIT            ; 100-199
//               / "2" %x30-34 DIGIT     ; 200-249
//               / "25" %x30-35          ; 250-255
auto dec_octet (rule const& r) {
  return r
    .alternative (
      [] (rule const& r4) {
        return r4               // 250-255
          .concat (digit_two)   // "2"
          .concat (digit_five)  // "5"
          .concat (code_point_range (code_point::digit_zero,
                                     code_point::digit_five))  // %x30-35
          .matched ("\"25\" %x30-35", r4);
      },
      [] (rule const& r3) {
        return r3              // 200-249
          .concat (digit_two)  // "2"
          .concat (code_point_range (code_point::digit_zero,
                                     code_point::digit_four))  // %x30-34
          .concat (digit)                                      // DIGIT
          .matched ("\"2\" %x30-34 DIGIT", r3);
      },
      [] (rule const& r2) {
        return r2              // 100-199
          .concat (digit_one)  // "1"
          .concat (digit)      // 2DIGIT
          .concat (digit)      // (...)
          .matched ("\"1\" 2DIGIT", r2);
      },
      [] (rule const& r1) {
        return r1  // 10-99
          .concat (code_point_range (code_point::digit_one,
                                     code_point::digit_nine))  // %x31-39
          .concat (digit)                                      // DIGIT
          .matched ("%x31-39 DIGIT", r1);
      },
      digit)
    .matched ("dec-octet", r);
}

// IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
auto ipv4address (rule const& r) {
  return r
    .concat (dec_octet)  // dec-octet
    .concat (full_stop)  // "."
    .concat (dec_octet)  // dec-octet
    .concat (full_stop)  // "."
    .concat (dec_octet)  // dec-octet
    .concat (full_stop)  // "."
    .concat (dec_octet)  // dec-octet
    .matched ("IPv4address", r);
}

// h16 = 1*4HEXDIG
auto h16 (rule const& r) {
  return r.star (hexdig, 1, 4).matched ("h16", r);
}

// h16colon = h16 ":"
auto h16_colon (rule const& r) {
  return r.concat (h16).concat (single_colon).matched ("h16:", r);
}
auto colon_colon (rule const& r) {
  return r.concat (colon).concat (colon).matched ("\"::\"", r);
}

// ls32          = ( h16 ":" h16 ) / IPv4address
auto ls32 (rule const& r) {
  return r
    .alternative (
      [] (rule const& r1) {
        return r1.concat (h16).concat (colon).concat (h16).matched ("h16:h16",
                                                                    r1);
      },
      ipv4address)
    .matched ("ls32", r);
}

// IPv6address =                            6( h16 ":" ) ls32 // r1
//             /                       "::" 5( h16 ":" ) ls32 // r2
//             / [               h16 ] "::" 4( h16 ":" ) ls32 // r3
//             / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32 // r4
//             / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32 // r5
//             / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32 // r6
//             / [ *4( h16 ":" ) h16 ] "::"              ls32 // r7
//             / [ *5( h16 ":" ) h16 ] "::"              h16  // r8
//             / [ *6( h16 ":" ) h16 ] "::"                   // r9
auto ipv6address (rule const& r) {
  return r
    .alternative (
      [] (rule const& r1) {
        // 6( h16 ":" ) ls32
        return r1.star (h16_colon, 6, 6)
          .concat (ls32)
          .matched ("6( h16: ) ls32", r1);
      },
      [] (rule const& r2) {
        // "::" 5( h16 ":" ) ls32
        return r2.concat (colon_colon)
          .star (h16_colon, 5, 5)
          .concat (ls32)
          .matched ("\"::\" 5( h16 colon ) ls32", r2);
      },
      [] (rule const& r3) {
        // [ h16 ] "::" 4( h16 ":" ) ls32
        return r3.optional (h16)
          .concat (colon_colon)
          .star (h16_colon, 4, 4)
          .concat (ls32)
          .matched ("[ h16 ] \"::\" 4( h16 colon ) ls32", r3);
      },
      [] (rule const& r4) {
        // [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
        return r4
          .optional ([] (rule const& r4a) {
            return r4a.star (h16_colon, 0, 1)
              .concat (h16)
              .matched ("*1( h16 colon ) h16", r4a);
          })
          .concat (colon_colon)
          .star (h16_colon, 3, 3)
          .concat (ls32)
          .matched ("[ *1( h16 colon ) h16 ] \"::\" 3( h16 colon ) ls32", r4);
      },
      [] (rule const& r5) {
        // [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
        return r5
          .optional ([] (rule const& r5a) {
            return r5a.star (h16_colon, 0, 2)
              .concat (h16)
              .matched ("*2( h16 colon ) h16", r5a);
          })
          .concat (colon_colon)
          .star (h16_colon, 2, 2)
          .concat (ls32)
          .matched ("[ *2( h16 colon ) h16 ] \"::\" 2( h16 colon ) ls32", r5);
      },
      [] (rule const& r6) {
        // [ *3( h16 ":" ) h16 ] "::" h16 ":" ls32
        return r6
          .optional ([] (rule const& r6a) {
            return r6a.star (h16_colon, 0, 3)
              .concat (h16)
              .matched ("*3( h16 colon ) h16", r6a);
          })
          .concat (colon_colon)
          .concat (h16_colon)
          .concat (ls32)
          .matched ("[ *3( h16 colon ) h16 ] \"::\" h16 colon ls32", r6);
      },
      [] (rule const& r7) {
        // [ *4( h16 ":" ) h16 ] "::" ls32
        return r7
          .optional ([] (rule const& r7a) {
            return r7a.star (h16_colon, 0, 4)
              .concat (h16)
              .matched ("*4( h16 colon ) h16", r7a);
          })
          .concat (colon_colon)
          .concat (ls32)
          .matched ("[ *4( h16 colon ) h16 ] \"::\" ls32", r7);
      },
      [] (rule const& r8) {
        // [ *5( h16 ":" ) h16 ] "::" h16
        return r8
          .optional ([] (rule const& r8a) {
            return r8a.star (h16_colon, 0, 5)
              .concat (h16)
              .matched ("*5( h16 colon ) h16", r8a);
          })
          .concat (colon_colon)
          .concat (h16)
          .matched ("[ *5( h16 colon ) h16 ] \"::\" h16", r8);
      },
      [] (rule const& r9) {
        // [ *6( h16 ":" ) h16 ] "::"
        return r9
          .optional ([] (rule const& r9a) {
            return r9a.star (h16_colon, 0, 6)
              .concat (h16)
              .matched ("*6( h16 colon ) h16", r9a);
          })
          .concat (colon_colon)
          .matched ("[ *6( h16 colon ) h16 ] \"::\"", r9);
      })
    .matched ("IPv6address", r);
}

// IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
auto ipvfuture (rule const& r) {
  return r.concat (latin_small_letter_v)
    .star (hexdig, 1)
    .concat (full_stop)
    .star (
      [] (rule const& r1) {
        return r1.alternative (unreserved, sub_delims, colon)
          .matched ("unreserved / sub-delims / colon", r1);
      },
      1)
    .matched ("IPvFuture", r);
}

// IP-literal    = "[" ( IPv6address / IPvFuture ) "]"
auto ip_literal (rule const& r) {
  return r.concat (left_square_bracket)
    .concat ([] (rule const& r1) {
      return r1.alternative (ipv6address, ipvfuture)
        .matched ("IPv6address / IPvFuture", r1);
    })
    .concat (right_square_bracket)
    .matched ("IP-literal", r);
}

// host = IP-literal / IPv4address / reg-name
auto host (rule const& r) {
  return r.alternative (ip_literal, ipv4address, reg_name)
    .matched ("IP-literal / IPv4address / reg-name", r);
}
constexpr auto hostfn = host;

auto host_rule (uri::parts& result) {
  return [&result] (rule const& r) {
    return r
      .concat (hostfn,
               [&result] (std::string_view host) {
                 result.ensure_authority ().host = host;
               })
      .matched ("host", r);
  };
}

auto userinfo_at (uri::parts& result) {
  // userinfo-at = userinfo "@"
  return [&result] (rule const& r) {
    return r
      .concat (userinfo,
               [&result] (std::string_view const userinfo) {
                 result.ensure_authority ().userinfo = userinfo;
               })
      .concat (commercial_at)
      .matched ("userinfo \"@\"", r);
  };
}

// port = *DIGIT
auto port (rule const& r) {
  return r.star (digit).matched ("port", r);
}
constexpr auto portfn = port;

auto colon_port (uri::parts& result) {
  // colon-port = ":" port
  return [&result] (rule const& r) {
    return r.concat (colon)
      .concat (port,
               [&result] (std::string_view const p) {
                 result.ensure_authority ().port = p;
               })
      .matched ("\":\" port", r);
  };
}

auto authority (uri::parts& result) {
  // authority = [ userinfo "@" ] host [ ":" port ]
  return [&result] (rule const& r) {
    return r.optional (userinfo_at (result))
      .concat (host_rule (result))
      .optional (colon_port (result))
      .matched ("authority", r);
  };
}

// segment       = *pchar
auto segment (rule const& r) {
  return r.star (pchar).matched ("segment", r);
}

// segment-nz    = 1*pchar
auto segment_nz (rule const& r) {
  return r.star (pchar, 1U).matched ("segment-nz", r);
}

// segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
//                  ; non-zero-length segment without any colon ":"
auto segment_nz_nc (rule const& r) {
  return r
    .star (
      [] (rule const& r2) {
        return r2
          .alternative (unreserved, pct_encoded, sub_delims, commercial_at)
          .matched ("unreserved / pct-encoded / sub-delims / \"@\"", r2);
      },
      1U)
    .matched ("segment-nz-nc", r);
}

template <bool IsAbs>
class append_dir {
public:
  explicit constexpr append_dir (uri::parts& result) noexcept
      : result_{result} {}
  void operator() (std::string_view const str) {
    (void)str;
    if constexpr (IsAbs) {
      result_.path.absolute = true;
    }
    result_.path.segments.emplace_back ();
  }

private:
  uri::parts& result_;
};

class append_segment {
public:
  explicit constexpr append_segment (uri::parts& result) noexcept
      : result_{result} {}
  void operator() (std::string_view const seg) {
    assert (!result_.path.segments.empty ());
    result_.path.segments.back () = seg;
  }

private:
  uri::parts& result_;
};

class record_initial_segment {
public:
  explicit constexpr record_initial_segment (uri::parts& result) noexcept
      : result_{result} {}
  void operator() (std::string_view const seg) {
    result_.path.segments.emplace_back (seg);
  }

private:
  uri::parts& result_;
};

auto path_abempty (uri::parts& result) {
  // path-abempty  = *( "/" segment )
  return [&result] (rule const& r) {
    return r
      .star ([&result] (rule const& r2) {
        return r2.concat (solidus, append_dir<true>{result})
          .concat (segment, append_segment{result})
          .matched ("\"/\" segment", r2);
      })
      .matched ("path-abempty", r);
  };
}

auto path_absolute (uri::parts& result) {
  // path-absolute = "/" [ segment-nz *( "/" segment ) ]
  return [&result] (rule const& r) {
    return r.concat (solidus, append_dir<true>{result})
      .optional ([&result] (rule const& r1) {
        return r1.concat (segment_nz, append_segment{result})
          .star ([&result] (rule const& r2) {
            return r2.concat (solidus, append_dir<false>{result})
              .concat (segment, append_segment{result})
              .matched ("\"/\" segment", r2);
          })
          .matched ("*( \"/\" segment )", r1);
      })
      .matched ("path-absolute", r);
  };
}

// path-noscheme = segment-nz-nc *( "/" segment )
auto path_noscheme (uri::parts& result) {
  return [&result] (rule const& r) {
    return r.concat (segment_nz_nc, record_initial_segment{result})
      .star ([&result] (rule const& r1) {
        return r1.concat (solidus, append_dir<false>{result})
          .concat (segment, append_segment{result})
          .matched ("\"/\" segment", r1);
      })
      .matched ("path-noscheme", r);
  };
}

// path-empty    = 0<pchar>
auto path_empty (rule const& r) {
  return r.star (pchar, 0, 0).matched ("path-empty", r);
}

// path-rootless = segment-nz *( "/" segment )
auto path_rootless (uri::parts& result) {
  return [&result] (rule const& r) {
    return r.concat (segment_nz, record_initial_segment{result})
      .star ([&result] (rule const& r1) {
        return r1.concat (solidus, append_dir<false>{result})
          .concat (segment, append_segment{result})
          .matched ("\"/\" segment", r1);
      })
      .matched ("path-rootless", r);
  };
}

// auth-abempty = "//" authority path-abempty
auto auth_abempty (uri::parts& result) {
  return [&result] (rule const& r) {
    return r.concat (solidus)
      .concat (solidus)
      .concat (authority (result))
      .concat (path_abempty (result))
      .matched ("auth-abempty", r);
  };
}

// relative-part = auth-abempty
//               / path-absolute
//               / path-noscheme
//               / path-empty
auto relative_part (uri::parts& result) {
  return [&result] (rule const& r) {
    return r
      .alternative (auth_abempty (result), path_absolute (result),
                    path_noscheme (result), path_empty)
      .matched ("relative-part", r);
  };
}

// query         = *( pchar / "/" / "?" )
auto query (rule const& r) {
  return r
    .star ([] (rule const& r2) {
      return r2.alternative (pchar, solidus, question_mark)
        .matched (R"(pchar / "/" / "?")", r2);
    })
    .matched ("query", r);
}
constexpr auto queryfn = query;

// question-query = "?" query
auto question_query (uri::parts& result) {
  return [&result] (rule const& r) {
    return r
      .concat (question_mark)  // "?"
      .concat (query, [&result] (
                        std::string_view const query) { result.query = query; })
      .matched ("question-query", r);
  };
}

// fragment      = *( pchar / "/" / "?" )
auto fragment (rule const& r) {
  return query (r);
}
constexpr auto fragmentfn = fragment;

// hash-fragment = "#" fragment
auto hash_fragment (uri::parts& result) {
  return [&result] (rule const& rf) {
    return rf.concat (hash)
      .concat (fragment,
               [&result] (std::string_view const fragment) {
                 result.fragment = fragment;
               })
      .matched ("hash-fragment", rf);
  };
};

// relative-ref  = relative-part [ question-query ] [ hash-fragment ]
auto relative_ref (uri::parts& result) {
  return [&result] (rule const& r) {
    return r.concat (relative_part (result))
      .optional (question_query (result))
      .optional ([&result] (rule const& rf) {
        // "#" fragment
        return rf
          .concat (hash)  // "#"
          .concat (       // fragment
            fragment,
            [&result] (std::string_view const fragment) {
              result.fragment = fragment;
            })
          .matched ("\"#\" fragment", rf);
      })
      .matched ("relative-ref", r);
  };
}

// relative-part = auth-abempty
//               / path-absolute
//               / path-noscheme
//               / path-empty

// hier-part     = auth-abempty
//               / path-absolute
//               / path-rootless
//               / path-empty
auto hier_part (uri::parts& result) {
  return [&result] (rule const& r) {
    return r
      .alternative (auth_abempty (result), path_absolute (result),
                    path_rootless (result), path_empty)
      .matched ("hier-part", r);
  };
}

// URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
auto URI (uri::parts& result) {
  return [&result] (rule const& r) {
    return r
      .concat (
        scheme,
        [&result] (std::string_view const scheme) { result.scheme = scheme; })
      .concat (colon)
      .concat (hier_part (result))
      .optional (question_query (result))
      .optional (hash_fragment (result))
      .matched ("URI", r);
  };
}

// URI-reference = URI / relative-ref
auto URI_reference (uri::parts& result) {
  return [&result] (rule const& r) {
    return r.alternative (URI (result), relative_ref (result))
      .matched ("URI-reference", r);
  };
}

#if 0
// absolute-URI  = scheme ":" hier-part [ "?" query ]
auto absolute_URI (uri::parts& result) {
  return [&result] (rule const& r) {
    return r
      .concat (
        scheme,
        [&result] (std::string_view const scheme) { result.scheme = scheme; })
      .concat (colon)
      .concat (hier_part (result))
      .optional (question_query (result))
      .matched ("absolute-URI", r);
  };
}
#endif

// merge
// ~~~~~
/// An implementation of the algorithm in RFC 3986, section 5.2.3 "Merge Paths"
/// (http://tools.ietf.org/html/rfc3986#section-5.2.3). It is responsible for
/// merging a relative-path reference with the path of the base URI.
///
/// \param base  The base URL.
/// \param ref  A relative-path reference.
/// \result  The merged path.
struct uri::parts::path merge (uri::parts const& base, uri::parts const& ref) {
  // If the base URI has a defined authority component and an empty path, then
  // return a path consisting of "/" concatenated with the reference's path
  if (base.authority && base.path.empty ()) {
    struct uri::parts::path r1;
    r1.absolute = true;
    r1.segments = ref.path.segments;
    return r1;
  }

  // Return a path consisting of the reference's path component appended to all
  // but the last segment of the base URI's path.
  struct uri::parts::path r2;
  r2.absolute = base.path.absolute;

  auto last = std::end (base.path.segments);
  if (base.path.segments.size () > 1) {
    std::advance (last, -1);
  }
  auto out = std::back_inserter (r2.segments);
  out = std::copy (std::begin (base.path.segments), last, out);
  std::copy (std::begin (ref.path.segments), std::end (ref.path.segments), out);
  return r2;
}

}  // end anonymous namespace

namespace uri {

bool parts::path::operator== (path const& rhs) const {
  return absolute == rhs.absolute && segments == rhs.segments;
}

// remove dot segments
// ~~~~~~~~~~~~~~~~~~~
// This function removes the special "." and ".." complete path segments from a
// referenced path. An implementation of the algorithm in RFC 3986,
// section 5.2.4 "Remove Dot Segments"
// (http://tools.ietf.org/html/rfc3986#section-5.2.4).
void parts::path::remove_dot_segments () {
  auto const begin = std::begin (this->segments);
  auto const end = std::end (this->segments);
  auto outit = begin;
  bool last_dir = false;
  for (auto it = begin; it != end; ++it) {
    if (*it == ".") {
      // '.' means "the current directory": remove it.
      last_dir = true;
    } else if (*it == "..") {
      // ".." is "up one directory" so, if we can, pop the last
      // element.
      last_dir = true;
      if (outit != begin) {
        --outit;
      }
    } else {
      last_dir = it->empty ();
      if (outit != it) {
        *outit = *it;
      }
      ++outit;
    }
  }
  this->segments.erase (outit, end);

  if (last_dir) {
    if (segments.empty ()) {
      this->segments.emplace_back ();
    } else {
      if (!this->segments.back ().empty ()) {
        this->segments.emplace_back ();
      }
    }
  }
}

parts::path::operator std::string () const {
  std::string p;
  auto const* separator = absolute ? "/" : "";
  for (auto const& seg : segments) {
    p += separator;
    p += seg;
    separator = "/";
  }
  return p;
}

parts::path::operator std::filesystem::path () const {
  std::filesystem::path p;
  if (absolute) {
    p /= "/";
  }
  for (auto const& seg : segments) {
    p /= seg;
  }
  return p;
}

bool parts::path::valid () const noexcept {
  for (auto const& seg : segments) {
    if (!rule{seg}.concat (segment).done ()) {
      return false;
    }
  }
  return true;
}

bool parts::authority::operator== (authority const& rhs) const {
  return userinfo == rhs.userinfo && host == rhs.host && port == rhs.port;
}

bool parts::authority::valid () const noexcept {
  if (userinfo.has_value () &&
      !rule{userinfo.value ()}.concat (userinfofn).done ()) {
    return false;
  }
  if (!rule{host}.concat (hostfn).done ()) {
    return false;
  }
  if (port.has_value () && !rule{port.value ()}.concat (portfn).done ()) {
    return false;
  }
  return true;
}

bool parts::valid () const noexcept {
  if (!this->scheme.has_value () ||
      !rule{this->scheme.value ()}.concat (schemefn).done ()) {
    return false;
  }
  if (this->authority.has_value () && !this->authority->valid ()) {
    return false;
  }
  if (!this->path.valid ()) {
    return false;
  }
  if (this->query.has_value () &&
      !rule{this->query.value ()}.concat (queryfn).done ()) {
    return false;
  }
  if (this->fragment.has_value () &&
      !rule{this->fragment.value ()}.concat (fragmentfn).done ()) {
    return false;
  }
  return true;
}

bool parts::operator== (parts const& rhs) const {
  if (scheme != rhs.scheme || authority != rhs.authority ||
      query != rhs.query || fragment != rhs.fragment) {
    return false;
  }

  if (this->authority && rhs.authority) {
    // Ignore the 'absolute' field. Both are implicitly absolute paths.
    if (path.segments != rhs.path.segments) {
      return false;
    }
  } else {
    if (path != rhs.path) {
      return false;
    }
  }
  return true;
}

std::optional<parts> split (std::string_view const in) {
  if (parts result; rule{in}.concat (URI (result)).done ()) {
    return result;
  }
  return {};
}
std::optional<parts> split_reference (std::string_view const in) {
  if (parts result; rule{in}.concat (URI_reference (result)).done ()) {
    return result;
  }
  return {};
}

std::ostream& operator<< (std::ostream& os,
                          struct parts::authority const& auth) {
  if (auth.userinfo.has_value ()) {
    os << *auth.userinfo << "@";
  }
  if (!auth.host.empty ()) {
    os << auth.host;
  }
  if (auth.port.has_value ()) {
    os << ':';
    if (!auth.port->empty ()) {
      os << *auth.port;
    }
  }
  return os;
}

std::ostream& operator<< (std::ostream& os, struct parts::path const& path) {
  return os << static_cast<std::string> (path);
}

// join
// ~~~~
/// Transforms a URI reference relative to a base URI into its target URI. An
/// implementation of the algorithm in RFC 3986, section 5.2.2 "Transform
/// References" (http://tools.ietf.org/html/rfc3986#section-5.2.2).
///
/// \param base  The base URI.
/// \param reference The URI reference.
/// \param strict  Strict mode.
/// \result  The target URI.
parts join (parts const& base, parts const& reference, bool strict) {
  // In "non-strict" mode we ignore a scheme in the reference if it is identical
  // to the base URI's scheme.
  std::optional<std::string_view> empty;
  auto const * ref_scheme = &reference.scheme;
  if (!strict && reference.scheme == base.scheme) {
    ref_scheme = &empty;
  }

  parts target;
  if (*ref_scheme) {
    target.scheme = *ref_scheme;
    target.authority = reference.authority;
    target.path = reference.path;
    target.path.remove_dot_segments ();
    target.query = reference.query;
  } else {
    if (reference.authority) {
      target.authority = reference.authority;
      target.path = reference.path;
      target.path.remove_dot_segments ();
      target.query = reference.query;
    } else {
      if (reference.path.empty ()) {
        target.path = base.path;
        target.query = reference.query ? reference.query : base.query;
      } else {
        if (reference.path.absolute) {
          target.path.absolute = true;
          target.path = reference.path;
          target.path.remove_dot_segments ();
        } else {
          target.path = merge (base, reference);
          target.path.remove_dot_segments ();
        }
        target.query = reference.query;
      }
      target.authority = base.authority;
    }
    target.scheme = base.scheme;
  }
  target.fragment = reference.fragment;
  return target;
}

std::optional<parts> join (std::string_view base, std::string_view reference,
                           bool strict) {
  auto const base_parts = split (base);
  if (!base_parts) {
    return {};
  }
  auto const reference_parts = split_reference (reference);
  if (!reference_parts) {
    return {};
  }
  return join (*base_parts, *reference_parts, strict);
}

std::ostream& compose (std::ostream& os, parts const& p) {
  if (p.scheme.has_value ()) {
    os << p.scheme.value () << ':';
  }
  if (p.authority.has_value ()) {
    os << "//" << p.authority.value ();
    if (!p.path.empty () && !p.path.absolute) {
      os << '/';
    }
  }
  os << p.path;
  if (p.query.has_value ()) {
    os << '?' << p.query.value ();
  }
  if (p.fragment.has_value ()) {
    os << '#' << p.fragment.value ();
  }
  return os;
}

std::string compose (parts const& p) {
  std::stringstream result;
  result << p;
  return result.str ();
}

std::ostream& operator<< (std::ostream& os, parts const& p) {
  return compose (os, p);
}

}  // end namespace uri
