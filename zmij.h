// A double-to-string conversion algorithm based on Schubfach.
// Copyright (c) 2025 - present, Victor Zverovich
// Distributed under the MIT license (see LICENSE) or alternatively
// the Boost Software License, Version 1.0.

#ifndef ZMIJ_H_
#define ZMIJ_H_

#include <stddef.h>  // size_t
#include <string.h>  // memcpy

namespace zmij {
namespace detail {
template <typename Float>
auto write(Float value, char* buffer) noexcept -> char*;
}  // namespace detail

enum {
  non_finite_exp = int(~0u >> 1),
};

// A decimal floating-point number sig * pow(10, exp).
// If exp is non_finite_exp then the number is a NaN or an infinity.
struct dec_fp {
  long long sig;  // significand
  int exp;        // exponent
  bool negative;
};

/// Converts `value` into the shortest correctly rounded decimal representation.
/// Usage:
///   auto [sig, exp, negative] = to_decimal(6.62607015e-34);
auto to_decimal(double value) noexcept -> dec_fp;

/// Rounding modes for `to_decimal`. The numeric values match the corresponding
/// modes of the classic David Gay dtoa used by CPython's `_Py_dg_dtoa`.
enum class dtoa_mode {
  shortest = 0,     ///< Shortest decimal that round-trips; `ndigits` is ignored.
  significant = 2,  ///< `max(1, ndigits)` significant digits.
  fractional = 3    ///< `ndigits` digits past the decimal point; may be negative
                    ///< (e.g. `-2` rounds to a multiple of 100).
};

/// Rounds `value` to the requested precision and returns `{sig, exp, negative}`
/// so that `sig * 10^exp` equals the rounded value. Trailing zeros are stripped
/// from `sig`, so the returned `exp` may differ from `-ndigits` in `fractional`
/// mode when the rounded value has trailing zeros.
///
/// For NaN or infinity, `exp == non_finite_exp`. For a zero result the function
/// returns `{0, 0, negative}`.
///
/// Modes `significant` and `fractional` are backed by a port of Ryū's
/// d2fixed/d2exp (see `_ryu/`), which gives bit-exact agreement with
/// bignum-based dtoa implementations. Up to approximately 18 significant
/// digits fit in `sig`; requests whose exact rounded value has more
/// non-trailing-zero digits (e.g. `fractional` mode with a very negative
/// `ndigits` on a large-magnitude double) will saturate — callers who need
/// the full digit expansion should use `dtoa` below.
auto to_decimal(double value, dtoa_mode mode, int ndigits = 0) noexcept
    -> dec_fp;

/// Digits-and-`decpt` view used by `dtoa`. Matches the shape of David Gay's
/// `_Py_dg_dtoa`: `digits` holds `digits_len` characters (no sign, no decimal
/// point, no exponent, trailing zeros stripped for finite values), with the
/// decimal point sitting `decpt` positions to the right of the first digit.
/// For `±inf`/NaN, `finite` is false and `digits` holds "Infinity" or "NaN".
struct dtoa_result {
  int digits_len;
  int decpt;
  bool negative;
  bool finite;
};

/// Writes a bit-exact rounded decimal representation of `value` into `buffer`
/// (which must have room for at least 2048 bytes). Returns the shape of the
/// output; `buffer[0..digits_len)` holds the digit string.
///
/// Handles all cpython dtoa modes at arbitrary precision:
///   - shortest: buffer holds the shortest round-trip decimal.
///   - significant: buffer holds `max(1, ndigits)` significant digits.
///   - fractional: buffer holds the digits of `round(value * 10^ndigits)`
///     with trailing zeros stripped. `ndigits` may be negative.
auto dtoa(double value, dtoa_mode mode, int ndigits, char* buffer) noexcept
    -> dtoa_result;

enum {
  float_buffer_size = 17,
  double_buffer_size = 33,
};

/// Writes the shortest correctly rounded decimal representation of `value` to
/// `out` without a null terminator. Returns a pointer past the last character
/// written; if the representation exceeds `n` characters, only the first `n`
/// are written.
inline auto write(char* out, size_t n, float value) noexcept -> char* {
  if (n >= float_buffer_size) return detail::write(value, out);
  char buffer[float_buffer_size];
  size_t size = detail::write(value, buffer) - buffer;
  if (size > n) size = n;
  memcpy(out, buffer, size);
  return out + size;
}

/// Writes the shortest correctly rounded decimal representation of `value` to
/// `out` without a null terminator. Returns a pointer past the last character
/// written; if the representation exceeds `n` characters, only the first `n`
/// are written.
inline auto write(char* out, size_t n, double value) noexcept -> char* {
  if (n >= double_buffer_size) return detail::write(value, out);
  char buffer[double_buffer_size];
  size_t size = detail::write(value, buffer) - buffer;
  if (size > n) size = n;
  memcpy(out, buffer, size);
  return out + size;
}

}  // namespace zmij

#endif  // ZMIJ_H_
