//===- big_uint.hpp -------------------------------------------------------===//
//*  _    _             _     _    *
//* | |__(_)__ _   _  _(_)_ _| |_  *
//* | '_ \ / _` | | || | | ' \  _| *
//* |_.__/_\__, |  \_,_|_|_||_\__| *
//*        |___/                   *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
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
/// \file big_uint.hpp
/// \brief A small fixed-capacity unsigned integer used for exact decimal-to-binary conversion.
///
/// The limb type is a template parameter so that the arithmetic can use whatever width suits the
/// target. 64-bit limbs need a 128-bit intermediate for multiplication and division: the compiler's
/// `unsigned __int128` is used where it exists. Define `PEEJAY_BIG_UINT_NO_INT128` to force the
/// portable (standard C++ only) implementation instead, for example to test it on a target that
/// does have `__int128`.
#ifndef PEEJAY_DETAILS_BIG_UINT_HPP
#define PEEJAY_DETAILS_BIG_UINT_HPP

#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "peejay/details/uinteger.hpp"

namespace peejay::details {

/// \brief Satisfied by the unsigned integer types that big_uint can use as a limb.
///
/// A limb must be an unsigned integer type (so not `bool`, a character type, or a signed type)
/// that is 8, 16, 32 or 64 bits wide.
template <typename T>
concept big_uint_limb =
    std::is_integral_v<T> && std::is_unsigned_v<T> && requires(T const x) { std::countl_zero(x); } &&
    (std::numeric_limits<T>::digits == 8 || std::numeric_limits<T>::digits == 16 ||
     std::numeric_limits<T>::digits == 32 || std::numeric_limits<T>::digits == 64);

#if defined(__SIZEOF_INT128__) && !defined(PEEJAY_BIG_UINT_NO_INT128)
/// True if the compiler's 128-bit integer is used for 64-bit limbs.
inline constexpr bool big_uint_has_int128 = true;
/// An unsigned 128-bit integer, used as the double-width intermediate for 64-bit limbs.
__extension__ using big_uint_u128 =  unsigned __int128;
#else
/// False: 64-bit limbs use the portable 64x64 -> 128 multiplication and 128 / 64 division.
inline constexpr bool big_uint_has_int128 = false;
#endif

/// \brief The two halves of a double-width result: the low limb and the high limb.
/// \tparam Limb The limb type.
template <typename Limb> struct limb_pair {
  /// The least significant limb.
  Limb low;
  /// The most significant limb.
  Limb high;
};

/// \brief The quotient and remainder of a double-width division.
/// \tparam Limb The limb type.
template <typename Limb> struct limb_quotient {
  /// The quotient.
  Limb quotient;
  /// The remainder.
  Limb remainder;
};

/// \brief Computes `x * m + carry` as a double-width result.
///
/// This is the inner step of big_uint::mul_add(). The exact result needs twice the width of a limb
/// and cannot overflow it: `(2^w - 1) * (2^w - 1) + (2^w - 1) < 2^(2w)` for a limb of `w` bits.
///
/// Limbs of up to 32 bits use a native integer of twice the width. 64-bit limbs use the compiler's
/// 128-bit integer if there is one, and otherwise multiply by splitting each operand into 32-bit
/// halves.
///
/// \tparam Limb The limb type (see big_uint_limb).
/// \param x The limb to multiply.
/// \param m The multiplier.
/// \param carry The value to add to the product (the carry from the next lower limb).
/// \returns The low and high limbs of `x * m + carry`.
template <big_uint_limb Limb>
constexpr limb_pair<Limb> mul_add_limb(Limb const x, Limb const m, Limb const carry) noexcept {
  // The width of a limb in bits.
  constexpr unsigned bits = std::numeric_limits<Limb>::digits;
  if constexpr (bits <= 32U) {
    // A native unsigned type of at least twice the limb width.
    //using wide = std::conditional_t<(bits <= 16U), std::uint32_t, std::uint64_t>;
    using wide = uinteger_t<bits * 2U>;
    auto const t = static_cast<wide>(static_cast<wide>(x) * static_cast<wide>(m) + carry);
    return {static_cast<Limb>(t), static_cast<Limb>(t >> bits)};
  } else {
#if defined(__SIZEOF_INT128__) && !defined(PEEJAY_BIG_UINT_NO_INT128)
    auto const t = static_cast<big_uint_u128>(x) * m + carry;
    return {static_cast<Limb>(t), static_cast<Limb>(t >> 64U)};
#else
    // Portable 64 x 64 -> 128 multiplication from four 32-bit partial products.
    constexpr std::uint64_t mask32 = 0xFFFFFFFFU;
    auto const xlo = static_cast<std::uint64_t>(x) & mask32;  // Low half of x.
    auto const xhi = static_cast<std::uint64_t>(x) >> 32U;    // High half of x.
    auto const mlo = static_cast<std::uint64_t>(m) & mask32;  // Low half of m.
    auto const mhi = static_cast<std::uint64_t>(m) >> 32U;    // High half of m.
    std::uint64_t const p00 = xlo * mlo;  // The four partial products.
    std::uint64_t const p01 = xlo * mhi;
    std::uint64_t const p10 = xhi * mlo;
    std::uint64_t const p11 = xhi * mhi;
    // The bits 32..63 of the product: the carries from the three terms that overlap there.
    std::uint64_t const middle = (p00 >> 32U) + (p01 & mask32) + (p10 & mask32);
    std::uint64_t low = (p00 & mask32) | (middle << 32U);
    std::uint64_t high = p11 + (p01 >> 32U) + (p10 >> 32U) + (middle >> 32U);
    // Add the carry in; a wrap of the low half carries into the high half.
    std::uint64_t const sum = low + static_cast<std::uint64_t>(carry);
    high += static_cast<std::uint64_t>(sum < low);
    low = sum;
    return {static_cast<Limb>(low), static_cast<Limb>(high)};
#endif
  }
}

/// \brief Divides the double-width value `high:low` by a single limb.
///
/// This is the inner step of big_uint::div_small(). The caller must ensure that `high < d`, which
/// guarantees that the quotient fits in a single limb. That always holds in long division because
/// `high` is the remainder of the previous step.
///
/// Limbs of up to 32 bits use a native integer of twice the width. 64-bit limbs use the compiler's
/// 128-bit integer if there is one, and otherwise the classic two-step algorithm that divides a
/// normalised divisor using 32-bit digits (Knuth's algorithm D specialised to two limbs, as in
/// Hacker's Delight, "divlu").
///
/// \tparam Limb The limb type (see big_uint_limb).
/// \param high The most significant limb of the dividend; must be less than \p d.
/// \param low The least significant limb of the dividend.
/// \param d The divisor; must not be zero.
/// \returns The quotient and the remainder, which is less than \p d.
template <big_uint_limb Limb>
constexpr limb_quotient<Limb> div_limb(Limb const high, Limb const low, Limb const d) noexcept {
  // The width of a limb in bits.
  constexpr unsigned bits = std::numeric_limits<Limb>::digits;
  if constexpr (bits <= 32U) {
    // A native unsigned type of at least twice the limb width.
    using wide = std::conditional_t<(bits <= 16U), std::uint32_t, std::uint64_t>;
    auto const t = static_cast<wide>(static_cast<wide>(high) << bits) | static_cast<wide>(low);
    auto const q = static_cast<wide>(t / static_cast<wide>(d));
    return {static_cast<Limb>(q), static_cast<Limb>(t - q * static_cast<wide>(d))};
  } else {
#if defined(__SIZEOF_INT128__) && !defined(PEEJAY_BIG_UINT_NO_INT128)
    big_uint_u128 const t = (static_cast<big_uint_u128>(high) << 64U) | low;
    // One division only: the remainder is recovered by multiplying back.
    big_uint_u128 const q = t / d;
    return {static_cast<Limb>(q), static_cast<Limb>(t - q * d)};
#else
    constexpr std::uint64_t base = std::uint64_t{1} << 32U;  // The base of the 32-bit digits.
    constexpr std::uint64_t mask32 = base - 1U;
    // Normalise: shift the divisor left until its top bit is set (d != 0, so shift < 64).
    auto const shift = static_cast<unsigned>(std::countl_zero(static_cast<std::uint64_t>(d)));
    std::uint64_t const v = static_cast<std::uint64_t>(d) << shift;
    std::uint64_t const vn1 = v >> 32U;    // The high 32-bit digit of the divisor.
    std::uint64_t const vn0 = v & mask32;  // The low 32-bit digit of the divisor.
    // The dividend shifted by the same amount, as three 32-bit digits (un32 is two digits wide).
    std::uint64_t const u1 = high;
    std::uint64_t const u0 = low;
    std::uint64_t const un32 = shift == 0U ? u1 : (u1 << shift) | (u0 >> (64U - shift));
    std::uint64_t const un10 = u0 << shift;
    std::uint64_t const un1 = un10 >> 32U;
    std::uint64_t const un0 = un10 & mask32;
    // First quotient digit, estimated from the top digits and then corrected (at most twice).
    std::uint64_t q1 = un32 / vn1;
    std::uint64_t rhat = un32 - q1 * vn1;
    while (q1 >= base || q1 * vn0 > base * rhat + un1) {
      --q1;
      rhat += vn1;
      if (rhat >= base) {
        break;
      }
    }
    std::uint64_t const un21 = un32 * base + un1 - q1 * v;
    // Second quotient digit, in the same way.
    std::uint64_t q0 = un21 / vn1;
    rhat = un21 - q0 * vn1;
    while (q0 >= base || q0 * vn0 > base * rhat + un0) {
      --q0;
      rhat += vn1;
      if (rhat >= base) {
        break;
      }
    }
    std::uint64_t const remainder = (un21 * base + un0 - q0 * v) >> shift;
    return {static_cast<Limb>(q1 * base + q0), static_cast<Limb>(remainder)};
#endif
  }
}

/// \brief Fixed-capacity unsigned integer of `LimbCount` limbs of type `Limb`, stored as a
/// little-endian array.
///
/// big_uint<> represents a non-negative integer in base 2^limb_bits, where limb_bits is the width
/// of `Limb`. The value is held in `LimbCount` limbs; limb 0 is the least significant and limb
/// `LimbCount - 1` is the most significant. The capacity is therefore `LimbCount * limb_bits` bits
/// and the representable values are `0` through `2^(LimbCount * limb_bits) - 1`.
///
/// Only the operations needed by number_parser are provided.
/// All operations are modulo 2^(LimbCount * limb_bits) unless stated otherwise: results that do not
/// fit are silently truncated (mul_add() additionally reports the bits that were lost). A default
/// constructed object has the value zero.
///
/// The limb type controls how much work each operation does and how fast it is: wider limbs mean
/// fewer iterations, but the intermediate of a 64-bit limb is 128 bits wide (see mul_add_limb()
/// and div_limb()). Choose it for the target, for example:
/// \code
/// big_uint<4> a;                       // 4 x 32 bits: the default
/// big_uint<2, std::uint64_t> b;        // 2 x 64 bits: the same 128 bit capacity, half the limbs
/// big_uint<4, std::uint_fast32_t> c;   // 4 limbs of whatever std::uint_fast32_t is on this target
/// \endcode
///
/// \note The width of a limb is the width of its *type*, not the number in its name. On some
/// targets (glibc on x86-64, for instance) `std::uint_fast32_t` is 64 bits wide, so it holds 64
/// bits per limb, and so does `std::uint_fast16_t`. Code that depends on a particular layout should
/// use limb_bits and capacity_bits rather than assuming 32.
///
/// \tparam LimbCount The number of limbs, and hence the capacity: the object holds
///         `LimbCount * limb_bits` bits. The capacity must be at least 64 bits, because low64()
///         and fits_u64() address the low 64 bits directly; this is enforced by a requires-clause.
/// \tparam Limb The type of each limb: an unsigned integer type that is 8, 16, 32 or 64 bits wide
///         (see big_uint_limb). It defaults to `std::uint32_t`.
template <std::size_t LimbCount, typename Limb = std::uint32_t>
  requires(big_uint_limb<Limb> && LimbCount * std::numeric_limits<Limb>::digits >= 64U)
class big_uint {
public:
  /// The type of a single limb.
  using limb_type = Limb;

