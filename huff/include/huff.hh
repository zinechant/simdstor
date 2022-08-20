#ifndef HUFF_HUFF_HH_
#define HUFF_HUFF_HH_

#include <vector>

#include "dict.hh"

namespace huff {

typedef std::vector<uint8_t> Bytes;

template <typename T>
uint64_t Encode(const std::vector<T> &data, const Dict &dict, Bytes &coded);

template <typename T>
const std::vector<T> Decode(const Dict &dict, const Bytes &coded,
                            uint64_t bits);

}  // namespace huff

#endif  // HUFF_HUFF_HH_