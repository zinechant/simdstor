#include <arm_sve.h>

#include <cassert>
#include <cstdint>
#include <type_traits>

#include "vsbytes.hh"

template <typename T>
void bitpack(int kind, const T* input, int n, uint8_t* output, int w) {
  if (kind == 3) {
    const int iw = sizeof(T) << 3;
    int si = frstream((const int8_t*)input, iw, n * iw);
    int so = fwstream((int8_t*)output, w);

    for (int i = 0, e = 0; i < n; i += e) {
      int ee = svvpack(svptrue_b8(), svvunpack(svptrue_b8(), si), so);
      e = svvrcnum();
      crstream(si, e);
      assert(ee == e);
    }

    int64_t bl = erstream(si);
    assert(!bl);
    int64_t bw = ewstream(so);
    assert(bw == n * w);
  } else if (kind == 2) {
    const int iw = sizeof(T) << 3;
    int si = frstream((const int8_t*)input, iw, n * iw);
    int so = fwstream((int8_t*)output, w);

    if constexpr (std::is_same<T, int8_t>::value) {
      for (int i = 0; i < n; i += svcntb())
        svpack_s8(svptrue_b8(), svunpack_s8(si), so);
    } else if constexpr (std::is_same<T, int16_t>::value) {
      for (int i = 0; i < n; i += svcnth())
        svpack_s16(svptrue_b16(), svunpack_s16(si), so);
    } else if constexpr (std::is_same<T, int32_t>::value) {
      for (int i = 0; i < n; i += svcntw())
        svpack_s32(svptrue_b32(), svunpack_s32(si), so);
    } else if constexpr (std::is_same<T, int64_t>::value) {
      for (int i = 0; i < n; i += svcntd())
        svpack_s64(svptrue_b64(), svunpack_s64(si), so);
    }
    int64_t bl = erstream(si);
    assert(!bl);
    int64_t bw = ewstream(so);
    assert(bw == n * w);
  } else {
    assert(kind == 0 || kind == 1);
    uint8_t pos = 0;
    for (int i = 0; i < n; i++) {
      PackBits(output, pos, w, input[i]);
      AdvanceBits(output, pos, w);
    }
  }
}
