#include "aes.hh"

#include <cassert>
#include <cstring>

#include "debug.h"

namespace aes {

const int Nb = 4;
const int LOG_BLOCK_BYTES = 4;
const int BLOCK_BYTES = 1 << LOG_BLOCK_BYTES;
static int Nk = 0;
static int Nr = 0;

void Init(unsigned char x) {
  assert(x == 4 || x == 6 || x == 8);
  Nk = x;
  Nr = x + 6;
}

inline void SubWord(uint8_t sbox[], unsigned char *a) {
  int i;
  for (i = 0; i < 4; i++) {
    a[i] = sbox[a[i]];
  }
}

inline void RotWord(unsigned char *a) {
  unsigned char c = a[0];
  a[0] = a[1];
  a[1] = a[2];
  a[2] = a[3];
  a[3] = c;
}

inline void XorWords(unsigned char *a, unsigned char *b, unsigned char *c) {
  int i;
  for (i = 0; i < 4; i++) {
    c[i] = a[i] ^ b[i];
  }
}

inline unsigned char xtime(unsigned char b)  // multiply on x
{
  return (b << 1) ^ (((b >> 7) & 1) * 0x1b);
}

void Rcon(unsigned char *a, int n) {
  int i;
  unsigned char c = 1;
  for (i = 0; i < n - 1; i++) {
    c = xtime(c);
  }

  a[0] = c;
  a[1] = a[2] = a[3] = 0;
}

void KeyExpansion(uint8_t sbox[], unsigned char key[], unsigned char w[]) {
  unsigned char temp[4];
  unsigned char rcon[4];

  int i = 0;
  while (i < 4 * Nk) {
    w[i] = key[i];
    i++;
  }

  i = 4 * Nk;
  while (i < 4 * Nb * (Nr + 1)) {
    temp[0] = w[i - 4 + 0];
    temp[1] = w[i - 4 + 1];
    temp[2] = w[i - 4 + 2];
    temp[3] = w[i - 4 + 3];

    if (i / 4 % Nk == 0) {
      RotWord(temp);
      SubWord(sbox, temp);
      Rcon(rcon, i / (Nk * 4));
      XorWords(temp, rcon, temp);
    } else if (Nk > 6 && i / 4 % Nk == 4) {
      SubWord(sbox, temp);
    }

    w[i + 0] = w[i - 4 * Nk] ^ temp[0];
    w[i + 1] = w[i + 1 - 4 * Nk] ^ temp[1];
    w[i + 2] = w[i + 2 - 4 * Nk] ^ temp[2];
    w[i + 3] = w[i + 3 - 4 * Nk] ^ temp[3];
    i += 4;
  }
}

void SubBytes(uint8_t sbox[], unsigned char state[][Nb]) {
  int i, j;
  unsigned char t;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < Nb; j++) {
      t = state[i][j];
      state[i][j] = sbox[t];
    }
  }
}

// shift row i on n positions
void ShiftRow(unsigned char state[][Nb], int i, int n) {
  unsigned char tmp[Nb];
  for (int j = 0; j < Nb; j++) {
    tmp[j] = state[i][(j + n) % Nb];
  }
  memcpy(state[i], tmp, Nb * sizeof(unsigned char));
}

void ShiftRows(unsigned char state[][Nb]) {
  ShiftRow(state, 1, 1);
  ShiftRow(state, 2, 2);
  ShiftRow(state, 3, 3);
}

/* Implementation taken from
 * https://en.wikipedia.org/wiki/Rijndael_mix_columns#Implementation_example */
