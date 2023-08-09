#include <arm_sve.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <type_traits>

#include "debug.h"
#include "vsbytes.hh"

#define INFO(fmt, ...)                               \
  fprintf(stderr, "[   INFO   ] " fmt, __VA_ARGS__); \
  fflush(stderr);

template <typename T>
const T* jacobi2d(int kind, T* A, T* B, const int n, const int s) {
  static_assert(std::is_same<T, int32_t>::value);
  T* P[2] = {A, B};
  if (kind == 3) {
    svvrcnum();
    for (int t = 0; t < s; t++) {
      A = P[t & 1];
      B = P[1 - (t & 1)];
      for (int i = 1; i < n - 1; i++) {
        int r0 = frstream((const int8_t*)(A + i * n + 1), 32, (n - 2) << 5);
        int r1 = frstream((const int8_t*)(A + i * n), 32, (n - 2) << 5);
        int r2 = frstream((const int8_t*)(A + i * n + 2), 32, (n - 2) << 5);
        int r3 = frstream((const int8_t*)(A + i * n + 1 - n), 32, (n - 2) << 5);
        int r4 = frstream((const int8_t*)(A + i * n + 1 + n), 32, (n - 2) << 5);
        int w = fwstream((int8_t*)(B + i * n + 1), 32);
        for (int j = 1, e = 0; j < n - 1; j += e) {
          svint32_t v0 = svvunpack(svptrue_b8(), r0);
          svint32_t v1 =
              svvadd_m(svptrue_b8(), v0, svvunpack(svptrue_b8(), r0));
          svint32_t v2 =
              svvadd_m(svptrue_b8(), v1, svvunpack(svptrue_b8(), r1));
          svint32_t v3 =
              svvadd_m(svptrue_b8(), v2, svvunpack(svptrue_b8(), r2));
          svint32_t v4 =
              svvadd_m(svptrue_b8(), v3, svvunpack(svptrue_b8(), r3));
          svint32_t v5 = svvdiv_m(svptrue_b8(), v4, svvcpz(svptrue_b8(), 5));
          int f = svvpack(svptrue_b8(), v5, w);
          e = svvrcnum();
          assert(e == f);
          crstream(r0, e);
          crstream(r1, e);
          crstream(r2, e);
          crstream(r3, e);
          crstream(r4, e);
        }
        int64_t bl0 = erstream(r0);
        assert(!bl0);
        int64_t bl1 = erstream(r1);
        assert(!bl1);
        int64_t bl2 = erstream(r2);
        assert(!bl2);
        int64_t bl3 = erstream(r3);
        assert(!bl3);
        int64_t bl4 = erstream(r4);
        assert(!bl4);
        int64_t bw = ewstream(w);
        assert(bw == ((n - 2) << 5));
      }
    }
  } else if (kind == 2) {
    for (int t = 0; t < s; t++) {
      A = P[t & 1];
      B = P[1 - (t & 1)];
      for (int i = 1; i < n - 1; i++) {
        int r0 = frstream((const int8_t*)(A + i * n + 1), 32, (n - 2) << 5);
        int r1 = frstream((const int8_t*)(A + i * n), 32, (n - 2) << 5);
        int r2 = frstream((const int8_t*)(A + i * n + 2), 32, (n - 2) << 5);
        int r3 = frstream((const int8_t*)(A + i * n + 1 - n), 32, (n - 2) << 5);
        int r4 = frstream((const int8_t*)(A + i * n + 1 + n), 32, (n - 2) << 5);
        int w = fwstream((int8_t*)(B + i * n + 1), 32);
        for (int j = 1; j < n - 1; j += svcntw()) {
          svbool_t p = svwhilelt_b32_s32(j, n - 1);
          svint32_t v0 = svunpack_s32(r0);
          svint32_t v1 = svadd_s32_m(p, v0, svunpack_s32(r1));
          svint32_t v2 = svadd_s32_m(p, v1, svunpack_s32(r2));
          svint32_t v3 = svadd_s32_m(p, v2, svunpack_s32(r3));
          svint32_t v4 = svadd_s32_m(p, v3, svunpack_s32(r4));
          svint32_t v5 = svdiv_n_s32_m(p, v4, 5);
          svpack_s32(p, v5, w);
        }
        int64_t bl0 = erstream(r0);
        assert(!bl0);
        int64_t bl1 = erstream(r1);
        assert(!bl1);
        int64_t bl2 = erstream(r2);
        assert(!bl2);
        int64_t bl3 = erstream(r3);
        assert(!bl3);
        int64_t bl4 = erstream(r4);
        assert(!bl4);
        int64_t bw = ewstream(w);
        assert(bw == ((n - 2) << 5));
      }
    }
  } else if (kind == 1) {
    for (int t = 0; t < s; t++) {
      A = P[t & 1];
      B = P[1 - (t & 1)];
      for (int i = 1; i < n - 1; i++)
        for (int j = 1; j < n - 1; j += svcntw()) {
          svbool_t p = svwhilelt_b32_s32(j, n - 1);
          svint32_t v0 = svld1_s32(p, A + i * n + j);
          svint32_t v1 = svadd_s32_m(p, v0, svld1_s32(p, A + i * n + j - 1));
          svint32_t v2 = svadd_s32_m(p, v1, svld1_s32(p, A + i * n + j + 1));
          svint32_t v3 = svadd_s32_m(p, v2, svld1_s32(p, A + i * n + j - n));
          svint32_t v4 = svadd_s32_m(p, v3, svld1_s32(p, A + i * n + j + n));
          svint32_t v5 = svdiv_n_s32_m(p, v4, 5);
          svst1_s32(p, B + i * n + j, v5);
        }
    }
  } else {
    assert(kind == 0);
    for (int t = 0; t < s; t++) {
      A = P[t & 1];
      B = P[1 - (t & 1)];
      for (int i = 1; i < n - 1; i++)
        for (int j = 1; j < n - 1; j++)
          B[i * n + j] = (A[i * n + j] + A[i * n + j - 1] + A[i * n + 1 + j] +
                          A[i * n + j - n] + A[i * n + j + n]) /
                         5;
    }
  }

  return P[s & 1];
}
