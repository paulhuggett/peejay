#===- lib/peejay/CMakeLists.txt -------------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
#
# Distributed under the Apache License v2.0.
# See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
# for license information.
# SPDX-License-Identifier: Apache-2.0
#
#===----------------------------------------------------------------------===//
set (include_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../include")

set (CPRUN_HPP ${include_dir}/peejay/cprun.hpp)
set (CPRUN_CPP ${CMAKE_CURRENT_SOURCE_DIR}/cprun.cpp)

add_library (peejay STATIC
  "${include_dir}/peejay/arrayvec.hpp"
  "${include_dir}/peejay/dom.hpp"
  "${include_dir}/peejay/emit.hpp"
  "${include_dir}/peejay/icubaby.hpp"
  "${include_dir}/peejay/json.hpp"
  "${include_dir}/peejay/json_error.hpp"
  "${include_dir}/peejay/null.hpp"
  "${include_dir}/peejay/portab.hpp"
  emit.cpp
  json_error.cpp
  "${CPRUN_HPP}"
  "${CPRUN_CPP}"
)
target_include_directories (peejay PUBLIC "${include_dir}")
setup_target (peejay)

set (GENCPRUN "${CMAKE_CURRENT_SOURCE_DIR}/../../cprun/parse.py")
set (UNICODE_DATA "${CMAKE_CURRENT_SOURCE_DIR}/../../cprun/UnicodeData.txt")

find_package(Python3 3.10 REQUIRED)
if (NOT Python3_FOUND)
  message(ERROR "Python 3.10 was not found")
endif()

add_custom_command (
  COMMENT "Generating cprun source/header file"
  COMMAND ${Python_EXECUTABLE} ${GENCPRUN} ARGS --unicode-data=${UNICODE_DATA} --hpp > ${CPRUN_HPP}
  COMMAND ${Python_EXECUTABLE} ${GENCPRUN} ARGS --unicode-data=${UNICODE_DATA} --cpp --header-file=peejay/cprun.hpp > ${CPRUN_CPP}
  OUTPUT ${CPRUN_HPP} ${CPRUN_CPP}
  VERBATIM
)