  /// The width of a limb in bits.
  static constexpr std::size_t limb_bits = std::numeric_limits<Limb>::digits;

  /// The total number of bits the object can hold: `LimbCount * limb_bits`.
  static constexpr std::size_t capacity_bits = LimbCount * limb_bits;

  /// \brief Returns one limb of the value.
  ///
  /// Limb 0 holds the least significant limb_bits bits of the value, limb 1 the next limb_bits
  /// bits, and so on. The index is not range checked: an index that is not less than `LimbCount`
  /// is undefined behaviour.
  ///
  /// \param index The zero-based position of the limb to read, in the range `[0, LimbCount)`.
  /// \returns The limb at position \p index.
  [[nodiscard]] limb_type limb(std::size_t const index) const noexcept {
    assert(index < LimbCount && "index out of range"); 
    return limbs_[index];
  }

  /// \brief Overwrites one 32-bit limb of the value.
  ///
  /// The other limbs are left untouched, so this can be used to build an arbitrary value
  /// one limb at a time. An index that is not less than `LimbCount` is undefined behaviour.
  ///
  /// \param index The zero-based position of the limb to write, in the range `[0, LimbCount)`.
  /// \param v The new value of the limb.
  void set_limb(std::size_t const index, limb_type const v) noexcept {
    assert(index < LimbCount && "index out of range"); 
	limbs_[index] = v; 
  }

