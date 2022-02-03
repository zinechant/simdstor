#ifndef ANS_HH_
#define ANS_HH_

#include <cstdint>
#include <memory>

#include "table.hh"

namespace ans {
Table* Init(uint8_t*& scratchpad, int bytes, uint8_t data[]);

// Returns bits or -1 when failure
int Encode(const Table* t, int bytes, const uint8_t data[], uint8_t code[]);

// Returns bytes or -1 when failure
int Decode(const Table* t, int bits, const uint8_t code[], uint8_t data[]);

}  // namespace ans

#endif  // ANS_HH_
