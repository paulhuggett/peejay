#===- lib/schema/CMakeLists.txt -------------------------------------------===//
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
set (include_dir "${PEEJAY_ROOT}/include")
set (
  include_files
  "${include_dir}/peejay/schema/almost_equal.hpp"
  "${include_dir}/peejay/schema/error.hpp"
  "${include_dir}/peejay/schema/schema.hpp"
)
add_library (schema STATIC schema.cpp ${include_files})

target_include_directories (peejay PUBLIC
  $<BUILD_INTERFACE:${include_dir}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}>
)
target_link_libraries(schema PUBLIC peejay)
setup_target (schema)
set_target_properties (schema PROPERTIES PUBLIC_HEADER "${include_files}")
