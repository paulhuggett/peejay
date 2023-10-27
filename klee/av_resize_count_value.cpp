//===- klee/av_resize_count_value.cpp -------------------------------------===//
//*                              _                                 _    *
//*   __ ___   __  _ __ ___  ___(_)_______    ___ ___  _   _ _ __ | |_  *
//*  / _` \ \ / / | '__/ _ \/ __| |_  / _ \  / __/ _ \| | | | '_ \| __| *
//* | (_| |\ V /  | | |  __/\__ \ |/ /  __/ | (_| (_) | |_| | | | | |_  *
//*  \__,_| \_/   |_|  \___||___/_/___\___|  \___\___/ \__,_|_| |_|\__| *
//*                                                                     *
//*             _             *
//* __   ____ _| |_   _  ___  *
//* \ \ / / _` | | | | |/ _ \ *
//*  \ V / (_| | | |_| |  __/ *
//*   \_/ \__,_|_|\__,_|\___| *
//*                           *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <cstddef>

#include "peejay/arrayvec.hpp"
#include "resize_count_value.hpp"

int main () {
  resize_count_value<peejay::arrayvec<member, max_elements>> ();
}
