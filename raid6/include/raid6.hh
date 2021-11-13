#ifndef RAID6_HH_
#define RAID6_HH_

#include <cstdint>

#include "erasure.hh"

static_assert(K == 2, "Wrong parity number");

namespace raid6 {

void Encode(int size, uint8_t* message[], uint8_t* parity[]);

void Decode(int size, uint8_t* message[], uint8_t* parity[]);

}  // namespace raid6

#endif  // RAID6_HH_
