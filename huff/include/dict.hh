#ifndef HUFF_DICT_HH_
#define HUFF_DICT_HH_


#include <cinttypes>
#include <vector>

namespace huff {

class Dict {
 private:
  uint32_t kSymbols;
  uint8_t kSymbolBits;
  uint8_t kCodeBits;
  std::vector<uint8_t> enc;
  std::vector<uint8_t> dec;

  void InitEnc(const uint8_t* data);
  void InitDec();
  void Init(const uint8_t* data);

 public:
  Dict(const std::vector<uint16_t>& enc, uint8_t cbits);
  Dict(const std::vector<uint32_t>& enc, uint8_t cbits);
  Dict(const char* inpath);

  const uint8_t* GetEnc(uint32_t elems);
  const uint8_t* GetDec(uint32_t elems);

  uint32_t Symbols() const { return kSymbols; }
  uint8_t SymbolBits() const { return kSymbolBits; }
  uint8_t CodeBits() const { return kCodeBits; }

  std::pair<uint8_t, uint32_t> Encode(uint32_t sid) const;
  std::pair<uint8_t, uint32_t> Decode(uint32_t code) const;

  void save(const char* outpath);
};

}  // namespace huff

#endif  // HUFF_DICT_HH_