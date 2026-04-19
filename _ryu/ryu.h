// Minimal zmij-local header declaring the two Ryu entry points we use from
// `zmij.cc`. Everything else in `_ryu/` is implementation detail and lives in
// the `zmij::ryu` namespace.
//
// The algorithms and tables here are vendored (nearly) verbatim from
// https://github.com/ulfjack/ryu (Apache-2.0 / Boost-1.0 dual license, see
// _ryu/LICENSE-Apache2 and _ryu/LICENSE-Boost). Only the outermost shape
// changed: we namespaced the code and dropped the malloc-wrapping C entry
// points we don't need.

#ifndef ZMIJ_RYU_H_
#define ZMIJ_RYU_H_

#include <stdint.h>

namespace zmij {
namespace ryu {

// Writes `d` in printf "%.*f" style (no decimal point when precision == 0).
// Returns the number of characters written. `result` must hold enough room for
// the output (up to ~340 + precision bytes for doubles).
int d2fixed_buffered_n(double d, uint32_t precision, char* result);

// Writes `d` in printf "%.*e" style. `precision` is digits after the leading
// digit, so the total significand length is precision + 1. Returns the number
// of characters written.
int d2exp_buffered_n(double d, uint32_t precision, char* result);

}  // namespace ryu
}  // namespace zmij

#endif  // ZMIJ_RYU_H_
