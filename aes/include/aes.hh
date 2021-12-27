#ifndef AES_HH_
#define AES_HH_

#include "gf_common.h"

// Implemented in GF(2^8) mod 0x11d instead of 0x11b.

namespace aes {

using GF::W;
using GF::W2Power;

void Init(unsigned char x);

unsigned int GetPaddingLength(unsigned int len);

unsigned int EncryptCTR(uint8_t GF_MUL[][W2Power], uint8_t sbox[],
                        unsigned char in[], unsigned int in_len,
                        unsigned char key[], unsigned char out[]);

unsigned int DecryptCTR(uint8_t GF_MUL[][W2Power], uint8_t sbox[],
                        uint8_t inv_sbox[], unsigned char in[],
                        unsigned int in_len, unsigned char key[],
                        unsigned char out[]);

}  // namespace aes

#endif  // AES_HH_