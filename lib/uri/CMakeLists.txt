#===- lib/uri/CMakeLists.txt ----------------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
# Distributed under the Apache License v2.0.
# See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
# SPDX-License-Identifier: Apache-2.0
#===----------------------------------------------------------------------===//
set (URI_INCLUDE_DIR "${PEEJAY_ROOT}/include")
add_library (uri STATIC
    "${URI_INCLUDE_DIR}/peejay/uri/find_last.hpp"
    "${URI_INCLUDE_DIR}/peejay/uri/parts.hpp"
    "${URI_INCLUDE_DIR}/peejay/uri/pctdecode.hpp"
    "${URI_INCLUDE_DIR}/peejay/uri/pctencode.hpp"
    "${URI_INCLUDE_DIR}/peejay/uri/punycode.hpp"
    "${URI_INCLUDE_DIR}/peejay/uri/rule.hpp"
    "${URI_INCLUDE_DIR}/peejay/uri/starts_with.hpp"
    "${URI_INCLUDE_DIR}/peejay/uri/uri.hpp"
    parts.cpp
    pctencode.cpp
    punycode.cpp
    rule.cpp
    uri.cpp
)
setup_target (uri)
target_include_directories (
  uri PUBLIC $<BUILD_INTERFACE:${URI_INCLUDE_DIR}> $<INSTALL_INTERFACE:uri>
)
target_link_libraries(uri PUBLIC peejay)
