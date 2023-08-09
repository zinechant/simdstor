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
void trisolv(int kind, const T* A, const T* C, T* D, const int n) {
  static_assert(std::is_same<T, int32_t>::value);
  if (kind == 3) {
    svvrcnum();
    D[0] = C[0] + A[0];
    for (int i = 1; i < n; i++) {
      D[i] = C[i];
      int ra = frstream((const int8_t*)(A + i * n), 32, i << 5);
      int rd = frstream((const int8_t*)(D), 32, i << 5);
      for (int j = 0, e = 0; j < i; j += e) {
        svint32_t va = svvunpack(svptrue_b8(), ra);
        svint32_t vd = svvunpack(svptrue_b8(), rd);
        D[i] -= svvaddv(svptrue_b8(), svvmul_m(svptrue_b8(), va, vd));

        e = svvrcnum();
        crstream(ra, e);
        crstream(rd, e);
      }
      D[i] += A[i * n + i];

      int64_t bla = erstream(ra);
      assert(!bla);
      int64_t bld = erstream(rd);
      assert(!bld);
    }
  } else if (kind == 2) {
    D[0] = C[0] + A[0];
    for (int i = 1; i < n; i++) {
      D[i] = C[i];
      int ra = frstream((const int8_t*)(A + i * n), 32, i << 5);
      int rd = frstream((const int8_t*)(D), 32, i << 5);
      for (int j = 0; j < i; j += svcntw()) {
        svbool_t p = svwhilelt_b32_s32(j, i);
        svint32_t va = svunpack_s32(ra);
        svint32_t vd = svunpack_s32(rd);
        D[i] -= svaddv_s32(p, svmul_s32_m(p, va, vd));
      }
      D[i] += A[i * n + i];

      int64_t bla = erstream(ra);
      assert(!bla);
      int64_t bld = erstream(rd);
      assert(!bld);
    }
  } else if (kind == 1) {
    for (int i = 0; i < n; i++) {
      D[i] = C[i];
      for (int j = 0; j < i; j += svcntw()) {
        svbool_t p = svwhilelt_b32_s32(j, i);
        svint32_t va = svld1_s32(p, A + i * n + j);
        svint32_t vd = svld1_s32(p, D + j);
        D[i] -= svaddv_s32(p, svmul_s32_m(p, va, vd));
      }
      D[i] += A[i * n + i];
    }
  } else {
    assert(kind == 0);
    for (int i = 0; i < n; i++) {
      D[i] = C[i];
      for (int j = 0; j < i; j++) D[i] -= A[i * n + j] * D[j];
      D[i] += A[i * n + i];
    }
  }
}
