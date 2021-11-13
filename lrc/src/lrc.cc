#include "lrc.hh"

#include <cassert>

static_assert(K == 2, "Wrong parity number");

namespace lrc {

void Encode(int size, uint8_t* message[], uint8_t* parity[]) {
  for (int k = 0; k < size; k++) {
    uint8_t p = 0, q = 0, r = 0, s = 0;
    for (int i = 0; i < M; i++) {
      p ^= message[i][k];
      q ^= message[i + M][k];
      r ^= GF_MUL[A[i]][message[i][k]] ^ GF_MUL[B[i]][message[i + M][k]];
      s ^= GF_MUL[C[i]][message[i][k]] ^ GF_MUL[D[i]][message[i + M][k]];
    }
    parity[0][k] = p;
    parity[1][k] = q;
    parity[2][k] = r;
    parity[3][k] = s;
  }
}

void Decode(int size, uint8_t* message[], uint8_t* parity[]) {
  for (int k = 0; k < size; k++) {
    uint8_t p = parity[0][k];
    uint8_t r = parity[2][k];
    uint8_t s = parity[3][k];

    for (int i = 0; i < M; i++) {
      if (i > 2) {
        p ^= message[i][k];
        r ^= GF_MUL[A[i]][message[i][k]] ^ GF_MUL[B[i]][message[i + M][k]];
        s ^= GF_MUL[C[i]][message[i][k]] ^ GF_MUL[D[i]][message[i + M][k]];
      } else {
        r ^= GF_MUL[B[i]][message[i + M][k]];
        s ^= GF_MUL[D[i]][message[i + M][k]];
      }
    }

    // p =       m0 ^       m1 ^       m2
    // r =    a0*m0 ^    a1*m1 ^    a2*m2
    // s = a0*a0*m0 ^ a1*a1*m1 ^ a2*a2*m2

    r ^= GF_MUL[A[0]][p];
    s ^= GF_MUL[C[0]][p];

    // p = m0 ^          m1 ^               m2
    // r =       (a1+a0)*m1 ^       (a2+a0)*m2
    // s = (a1*a1+a0*a0)*m1 ^ (a2*a2+a0*a0)*m2

    uint8_t e = GF_DIV[1][A[0] ^ A[1]];
    uint8_t f = GF_DIV[1][C[0] ^ C[1]];
    uint8_t t = GF_MUL[r][e] ^ GF_MUL[s][f];

    // t = (e * (a2 + a0) + f * (a2*a2 + a0*a0)) m2
    uint8_t z = GF_DIV[t][GF_MUL[e][A[0] ^ A[2]] ^ GF_MUL[f][C[0] ^ C[2]]];
    uint8_t y = GF_DIV[r ^ GF_MUL[A[0] ^ A[2]][z]][A[0] ^ A[1]];
    uint8_t x = p ^ y ^ z;

    dprintf("x = 0x%x, message[0][k] = 0x%x\n", x, message[0][k]);
    dprintf("y = 0x%x, message[1][k] = 0x%x\n", y, message[1][k]);
    dprintf("z = 0x%x, message[2][k] = 0x%x\n", z, message[2][k]);

    assert(x == message[0][k]);
    assert(y == message[1][k]);
    assert(z == message[2][k]);

    message[0][k] = x;
    message[1][k] = y;
    message[2][k] = z;
  }
}

}  // namespace lrc
