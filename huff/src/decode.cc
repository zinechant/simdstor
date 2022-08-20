#include "huff.hh"
#include "vsbytes.hh"

namespace huff {

template <typename T>
const std::vector<T> Decode(const Dict &dict, const Bytes &coded,
                            uint64_t bits) {
  std::vector<T> data;
  const uint8_t *psrc = coded.data();
  uint8_t pos = 0;

  for (uint8_t bs = 0; bits; bits -= bs) {
    uint32_t code = UnpackBits(psrc, pos, dict.CodeBits());
    const auto p = dict.Decode(code);
    bs = p.first;
    uint32_t sid = p.second;
    data.push_back(sid);
    AdvanceBits(psrc, pos, bs);
  }
  return data;
}

template const std::vector<uint8_t> Decode<uint8_t>(const Dict &dict,
                                                    const Bytes &coded,
                                                    uint64_t bits);
template const std::vector<uint16_t> Decode<uint16_t>(const Dict &dict,
                                                      const Bytes &coded,
                                                      uint64_t bits);
template const std::vector<uint32_t> Decode<uint32_t>(const Dict &dict,
                                                      const Bytes &coded,
                                                      uint64_t bits);

}  // namespace huff