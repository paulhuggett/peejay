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
// Copyright © 2025 Paul Bowen-Huggett
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// “Software”), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT
//===----------------------------------------------------------------------===//
#include <cstddef>

#include "peejay/details/arrayvec.hpp"
#include "resize_count_value.hpp"

int main() {
  resize_count_value<peejay::arrayvec<member, max_elements>>();
}
