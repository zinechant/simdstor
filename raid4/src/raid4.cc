#include "raid4.hh"

#include <cassert>

namespace raid4 {

void Encode(int size, uint8_t m0[], uint8_t m1[], uint8_t m2[],
            uint8_t parity[]) {
  for (int k = 0; k < size; k++) {
    m2[k] = m0[k] ^ m1[k] ^ m2[k];
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