void MixSingleColumn(unsigned char *r) {
  unsigned char a[4];
  unsigned char b[4];
  unsigned char c;
  /* The array 'a' is simply a copy of the input array 'r'
   * The array 'b' is each element of the array 'a' multiplied by 2
   * in Rijndael's Galois field
   * a[n] ^ b[n] is element n multiplied by 3 in Rijndael's Galois field */
  for (c = 0; c < 4; c++) {
    a[c] = r[c];
    b[c] = (r[c] << 1) ^ ((r[c] & 0x80) ? 0x1d : 0);
    // https://sthbrx.github.io/blog/2017/03/20/erasure-coding-for-programmers-part-1/
    // Section: Back to RAID 6. explains why b can be calculated this way.
  }
  r[0] = b[0] ^ a[3] ^ a[2] ^ b[1] ^ a[1]; /* 2 * a0 + a3 + a2 + 3 * a1 */
  r[1] = b[1] ^ a[0] ^ a[3] ^ b[2] ^ a[2]; /* 2 * a1 + a0 + a3 + 3 * a2 */
  r[2] = b[2] ^ a[1] ^ a[0] ^ b[3] ^ a[3]; /* 2 * a2 + a1 + a0 + 3 * a3 */
  r[3] = b[3] ^ a[2] ^ a[1] ^ b[0] ^ a[0]; /* 2 * a3 + a2 + a1 + 3 * a0 */
}

/* Performs the mix columns step. Theory from:
 * https://en.wikipedia.org/wiki/Advanced_Encryption_Standard#The_MixColumns_step
 */
void MixColumns(unsigned char state[][Nb]) {
  unsigned char temp[4];

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      temp[j] = state[j][i];  // place the current state column in temp
    }
    MixSingleColumn(temp);  // mix it using the wiki implementation
    for (int j = 0; j < 4; ++j) {
      state[j][i] =
          temp[j];  // when the column is mixed, place it back into the state
    }
  }
}

void AddRoundKey(unsigned char state[][Nb], unsigned char *key) {
  int i, j;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < Nb; j++) {
      state[i][j] = state[i][j] ^ key[i + 4 * j];
    }
  }
}

void InvSubBytes(uint8_t inv_sbox[], unsigned char state[][Nb]) {
  int i, j;
  unsigned char t;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < Nb; j++) {
      t = state[i][j];
      state[i][j] = inv_sbox[t];
    }
  }
}

void InvMixColumns(uint8_t GF_MUL[][W2Power], unsigned char state[][Nb]) {
  unsigned char s[4], s1[4];
  int i, j;

  for (j = 0; j < Nb; j++) {
    for (i = 0; i < 4; i++) {
      s[i] = state[i][j];
    }
    s1[0] = GF_MUL[0x0e][s[0]] ^ GF_MUL[0x0b][s[1]] ^ GF_MUL[0x0d][s[2]] ^
            GF_MUL[0x09][s[3]];
    s1[1] = GF_MUL[0x09][s[0]] ^ GF_MUL[0x0e][s[1]] ^ GF_MUL[0x0b][s[2]] ^
            GF_MUL[0x0d][s[3]];
    s1[2] = GF_MUL[0x0d][s[0]] ^ GF_MUL[0x09][s[1]] ^ GF_MUL[0x0e][s[2]] ^
            GF_MUL[0x0b][s[3]];
    s1[3] = GF_MUL[0x0b][s[0]] ^ GF_MUL[0x0d][s[1]] ^ GF_MUL[0x09][s[2]] ^
            GF_MUL[0x0e][s[3]];

    for (i = 0; i < 4; i++) {
      state[i][j] = s1[i];
    }
  }
}

void InvShiftRows(unsigned char state[][Nb]) {
  ShiftRow(state, 1, Nb - 1);
  ShiftRow(state, 2, Nb - 2);
  ShiftRow(state, 3, Nb - 3);
}

inline void XorBlocks(unsigned char *a, unsigned char *b, unsigned char *c,
                      unsigned int len) {
  for (unsigned int i = 0; i < len; i++) {
    c[i] = a[i] ^ b[i];
  }
}

void EncryptBlock(uint8_t sbox[], unsigned char in[], unsigned char out[],
                  unsigned char *roundKeys) {
  unsigned char state[4][Nb];
  int i, j, round;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < Nb; j++) {
      state[i][j] = in[i + 4 * j];
    }
  }

  AddRoundKey(state, roundKeys);

  for (round = 1; round <= Nr - 1; round++) {
    SubBytes(sbox, state);
    ShiftRows(state);
    MixColumns(state);
    AddRoundKey(state, roundKeys + round * 4 * Nb);
  }

  SubBytes(sbox, state);
  ShiftRows(state);
  AddRoundKey(state, roundKeys + Nr * 4 * Nb);

  for (i = 0; i < 4; i++) {
    for (j = 0; j < Nb; j++) {
      out[i + 4 * j] = state[i][j];
    }
  }
}

