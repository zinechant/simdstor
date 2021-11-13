#include "raid6.hh"

#include <cassert>

namespace raid6 {

void Encode(int size, uint8_t* message[], uint8_t* parity[]) {
  for (int k = 0; k < size; k++) {
    uint8_t p = 0, q = 0;
    for (int i = 0; i < M; i++) {
      p ^= message[i][k];
      q ^= GF_MUL[1 << i][message[i][k]];
    }
    parity[0][k] = p;
    parity[1][k] = q;
  }
}

void Decode(int size, uint8_t* message[], uint8_t* parity[]) {
  for (int k = 0; k < size; k++) {
    uint8_t p = parity[0][k];
    uint8_t q = parity[1][k];

    for (int i = 2; i < M; i++) {
      p ^= message[i][k];
      q ^= GF_MUL[1 << i][message[i][k]];
    }

    uint8_t y = GF_DIV[q ^ p][3];
    uint8_t x = p ^ y;

    dprintf("x = 0x%x, message[0][k] = 0x%x\n", x, message[0][k]);
    dprintf("y = 0x%x, message[1][k] = 0x%x\n", y, message[1][k]);

    assert(x == message[0][k]);
    assert(y == message[1][k]);

    message[0][k] = x;
    message[1][k] = y;
  }
}

}  // namespace raid6
