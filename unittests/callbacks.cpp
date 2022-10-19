//===- unittests/callbacks.cpp --------------------------------------------===//
//*            _ _ _                _         *
//*   ___ __ _| | | |__   __ _  ___| | _____  *
//*  / __/ _` | | | '_ \ / _` |/ __| |/ / __| *
//* | (_| (_| | | | |_) | (_| | (__|   <\__ \ *
//*  \___\__,_|_|_|_.__/ \__,_|\___|_|\_\___/ *
//*                                           *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "callbacks.hpp"

json_callbacks_base::~json_callbacks_base () noexcept = default;
mock_json_callbacks::~mock_json_callbacks () noexcept = default;