  /// \brief Sets the value with `value * m + a`.
  ///
  /// This is the multiply-and-accumulate primitive of the class. Appending a decimal
  /// digit to a number is `mul_add(10, digit)`, scaling by a power of five is `mul_add(5^k, 0)`,
  /// and incrementing is `mul_add(1, 1)`.
  ///
  /// The multiplication is performed one limb at a time from the least significant limb upwards,
  /// carrying into the next limb. Each step computes `limb * m + carry` in a double-width integer
  /// (see mul_add_limb()); that cannot overflow because the largest possible value,
  /// `(2^w - 1) * (2^w - 1) + (2^w - 1)` for a limb of w bits, is `2^(2w) - 2^w`, which is still
  /// below `2^(2w)`.
  ///
  /// \param multiplier The multiplier.
  /// \param addend  Added after the multiplication.
  /// \returns The carry out of the most significant limb: the part of the exact result
  ///          `value * m + a` that lies at or above `2^(LimbCount * limb_bits)` and was therefore
  ///          discarded. It is always less than `2^limb_bits`, and is zero if and only if the
  ///          result fitted in the capacity of the object.
  constexpr limb_type mul_add(limb_type const multiplier, limb_type const addend) noexcept {
    // The carry into the next limb; it starts as the addend.
    auto carry = addend;
    for (std::size_t i = 0; i < LimbCount; ++i) {
      auto const r = mul_add_limb<limb_type>(limbs_[i], multiplier, carry);
      limbs_[i] = r.low;  // The low part stays in this limb.
      carry = r.high;
    }
    return carry;
  }

