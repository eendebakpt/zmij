// Tests for https://github.com/vitaut/zmij/.
// Copyright (c) 2025 - present, Victor Zverovich
// Distributed under the MIT license (see LICENSE).

// Include zmij.cc instead of linking with the library to test multiple
// configurations without building multiple versions of the library and to test
// internal functions.
#ifndef ZMIJ_C
#define ZMIJ_C 0
#include "../zmij.cc"
#else
#define _Alignas(x) alignas(x)
#include "../zmij.c"

namespace zmij {
enum {
  double_buffer_size = 25,
  float_buffer_size = 16,
};

auto write(char* out, size_t n, double value) noexcept -> char* {
  return out + zmij_write_double(out, n, value);
}
auto write(char* out, size_t n, float value) noexcept -> char* {
  return out + zmij_write_float(out, n, value);
}
}
#endif

#include <gtest/gtest.h>

#include <limits>  // std::numeric_limits
#include <string>  // std::string

#include "dragonbox/dragonbox_to_chars.h"


auto dtoa(double value) -> std::string {
  char buffer[zmij::double_buffer_size + 1] = {};
  memset(buffer, '?', sizeof(buffer));
  auto end = zmij::write(buffer + 1, sizeof(buffer), value);
  if (buffer[0] != '?') throw std::runtime_error("buffer underrun");
  return {buffer + 1, end};
}

auto ftoa(float value) -> std::string {
  char buffer[zmij::float_buffer_size] = {};
  auto end = zmij::write(buffer, sizeof(buffer), value);
  return {buffer, end};
}