void DecryptBlock(uint8_t GF_MUL[][W2Power], uint8_t inv_sbox[],
                  unsigned char in[], unsigned char out[],
                  unsigned char *roundKeys) {
  unsigned char state[4][Nb];
  int i, j, round;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < Nb; j++) {
      state[i][j] = in[i + 4 * j];
    }
  }

  AddRoundKey(state, roundKeys + Nr * 4 * Nb);

  for (round = Nr - 1; round >= 1; round--) {
    InvSubBytes(inv_sbox, state);
    InvShiftRows(state);
    AddRoundKey(state, roundKeys + round * 4 * Nb);
    InvMixColumns(GF_MUL, state);
  }

  InvSubBytes(inv_sbox, state);
  InvShiftRows(state);
  AddRoundKey(state, roundKeys);

  for (i = 0; i < 4; i++) {
    for (j = 0; j < Nb; j++) {
      out[i + 4 * j] = state[i][j];
    }
  }
}

unsigned int GetPaddingLength(unsigned int len) {
  int loglen = LOG_BLOCK_BYTES;
  unsigned int blocks = len >> loglen;
  blocks++;
  return blocks << loglen;
}

inline void PaddingPKCS7(unsigned char in[], unsigned int in_len,
                         unsigned int padded_len) {
  unsigned char *out = in + in_len - 1;
  assert(padded_len - in_len < 256);
  for (unsigned char i = 1; i <= padded_len - in_len; i++) {
    out[i] = i;
  }
}

void CTR(uint8_t GF_MUL[][W2Power], unsigned char x, unsigned char *block) {
  for (int i = 0; i < BLOCK_BYTES; i++) {
    x = GF_MUL[7][x];
    block[i] = x;
  }
}

unsigned int EncryptCTR(uint8_t GF_MUL[][W2Power], uint8_t sbox[],
                        unsigned char in[], unsigned int in_len,
                        unsigned char key[], unsigned char out[]) {
  unsigned char block[BLOCK_BYTES];

  unsigned int out_len = GetPaddingLength(in_len);
  PaddingPKCS7(in, in_len, out_len);
  for (int i = 0; i < BLOCK_BYTES; i++) {
    dprintf("0x%02x ", int(in[out_len - BLOCK_BYTES + i]));
  }
  dprintf("%s", "\n");

  unsigned char *roundKeys = new unsigned char[4 * Nb * (Nr + 1)];
  KeyExpansion(sbox, key, roundKeys);
  for (int i = 0; i < 4 * Nb * (Nr + 1); i++) {
    dprintf("0x%02x ", int(roundKeys[i]));
  }
  dprintf("%s", "\n");

  for (unsigned int i = 0; i < out_len; i += BLOCK_BYTES) {
    CTR(GF_MUL, i, block);
    XorBlocks(block, in + i, block, BLOCK_BYTES);
    EncryptBlock(sbox, block, out + i, roundKeys);
  }

  delete[] roundKeys;
  return out_len;
}

unsigned int DecryptCTR(uint8_t GF_MUL[][W2Power], uint8_t sbox[],
                        uint8_t inv_sbox[], unsigned char in[],
                        unsigned int in_len, unsigned char key[],
                        unsigned char out[]) {
  unsigned char block[BLOCK_BYTES];

  unsigned char *roundKeys = new unsigned char[4 * Nb * (Nr + 1)];
  KeyExpansion(sbox, key, roundKeys);
  for (int i = 0; i < 4 * Nb * (Nr + 1); i++) {
    dprintf("0x%02x ", int(roundKeys[i]));
  }
  dprintf("%s", "\n");

  for (unsigned int i = 0; i < in_len; i += BLOCK_BYTES) {
    DecryptBlock(GF_MUL, inv_sbox, in + i, out + i, roundKeys);
    CTR(GF_MUL, i, block);
    XorBlocks(block, out + i, out + i, BLOCK_BYTES);
  }

  unsigned int out_len = in_len - out[in_len - 1];
  delete[] roundKeys;
  return out_len;
}

}  // namespace aes