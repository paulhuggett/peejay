//===- include/peejay/emit.hpp ----------------------------*- mode: C++ -*-===//
//*                 _ _    *
//*   ___ _ __ ___ (_) |_  *
//*  / _ \ '_ ` _ \| | __| *
//* |  __/ | | | | | | |_  *
//*  \___|_| |_| |_|_|\__| *
//*                        *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_TREE_EMIT_HPP
#define PEEJAY_TREE_EMIT_HPP

#include <iosfwd>

#include "peejay/dom.hpp"

namespace peejay {

std::ostream& emit (std::ostream& os, std::optional<element> const& root);

}  // end namespace peejay

#endif  // PEEJAY_TREE_EMIT_HPP
