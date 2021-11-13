#ifndef AES_HH_
#define AES_HH_

// Implemented in GF(2^8) mod 0x11d instead of 0x11b.

namespace aes {

void Init(unsigned char x);

unsigned int GetPaddingLength(unsigned int len);

unsigned int EncryptCTR(unsigned char in[], unsigned int in_len,
                        unsigned char key[], unsigned char out[]);

unsigned int DecryptCTR(unsigned char in[], unsigned int in_len,
                        unsigned char key[], unsigned char out[]);

}  // namespace aes

#endif  // AES_HH_