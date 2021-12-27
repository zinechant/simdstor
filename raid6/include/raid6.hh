#ifndef RAID6_HH_
#define RAID6_HH_

#include "erasure.hh"
#include "gf_common.h"

namespace raid6 {

using erasure::K;
using erasure::M;
using erasure::N;

using GF::W;
using GF::W2Power;

static_assert(K == 2, "Wrong parity number");

void Encode(uint8_t GF_MUL[][W2Power], uint8_t GF_INV[], int size,
            uint8_t* message[], uint8_t* parity[]);

void Decode(uint8_t GF_MUL[][W2Power], uint8_t GF_INV[], int size,
            uint8_t* message[], uint8_t* parity[]);

}  // namespace raid6

#endif  // RAID6_HH_
