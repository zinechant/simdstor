#include "raid4.hh"

#include <cassert>

#if !defined(__ARM_FEATURE_SVE)

namespace raid4 {

void Encode(int size, uint8_t m0[], uint8_t m1[], uint8_t m2[],
            uint8_t parity[]) {
  for (int k = 0; k < size; k++) {
    parity[k] = m0[k] ^ m1[k] ^ m2[k];
  }
}

void Decode(int size, uint8_t m0[], uint8_t m1[], uint8_t m2[],
            uint8_t parity[]) {
  for (int k = 0; k < size; k++) {
    uint8_t p = parity[k] ^ m1[k] ^ m2[k];
    assert(p == m0[k]);
    m0[k] = p;
  }
}

}  // namespace raid4

#else

#include <arm_sve.h>

namespace raid4 {

void Encode(int size, uint8_t m0[], uint8_t m1[], uint8_t m2[],
            uint8_t parity[]) {
  int k = 0;
  for (svbool_t pg = svwhilelt_b8(k, size); k < size; pg = svwhilelt_b8(k, size)) {
    svuint8_t v0 = svld1_u8(pg, m0 + k);
    svuint8_t v1 = svld1_u8(pg, m1 + k);
    svuint8_t v2 = svld1_u8(pg, m2 + k);
    v0 = sveor_u8_x(pg, v0, v1);
    v0 = sveor_u8_x(pg, v0, v2);
    svst1_u8(pg, parity + k, v0);
    k += svcntb();
  }
}

void Decode(int size, uint8_t m0[], uint8_t m1[], uint8_t m2[],
            uint8_t parity[]) {
  int k = 0;
  for (svbool_t pg = svwhilelt_b8(k, size); k < size; pg = svwhilelt_b8(k, size)) {
    svuint8_t v0 = svld1_u8(pg, parity + k);
    svuint8_t v1 = svld1_u8(pg, m1 + k);
    svuint8_t v2 = svld1_u8(pg, m2 + k);
    v0 = sveor_u8_x(pg, v0, v1);
    v0 = sveor_u8_x(pg, v0, v2);

    assert(!svptest_any(svptrue_b8(), svcmpne(pg, v0, svld1_u8(pg, m0 + k))));

    svst1_u8(pg, m0 + k, v0);
    k += svcntb();
  }
}

}  // namespace raid4

#endif /* __ARM_FEATURE_SVE */
