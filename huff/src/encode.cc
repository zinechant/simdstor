#include "huff.hh"
#include "vsbytes.hh"

namespace huff {

template <typename T>
uint64_t Encode(const std::vector<T> &data, const Dict &dict, Bytes &coded) {
  coded.resize(((dict.CodeBits() * data.size()) >> 3) + 32);
  uint8_t *pdest = coded.data();
  uint64_t bits = 0;
  uint8_t pos = 0;

  for (auto &l : data) {
    const auto p = dict.Encode(l);
    uint8_t bs = p.first;
    uint32_t code = p.second;

    PackBits(pdest, pos, bs, code);
    AdvanceBits(pdest, pos, bs);
    bits += bs;
  }
  coded.resize((bits + 7) >> 3);

  return bits;
}

template uint64_t Encode<uint8_t>(const std::vector<uint8_t> &data,
                                  const Dict &dict, Bytes &coded);
template uint64_t Encode<uint16_t>(const std::vector<uint16_t> &data,
                                   const Dict &dict, Bytes &coded);
template uint64_t Encode<uint32_t>(const std::vector<uint32_t> &data,
                                   const Dict &dict, Bytes &coded);

}  // namespace huff