  /// \brief Replaces the value with `floor(value / d)` and returns the remainder.
  ///
  /// This is schoolbook long division in base 2^limb_bits by a single-limb divisor. It works
  /// from the most significant limb downwards: the remainder of the higher limbs is carried down
  /// and prepended to the next limb to form a double-width partial dividend, which is divided by
  /// \p d to give the next quotient limb (see div_limb()). The partial dividend's quotient always
  /// fits in one limb because the carried remainder is less than \p d.
  ///
  /// After the call the original value equals `quotient * d + remainder`, where the remainder is
  /// at least 0 and less than \p d. Dividing repeatedly and noting whether any remainder was
  /// non-zero tells the caller whether a division by a product of the divisors was exact.
  ///
  /// \pre \p d is not zero.
  /// \param d The divisor.
  /// \returns The remainder of the division, in the range `[0, d)`.
  constexpr limb_type div_small(limb_type const d) noexcept {
    assert(d != 0);
    // The running remainder: what is left over after dividing the limbs seen so far.
    limb_type rem = 0;
    for (std::size_t index = LimbCount; index != 0U; --index) {
      std::size_t const i = index - 1U;
      auto const r = div_limb<limb_type>(rem, limbs_[i], d);
      limbs_[i] = r.quotient;  // This limb becomes a quotient limb.
      rem = r.remainder;       // Carry the remainder to the next limb.
    }
    return rem;
  }

