//===- klee/sv_resize_count_value.cpp -------------------------------------===//
//*                            _                                 _    *
//*  _____   __  _ __ ___  ___(_)_______    ___ ___  _   _ _ __ | |_  *
//* / __\ \ / / | '__/ _ \/ __| |_  / _ \  / __/ _ \| | | | '_ \| __| *
//* \__ \\ V /  | | |  __/\__ \ |/ /  __/ | (_| (_) | |_| | | | | |_  *
//* |___/ \_/   |_|  \___||___/_/___\___|  \___\___/ \__,_|_| |_|\__| *
//*                                                                   *
//*             _             *
//* __   ____ _| |_   _  ___  *
//* \ \ / / _` | | | | |/ _ \ *
//*  \ V / (_| | | |_| |  __/ *
//*   \_/ \__,_|_|\__,_|\___| *
//*                           *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <cstddef>

#include "peejay/small_vector.hpp"
#include "resize_count_value.hpp"

int main() {
  resize_count_value<peejay::small_vector<member, 5>>();
}