TEST(zmij_test, utilities) {
  EXPECT_EQ(clz(1), 63);
  EXPECT_EQ(clz(~0ull), 0);

  EXPECT_EQ(count_trailing_nonzeros(0x00000000'00000000ull), 0);
  EXPECT_EQ(count_trailing_nonzeros(0x00000000'00000001ull), 1);
  EXPECT_EQ(count_trailing_nonzeros(0x00000000'00000009ull), 1);
  EXPECT_EQ(count_trailing_nonzeros(0x00090000'09000000ull), 7);
  EXPECT_EQ(count_trailing_nonzeros(0x01000000'00000000ull), 8);
  EXPECT_EQ(count_trailing_nonzeros(0x09000000'00000000ull), 8);
}

TEST(dtoa_test, normal) {
  EXPECT_EQ(dtoa(6.62607015e-34), "6.62607015e-34");

  // Exact half-ulp tie when rounding to nearest integer.
  EXPECT_EQ(dtoa(5.444310685350916e+14), "544431068535091.6");
}

TEST(dtoa_test, subnormal) {
  EXPECT_EQ(dtoa(std::numeric_limits<double>::denorm_min()), "5e-324");
  EXPECT_EQ(dtoa(1e-323), "1e-323");
  EXPECT_EQ(dtoa(1.2e-322), "1.2e-322");
  EXPECT_EQ(dtoa(1.5e-323), "1.5e-323");
  EXPECT_EQ(dtoa(1.24e-322), "1.24e-322");
  EXPECT_EQ(dtoa(1.234e-320), "1.234e-320");
  EXPECT_EQ(dtoa(2.2250738585072004e-308), "2.2250738585072004e-308");
}

TEST(dtoa_test, all_irregular) {
  const char* fixed[] = {
    "0.0001220703125",
    "0.000244140625",
    "0.00048828125",
    "0.0009765625",
    "0.001953125",
    "0.00390625",
    "0.0078125",
    "0.015625",
    "0.03125",
    "0.0625",
    "0.125",
    "0.25",
    "0.5"
  };
  for (uint64_t exp = 1; exp < 0x3ff; ++exp) {
    uint64_t bits = exp << 52;
    double value = 0;
    memcpy(&value, &bits, sizeof(double));

    int fixed_start = 1010, fixed_end = 1022;
    if (exp >= fixed_start && exp <= fixed_end) {
      EXPECT_EQ(dtoa(value), fixed[exp - fixed_start]);
      continue;
    }

    char expected[32] = {};
    *jkj::dragonbox::to_chars(value, expected) = '\0';

    EXPECT_EQ(dtoa(value), expected) << exp;
  }
}

TEST(dtoa_test, all_exponents) {
  const char* fixed[] = {
    "0.00012207031250000003",
    "0.00024414062500000005",
    "0.0004882812500000001",
    "0.0009765625000000002",
    "0.0019531250000000004",
    "0.003906250000000001",
    "0.007812500000000002",
    "0.015625000000000003",
    "0.03125000000000001",
    "0.06250000000000001",
    "0.12500000000000003",
    "0.25000000000000006",
    "0.5000000000000001",
    "1.0000000000000002"
  };
  for (uint64_t exp = 0; exp <= 0x3ff; ++exp) {
    uint64_t bits = (exp << 52) | 1;
    double value = 0;
    memcpy(&value, &bits, sizeof(double));

    int fixed_start = 1010, fixed_end = 1023;
    if (exp >= fixed_start && exp <= fixed_end) {
      EXPECT_EQ(dtoa(value), fixed[exp - fixed_start]);
      continue;
    }

    char expected[32] = {};
    *jkj::dragonbox::to_chars(value, expected) = '\0';

    EXPECT_EQ(dtoa(value), expected) << exp;
  }
}

TEST(dtoa_test, small_int) { EXPECT_EQ(dtoa(1), "1"); }

TEST(dtoa_test, zero) {
  EXPECT_EQ(dtoa(0), "0");
  EXPECT_EQ(dtoa(-0.0), "-0");
}

TEST(dtoa_test, inf) {
  EXPECT_EQ(dtoa(std::numeric_limits<double>::infinity()), "inf");
}

TEST(dtoa_test, nan) {
  EXPECT_EQ(dtoa(-std::numeric_limits<double>::quiet_NaN()), "-nan");
}

TEST(dtoa_test, shorter) {
  // A possibly shorter underestimate is picked (u' in Schubfach).
  EXPECT_EQ(dtoa(-4.932096661796888e-226), "-4.932096661796888e-226");

  // A possibly shorter overestimate is picked (w' in Schubfach).
  EXPECT_EQ(dtoa(3.439070283483335e+35), "3.439070283483335e+35");
}

TEST(dtoa_test, single_candidate) {
  // Only an underestimate is in the rounding region (u in Schubfach).
  EXPECT_EQ(dtoa(6.606854224493745e-17), "6.606854224493745e-17");

  // Only an overestimate is in the rounding region (w in Schubfach).
  EXPECT_EQ(dtoa(6.079537928711555e+61), "6.079537928711555e+61");
}

TEST(dtoa_test, boundary_cases) {
  EXPECT_EQ(dtoa(1.3588129002659584e-245), "1.3588129002659584e-245");
  EXPECT_EQ(dtoa(2.9802322387695312e-08), "2.9802322387695312e-08");
  EXPECT_EQ(dtoa(5.960464477539063e-08), "5.960464477539063e-08");
  EXPECT_EQ(dtoa(1.3076622631878654e+65), "1.3076622631878654e+65");
  EXPECT_EQ(dtoa(9.03725590277404e+159), "9.03725590277404e+159");
  EXPECT_EQ(dtoa(9.03725590277404e+160), "9.03725590277404e+160");
  EXPECT_EQ(dtoa(9.03725590277404e+161), "9.03725590277404e+161");
  EXPECT_EQ(dtoa(9.03725590277404e+162), "9.03725590277404e+162");
  EXPECT_EQ(dtoa(5.960464477539062e-07), "5.960464477539062e-07");
}

TEST(dtoa_test, fixed_with_zeros) {
  EXPECT_EQ(dtoa(43210.0), "43210");
  EXPECT_EQ(dtoa(43210.1), "43210.1");
  EXPECT_EQ(dtoa(10000), "10000");
}

#if !ZMIJ_C
TEST(dtoa_test, no_buffer) {
  double value = 6.62607015e-34;
  char buffer[zmij::double_buffer_size];
  auto end = zmij::write(buffer, sizeof(buffer), value);
  std::string result(buffer, end);
  EXPECT_EQ(result, "6.62607015e-34");
}

TEST(ftoa_test, no_buffer) {
  float value = 6.62607e-34;
  char buffer[zmij::float_buffer_size];
  auto end = zmij::write(buffer, sizeof(buffer), value);
  std::string result(buffer, end);
  EXPECT_EQ(result, "6.62607e-34");
}

TEST(dtoa_test, to_decimal) {
  zmij::dec_fp dec = zmij::to_decimal(6.62607015e-34);
  EXPECT_EQ(dec.sig, 66260701500000000);
  EXPECT_EQ(dec.exp, -50);
  EXPECT_EQ(dec.negative, false);

  dec = zmij::to_decimal(-6.62607015e-34);
  EXPECT_EQ(dec.sig, 66260701500000000);
  EXPECT_EQ(dec.exp, -50);
  EXPECT_EQ(dec.negative, true);

  dec = zmij::to_decimal(-0.0);
  EXPECT_EQ(dec.sig, 0);
  EXPECT_EQ(dec.exp, 0);
  EXPECT_EQ(dec.negative, true);

  uint32_t garlic = 0;
  memcpy(&garlic, "🧄", 4);
  uint64_t bits = 0x7FF0000000000000 | garlic;
  double garlic_nan = 0;
  memcpy(&garlic_nan, &bits, sizeof(bits));
  dec = zmij::to_decimal(garlic_nan);
  EXPECT_EQ(dec.sig, garlic);
}

TEST(ftoa_test, fixed_with_zeros) {
  EXPECT_EQ(ftoa(43210.0f), "43210");
  EXPECT_EQ(ftoa(43210.1f), "43210.1");
  EXPECT_EQ(ftoa(10000.f), "10000");
}
#endif  // ZMIJ_C

TEST(dtoa_test, no_overrun) {
  char buffer[zmij::double_buffer_size + 1];
  memset(buffer, '?', sizeof(buffer));
  auto end = zmij::write(buffer, zmij::double_buffer_size, -1.2345678901234567e+123);
  EXPECT_EQ(std::string(buffer, end), std::string("-1.2345678901234567e+123"));
  EXPECT_EQ(buffer[zmij::double_buffer_size], '?');
}

TEST(dtoa_test, no_underrun) {
  dtoa(9.061488e+15);
}

TEST(ftoa_test, normal) {
  EXPECT_EQ(ftoa(6.62607e-34f), "6.62607e-34");
  EXPECT_EQ(ftoa(1.342178e+08f), "1.342178e+08");
  EXPECT_EQ(ftoa(1.3421781e+08f), "1.3421781e+08");
}

TEST(ftoa_test, subnormal) {
  EXPECT_EQ(ftoa(std::numeric_limits<float>::denorm_min()), "1e-45");
}

TEST(ftoa_test, no_overrun) {
  char buffer[zmij::float_buffer_size + 1];
  memset(buffer, '?', sizeof(buffer));
  auto end = zmij::write(buffer, zmij::float_buffer_size, -1.00000005e+15f);
  EXPECT_EQ(std::string(buffer, end), std::string("-1.00000005e+15"));
  EXPECT_EQ(buffer[zmij::float_buffer_size], '?');
}

#if !ZMIJ_C
// ---- to_decimal(value, dtoa_mode, ndigits) ------------------------------
//
// The new overload implements CPython-compatible rounding modes:
//   dtoa_mode::shortest    - same output as the one-arg to_decimal.
//   dtoa_mode::significant - round to N significant digits.
//   dtoa_mode::fractional  - round to N digits past the decimal point
//                            (N may be negative).
// Trailing zeros are always stripped; `decpt = exp + num_digits` gives the
// classic _Py_dg_dtoa decimal-point position.

using zmij::dtoa_mode;

// Packs a dec_fp into a compact string so EXPECT_EQ failures are readable.
auto dec_fp_str(zmij::dec_fp d) -> std::string {
  if (d.exp == zmij::non_finite_exp)
    return std::string(d.negative ? "-" : "+") +
           (d.sig == 0 ? "inf" : "nan(" + std::to_string(d.sig) + ")");
  return std::string(d.negative ? "-" : "+") + std::to_string(d.sig) + "e" +
         std::to_string(d.exp);
}

auto fp(long long sig, int exp, bool neg = false) -> std::string {
  return dec_fp_str(zmij::dec_fp{sig, exp, neg});
}

auto round_ex(double v, dtoa_mode m, int n = 0) -> std::string {
  return dec_fp_str(zmij::to_decimal(v, m, n));
}

TEST(to_decimal_mode, shortest_matches_existing) {
  // Mode 0 must reproduce the shortest-form output of the one-arg overload
  // (after the one-arg overload's trailing-zero strip is applied).
  EXPECT_EQ(round_ex(1.0, dtoa_mode::shortest), fp(1, 0));
  EXPECT_EQ(round_ex(0.1, dtoa_mode::shortest), fp(1, -1));
  EXPECT_EQ(round_ex(0.125, dtoa_mode::shortest), fp(125, -3));
  EXPECT_EQ(round_ex(5.0507837461e-27, dtoa_mode::shortest),
            fp(50507837461LL, -37));
}

TEST(to_decimal_mode, shortest_zero_and_nonfinite) {
  EXPECT_EQ(round_ex(0.0, dtoa_mode::shortest), fp(0, 0));
  EXPECT_EQ(round_ex(-0.0, dtoa_mode::shortest), fp(0, 0, true));
  EXPECT_EQ(round_ex(0.0, dtoa_mode::fractional, 5), fp(0, 0));
  // inf/nan pass through regardless of mode.
  auto inf = std::numeric_limits<double>::infinity();
  auto r = zmij::to_decimal(inf, dtoa_mode::fractional, 3);
  EXPECT_EQ(r.exp, zmij::non_finite_exp);
  EXPECT_EQ(r.sig, 0);
  EXPECT_FALSE(r.negative);
}

TEST(to_decimal_mode, significant) {
  // 0.125 has shortest "125"; any ndigits >= 3 leaves it alone.
  EXPECT_EQ(round_ex(0.125, dtoa_mode::significant, 5), fp(125, -3));
  EXPECT_EQ(round_ex(0.125, dtoa_mode::significant, 3), fp(125, -3));
  EXPECT_EQ(round_ex(0.125, dtoa_mode::significant, 2), fp(12, -2));  // 0.12
  EXPECT_EQ(round_ex(0.125, dtoa_mode::significant, 1), fp(1, -1));   // 0.1

  // ndigits <= 0 is treated as 1 (like _Py_dg_dtoa).
  EXPECT_EQ(round_ex(0.125, dtoa_mode::significant, 0), fp(1, -1));
  EXPECT_EQ(round_ex(0.125, dtoa_mode::significant, -7), fp(1, -1));

  // The literal 9.95 is the double 9.94999999... (9.95 isn't exactly
  // representable); correct bit-exact rounding at 2 sig digits yields 9.9,
  // not 10.0. This is the case where the old shortest-then-round path
  // disagreed with _Py_dg_dtoa by 1 ulp.
  EXPECT_EQ(round_ex(9.95, dtoa_mode::significant, 2), fp(99, -1));
  // 9.5 is exactly representable; 2 sig digits is 9.5 unchanged.
  EXPECT_EQ(round_ex(9.5, dtoa_mode::significant, 2), fp(95, -1));
  // Genuine carry: 99.5 at 2 sig digits ties to even -> 100.
  EXPECT_EQ(round_ex(99.5, dtoa_mode::significant, 2), fp(1, 2));
  // 1.2345 literal is the double 1.23449999... (slightly below rational
  // 1.2345); bit-exact 4-sig-digit rounding picks 1.234.
  EXPECT_EQ(round_ex(1.2345, dtoa_mode::significant, 4), fp(1234, -3));
}

TEST(to_decimal_mode, fractional_positive_ndigits) {
  // 0.125 to 2 fractional digits: ties at 0.125 between 0.12 and 0.13,
  // last kept digit should be even -> 0.12.
  EXPECT_EQ(round_ex(0.125, dtoa_mode::fractional, 2), fp(12, -2));
  // 3 or more frac digits: shortest already covers it; exp stays at -3.
  EXPECT_EQ(round_ex(0.125, dtoa_mode::fractional, 3), fp(125, -3));
  EXPECT_EQ(round_ex(0.125, dtoa_mode::fractional, 10), fp(125, -3));

  // 1.5 with 0 frac digits: ties at 1.5 between 1 and 2; even is 2.
  EXPECT_EQ(round_ex(1.5, dtoa_mode::fractional, 0), fp(2, 0));
  EXPECT_EQ(round_ex(2.5, dtoa_mode::fractional, 0), fp(2, 0));
  EXPECT_EQ(round_ex(-1.5, dtoa_mode::fractional, 0), fp(2, 0, true));
}

TEST(to_decimal_mode, fractional_negative_ndigits) {
  // Round 15 to the tens place -> 20 (ties to even, 2 is even).
  EXPECT_EQ(round_ex(15.0, dtoa_mode::fractional, -1), fp(2, 1));
  EXPECT_EQ(round_ex(45.0, dtoa_mode::fractional, -1), fp(4, 1));  // 40
  EXPECT_EQ(round_ex(55.0, dtoa_mode::fractional, -1), fp(6, 1));  // 60
  EXPECT_EQ(round_ex(50.0, dtoa_mode::fractional, -1), fp(5, 1));  // 50

  // Rounding entirely below the requested scale.
  EXPECT_EQ(round_ex(15.0, dtoa_mode::fractional, -2), fp(0, 0));  // 0
  EXPECT_EQ(round_ex(50.0, dtoa_mode::fractional, -2), fp(0, 0));  // 50 ties -> 0
  EXPECT_EQ(round_ex(51.0, dtoa_mode::fractional, -2), fp(1, 2));  // 100
  EXPECT_EQ(round_ex(500.0, dtoa_mode::fractional, -3), fp(0, 0)); // 500 ties -> 0

  // Sign is preserved on a "round to zero" result.
  EXPECT_EQ(round_ex(-15.0, dtoa_mode::fractional, -2), fp(0, 0, true));
  EXPECT_EQ(round_ex(-51.0, dtoa_mode::fractional, -2), fp(1, 2, true));
}

TEST(to_decimal_mode, fractional_carry_across_digit_boundary) {
  // 9.5 -> 10 at 0 frac digits (carry increases digit count).
  EXPECT_EQ(round_ex(9.5, dtoa_mode::fractional, 0), fp(1, 1));   // 10
  // 99.5 -> 100.
  EXPECT_EQ(round_ex(99.5, dtoa_mode::fractional, 0), fp(1, 2));  // 100
  // 0.99 at 1 frac digit -> 1.0.
  EXPECT_EQ(round_ex(0.99, dtoa_mode::fractional, 1), fp(1, 0));
}

TEST(to_decimal_mode, fractional_subnormals_and_tiny) {
  // Denormal min is about 5e-324; coarser rounding should collapse it to 0.
  double tiny = std::numeric_limits<double>::denorm_min();
  EXPECT_EQ(round_ex(tiny, dtoa_mode::fractional, 5), fp(0, 0));
  EXPECT_EQ(round_ex(tiny, dtoa_mode::fractional, -1), fp(0, 0));
}
#endif  // !ZMIJ_C

auto main(int argc, char** argv) -> int {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