  /// \brief Multiplies the value by `2^bits`, discarding any bits shifted out of the top.
  ///
  /// The shift is split into a whole-limb part (`bits / limb_bits` limbs) and a remaining bit
  /// part (`bits % limb_bits` bits). Limbs are rewritten from the most significant down to the
  /// least, so each result limb is built from source limbs at lower indices that have not been
  /// overwritten yet; the operation is therefore safe to do in place.
  ///
  /// Any value of \p bits is accepted. A shift of `capacity_bits` or more leaves the value
  /// zero.
  ///
  /// \param bits The number of bit positions to shift left by.
  constexpr void shift_left(std::size_t const bits) noexcept {
    std::size_t const limb_shift = bits / limb_bits;
    std::size_t const bit_shift = bits % limb_bits;
    for (std::size_t index = LimbCount; index != 0U; --index) {
      std::size_t const i = index - 1U;
      auto v = limb_type{0};
      if (i >= limb_shift) {
        std::size_t const src = i - limb_shift;
        v = static_cast<limb_type>(limbs_[src] << bit_shift);
        // The low bits of the next lower limb fill the vacated bottom of the result limb. The
        // bit_shift != 0 guard also avoids shifting by the full width of a limb, which is undefined.
        if (bit_shift != 0 && src > 0U) {
          v = static_cast<limb_type>(v | (limbs_[src - 1] >> (limb_bits - bit_shift)));
        }
      }
      limbs_[i] = v;
    }
  }

  /// \brief Divides the value by `2^bits`, discarding the remainder.
  ///
  /// This is `floor(value / 2^bits)`. As with shift_left(), the shift is split into
  /// whole limbs (`bits / limb_bits`) and a remaining bit part (`bits % limb_bits`). Limbs are
  /// rewritten from the least significant upwards, so each result limb is built from source limbs
  /// at the same or higher index that have not been overwritten yet.
  ///
  /// A shift of `capacity_bits` or more leaves the value zero; bits shifted out are lost.
  ///
  /// \param bits The number of bit positions by which to shift.
  void shift_right(std::size_t const bits) noexcept {
    std::size_t const limb_shift = bits / limb_bits;
    // The bits to shift within a limb.
    std::size_t const bit_shift = bits % limb_bits;
    for (std::size_t i = 0; i < LimbCount; ++i) {
      auto v = limb_type{0};
      if (std::size_t const src = i + limb_shift; src < LimbCount) {
        v = static_cast<limb_type>(limbs_[src] >> bit_shift);
        // The low bits of the next higher limb fill the vacated top of the result limb. The
        // bit_shift != 0 guard also avoids shifting by the full width of a limb, which is undefined.
        if (bit_shift != 0 && src + 1 < LimbCount) {
          v = static_cast<limb_type>(v | (limbs_[src + 1] << (limb_bits - bit_shift)));
        }
      }
      limbs_[i] = v;
    }
  }

  /// \brief Returns the number of bits needed to represent the value.
  ///
  /// This is the index of the highest set bit plus one, i.e. `floor(log2(value)) + 1`
  /// for a non-zero value, and zero for the value zero. It is found by locating the most
  /// significant non-zero limb and counting its leading zero bits.
  ///
  /// \returns The bit length, in the range `[0, capacity_bits]`.
  [[nodiscard]] std::size_t bit_length() const noexcept {
    for (std::size_t limb = LimbCount; limb != 0U; --limb) {
      if (std::size_t const i = limb - 1U; limbs_[i] != 0U) {
        return i * limb_bits + (limb_bits - static_cast<std::size_t>(std::countl_zero(limbs_[i])));
      }
    }
    return 0;
  }

