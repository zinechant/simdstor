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
T* gemm(int kind, const T* A, const T* B, T* C, T* D, const T a, const T b,
        const int m, const int n, const int l) {
  static_assert(std::is_same<T, int32_t>::value);
  if (kind == 3) {
    T* const PP[2] = {C, D};
    constexpr int BS = 8;
    int rb[BS];
    svvrcnum();

    for (int i = 0; i < m; i++) {
      for (int kk = 0; kk < (n - 1) / BS + 1; kk++) {
        const int rc =
            frstream((const int8_t*)(PP[kk & 1] + i * l), 32, l << 5);
        const int wd = fwstream((int8_t*)(PP[1 - (kk & 1)] + i * l), 32);
        const int bs = (BS < n - kk * BS) ? BS : n - kk * BS;
        for (int k = 0; k < bs; k++) {
          rb[k] = frstream((const int8_t*)(B + (kk * BS + k) * l), 32, l << 5);
        }

        for (int j = 0, e = 0; j < l; j += e) {
          svint32_t vc = svvunpack(svptrue_b8(), rc);
          if (kk == 0) {
            svint32_t vn = svvcpz(svptrue_b8(), b);
            vc = svvmul_m(svptrue_b8(), vc, vn);
          }
          for (int k = 0; k < bs; k++) {
            svint32_t vb = svvunpack(svptrue_b8(), rb[k]);
            svint32_t vn = svvcpz(svptrue_b8(), a * A[i * n + kk * BS + k]);
            vc = svvadd_m(svptrue_b8(), vc, svvmul_m(svptrue_b8(), vb, vn));
          }
          e = svvpack(svptrue_b8(), vc, wd);
          const int f = svvrcnum();
          assert(e == f);
          for (int k = 0; k < bs; k++) {
            crstream(rb[k], e);
          }
          crstream(rc, e);
        }

        for (int k = 0; k < bs; k++) {
          int64_t bl = erstream(rb[k]);
          assert(!bl);
        }
        int64_t bl = erstream(rc);
        assert(!bl);
        int64_t bw = ewstream(wd);
        assert(bw == (l << 5));
      }
    }
    return PP[((n - 1) / BS + 1) & 1];
  } else if (kind == 2) {
    for (int i = 0; i < m; i++) {
      int rc = frstream((const int8_t*)(C + i * l), 32, l << 5);
      int wd = fwstream((int8_t*)(D + i * l), 32);
      for (int j = 0; j < l; j += svcntw()) {
        svbool_t p = svwhilelt_b32_s32(j, l);
        svint32_t vc = svunpack_s32(rc);
        svint32_t vd = svmul_n_s32_x(p, vc, b);
        for (int k = 0; k < n; k++) {
          svint32_t vb = svld1_s32(p, B + k * l + j);
          svint32_t ve = svmul_n_s32_x(p, vb, a * A[i * n + k]);
          vd = svadd_s32_x(p, vd, ve);
        }
        svpack_s32(p, vd, wd);
      }
      int64_t bl = erstream(rc);
      assert(!bl);
      int64_t bw = ewstream(wd);
      assert(bw == (l << 5));
    }
    return D;
  } else if (kind == 1) {
    for (int i = 0; i < m; i++)
      for (int j = 0; j < l; j += svcntw()) {
        svbool_t p = svwhilelt_b32_s32(j, l);
        svint32_t vc = svld1_s32(p, C + i * l + j);
        svint32_t vd = svmul_n_s32_x(p, vc, b);
        for (int k = 0; k < n; k++) {
          svint32_t vb = svld1_s32(p, B + k * l + j);
          svint32_t ve = svmul_n_s32_x(p, vb, a * A[i * n + k]);
          vd = svadd_s32_x(p, vd, ve);
        }
        svst1_s32(p, D + i * l + j, vd);
      }
    return D;
  } else {
    assert(kind == 0);
    for (int i = 0; i < m; i++)
      for (int j = 0; j < l; j++) {
        D[i * l + j] = C[i * l + j] * b;
        for (int k = 0; k < n; k++) {
          D[i * l + j] += a * A[i * n + k] * B[k * l + j];
        }
      }
    return D;
  }
}
