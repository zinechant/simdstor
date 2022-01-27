#ifndef RAID4_HH_
#define RAID4_HH_

#include "erasure.hh"

namespace raid4 {

const int M = 3;
const int K = 1;
const int N = M + K;

void Encode(int size, uint8_t m0[], uint8_t m1[], uint8_t m2[],
            uint8_t parity[]);

void Decode(int size, uint8_t m0[], uint8_t m1[], uint8_t m2[],
            uint8_t parity[]);

}  // namespace raid4

#endif  // RAID4_HH_