  /// \brief Returns the value of a single bit.
  ///
  /// Bit 0 is the least significant bit of the whole value (the lowest bit of limb 0).
  /// Unlike limb(), the index is checked: every bit position at or beyond the capacity
  /// (`capacity_bits`) reads as zero, which is what an unsigned integer extended with leading
  /// zeros would give.
  ///
  /// \param index The zero-based bit position, counted from the least significant bit.
  /// \returns `true` if the bit is set; `false` if it is clear or \p index is beyond the capacity.
  [[nodiscard]] bool bit(std::size_t const index) const noexcept {
    std::size_t const i = index / limb_bits;
    return i < LimbCount && ((limbs_[i] >> (index % limb_bits)) & 1U) != 0U;
  }

  /// \brief Reports whether any bit below a given position is set.
  ///
  /// This tests the bits `[0, index)`, that is, every bit strictly less significant than
  /// \p index. In correct rounding it supplies the "sticky" bit: whether any non-zero bits lie
  /// below the rounding position. It does so without shifting or copying the value.
  ///
  /// The whole limbs below the position are scanned first, and then the low bits of the limb that
  /// straddles the position are tested with a mask. If \p index is at or beyond the capacity,
  /// every bit of the value is examined, so the result is `true` exactly when the value is
  /// non-zero.
  ///
  /// \param index  The bit position; bits at positions less than this are tested. An \p index of
  ///               zero tests no bits and always gives `false`.
  /// \returns `true` if at least one of the bits `[0, index)` is set; otherwise `false`.
  [[nodiscard]] bool any_below(std::size_t const index) const noexcept {
    std::size_t const whole = index / limb_bits;
    for (auto i = std::size_t{0}; i < whole && i < LimbCount; ++i) {
      if (limbs_[i] != 0U) {
        return true;
      }
    }
    if (std::size_t const rest = index % limb_bits; rest != 0U && whole < LimbCount) {
      return (limbs_[whole] & (static_cast<limb_type>(limb_type{1U} << rest) - 1U)) != 0U;
    }
    return false;
  }

  /// \brief Reports whether the value is below `2^64`.
  ///
  /// The value fits in an unsigned 64-bit integer exactly when every limb above the ones that
  /// hold the low 64 bits is zero. When this returns `true`, low64() returns the whole value.
  ///
  /// \returns `true` if the value is less than `2^64`; otherwise `false`.
  [[nodiscard]] bool fits_u64() const noexcept {
    // With a capacity of exactly 64 bits there are no higher limbs to check.
    if constexpr (LimbCount > limbs_in_u64) {
      for (std::size_t index = limbs_in_u64; index < LimbCount; ++index) {
        if (limbs_[index] != 0U) {
          return false;
        }
      }
    }
    return true;
  }

  /// \brief Returns the low 64 bits of the value.
  ///
  /// The result is the value modulo 2^64, formed from the lowest limbs (the higher limb
  /// supplying the high part). It equals the whole value only if fits_u64() is `true`; otherwise
  /// the higher limbs are ignored.
  ///
  /// \returns `value mod 2^64`.
  [[nodiscard]] std::uint64_t low64() const noexcept {
    if constexpr (limb_bits >= 64U) {
      return static_cast<std::uint64_t>(limbs_[0]);
    } else {
      std::uint64_t result = 0;
      for (std::size_t i = limbs_in_u64; i != 0U; --i) {
        result = (result << limb_bits) | limbs_[i - 1U];
      }
      return result;
    }
  }

private:
  /// The number of limbs that hold the low 64 bits (at least 1).
  static constexpr std::size_t limbs_in_u64 = (64U + limb_bits - 1U) / limb_bits;

  /// Storage for the value in little-endian order.
  std::array<limb_type, LimbCount> limbs_{};
};

}  // namespace peejay::details

#endif  // PEEJAY_DETAILS_BIG_UINT_HPP
