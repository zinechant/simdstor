#include "raid5.hh"

#include <arm_sve.h>

#include <cassert>

namespace raid5 {

void Encode(int size, int kind, const int32_t m0[], const int32_t m1[],
            const int32_t m2[], int32_t mp[]) {
  if (kind == 3) {
    int s0 = frstream((const int8_t*)m0, 32, size << 5);
    int s1 = frstream((const int8_t*)m1, 32, size << 5);
    int s2 = frstream((const int8_t*)m2, 32, size << 5);
    int sp = fwstream((int8_t*)mp, 32);
    for (int i = 0, n = 0; i < size; i += n) {
      svint32_t v = svvunpack(svptrue_b8(), s0);
      v = svveor_m(svptrue_b8(), v, svvunpack(svptrue_b8(), s1));
      v = svveor_m(svptrue_b8(), v, svvunpack(svptrue_b8(), s2));
      int nn = svvpack(svptrue_b8(), v, sp);
      n = svvrcnum();
      assert(nn == n);
      crstream(s0, n);
      crstream(s1, n);
      crstream(s2, n);
    }
    uint64_t b0 = erstream(s0);
    uint64_t b1 = erstream(s1);
    uint64_t b2 = erstream(s2);
    assert(!b0 && !b1 && !b2);
    int64_t bp = ewstream(sp);
    assert(bp == (size << 5));
  } else if (kind == 2) {
    int s0 = frstream((const int8_t*)m0, 32, size << 5);
    int s1 = frstream((const int8_t*)m1, 32, size << 5);
    int s2 = frstream((const int8_t*)m2, 32, size << 5);
    int sp = fwstream((int8_t*)mp, 32);
    for (int i = 0; i < size; i += svcntw()) {
      svbool_t pg = svwhilelt_b32_s32(i, size);
      svint32_t v = svunpack_s32(s0);
      v = sveor_m(pg, v, svunpack_s32(s1));
      v = sveor_m(pg, v, svunpack_s32(s2));
      svpack_s32(pg, v, sp);
    }
    uint64_t b0 = erstream(s0);
    uint64_t b1 = erstream(s1);
    uint64_t b2 = erstream(s2);
    assert(!b0 && !b1 && !b2);
    int64_t bp = ewstream(sp);
    assert(bp == (size << 5));
  } else if (kind == 1) {
    for (int i = 0; i < size; i += svcntw()) {
      svbool_t pg = svwhilelt_b32_s32(i, size);
      svint32_t v = svld1_s32(pg, m0);
      v = sveor_m(pg, v, svld1_s32(pg, m1));
      v = sveor_m(pg, v, svld1_s32(pg, m2));
      svst1_s32(pg, mp, v);

      m0 += svcntw();
      m1 += svcntw();
      m2 += svcntw();
      mp += svcntw();
    }
  } else {
    assert(kind == 0);
    for (int i = 0; i < size; i++) {
      mp[i] = m0[i] ^ m1[i] ^ m2[i];
    }
  }
}

}  // namespace raid5
