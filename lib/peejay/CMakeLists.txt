#===- lib/peejay/CMakeLists.txt -------------------------------------------===//
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
set (CPRUN_HPP ${include_dir}/peejay/json/cprun.hpp)
set (
  include_files
  "${include_dir}/peejay/json/arrayvec.hpp"
  "${include_dir}/peejay/json/avbase.hpp"
  "${include_dir}/peejay/json/cbii.hpp"
  "${include_dir}/peejay/json/dom.hpp"
  "${include_dir}/peejay/json/emit.hpp"
  "${include_dir}/peejay/json/icubaby.hpp"
  "${include_dir}/peejay/json/json.hpp"
  "${include_dir}/peejay/json/json_error.hpp"
  "${include_dir}/peejay/json/null.hpp"
  "${include_dir}/peejay/json/pointer_based_iterator.hpp"
  "${include_dir}/peejay/json/portab.hpp"
  "${include_dir}/peejay/json/small_vector.hpp"
  "${include_dir}/peejay/json/stack.hpp"
  "${include_dir}/peejay/json/uinteger.hpp"
  "${CPRUN_HPP}"
)
add_library (peejay STATIC dummy.cpp ${include_files})

target_include_directories (peejay PUBLIC
  $<BUILD_INTERFACE:${include_dir}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}>
)

setup_target (peejay)

set (GENCPRUN "${CMAKE_CURRENT_SOURCE_DIR}/../../cprun/parse.py")
set (UNICODE_DATA "${CMAKE_CURRENT_SOURCE_DIR}/../../cprun/UnicodeData.txt")

# Look for python 3.10. The parse.py program uses some features from typing that
# were introduced in that version of Python.
find_package (Python3 3.10)
if (NOT Python3_FOUND)
  message (WARNING "Python 3.10 was not found: cprun generation is disabled")
else ()
  # Run the gencprun utility to create cprun.hpp. Uncomment the second COMMAND
  # to see the contents of the file in the build transcript.
  add_custom_command (
    COMMENT "Generating cprun header file '${CPRUN_HPP}'"
    COMMAND ${Python3_EXECUTABLE} ${GENCPRUN} --unicode-data=${UNICODE_DATA}
            --include-guard=PEEJAY_CPRUN_HPP > ${CPRUN_HPP}
    OUTPUT ${CPRUN_HPP}
    VERBATIM
  )
endif ()

set_target_properties (peejay PROPERTIES PUBLIC_HEADER "${include_files}")
include (GNUInstallDirs)
install (
  TARGETS peejay
  EXPORT peejay
  PUBLIC_HEADER
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/peejay"
    COMPONENT peejay
  LIBRARY COMPONENT peejay
  ARCHIVE COMPONENT peejay
  INCLUDES
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
add_dependencies (install-peejay peejay)
