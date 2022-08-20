#include "dict.hh"

#include <cassert>
#include <cstdio>
#include <cstring>

#include "util.hh"

namespace huff {

void Dict::InitEnc(const uint8_t* data) {
  uint32_t ibytes = kSymbols << (kCodeBits <= 8 ? 1 : 2);
  enc.resize(ibytes + 8);
  std::memcpy(enc.data() + 8, data, ibytes);
  enc[4] = kSymbolBits;
  enc[5] = kCodeBits;
}

void Dict::InitDec() {
  const uint16_t* data16 = reinterpret_cast<const uint16_t*>(enc.data() + 8);
  const uint32_t* data32 = reinterpret_cast<const uint32_t*>(enc.data() + 8);

  dec.resize(8 + (1 << (kCodeBits + (kSymbolBits <= 8 ? 1 : 2))));
  dec[4] = kSymbolBits;
  dec[5] = kCodeBits;
  uint16_t* dic16 = reinterpret_cast<uint16_t*>(dec.data() + 8);
  uint32_t* dic32 = reinterpret_cast<uint32_t*>(dec.data() + 8);
  for (uint32_t i = 0; i < kSymbols; i++) {
    int32_t x = kCodeBits <= 8 ? data16[i] : data32[i];
    uint8_t w = x & 127;
    x = (x >> 8) & ((1 << w) - 1);
    uint32_t y = (i << 8) + w;
    for (uint32_t j = 0; j < (1 << kCodeBits); j += (1 << w)) {
      if (kSymbolBits <= 8) {
        dic16[x | j] = y;
      } else {
        dic32[x | j] = y;
      }
    }
  }
}

void Dict::Init(const uint8_t* data) {
  assert(kSymbols && !(kSymbols >> kCodeBits));
  InitEnc(data);
  InitDec();
}

Dict::Dict(const std::vector<uint16_t>& enc, uint8_t cbits)
    : kSymbols(enc.size()),
      kSymbolBits(32 - __builtin_clz(kSymbols)),
      kCodeBits(cbits) {
  assert(kCodeBits && (kCodeBits <= 8));
  Init(reinterpret_cast<const uint8_t*>(enc.data()));
}
Dict::Dict(const std::vector<uint32_t>& enc, uint8_t cbits)
    : kSymbols(enc.size()),
      kSymbolBits(32 - __builtin_clz(kSymbols)),
      kCodeBits(cbits) {
  assert(kCodeBits && (kCodeBits <= 24));
  Init(reinterpret_cast<const uint8_t*>(enc.data()));
}

Dict::Dict(const char* inpath) {
  uint32_t bytes = filesize(inpath);
  enc.resize(bytes);

  FILE* in = fopen(inpath, "rb");
  fread(enc.data(), 1, bytes, in);
  fclose(in);

  kSymbolBits = enc[4];
  kCodeBits = enc[5];
  kSymbols = *reinterpret_cast<uint32_t*>(enc.data());

  assert(kCodeBits && kCodeBits <= 24);
  assert(kSymbols && !(kSymbols >> kCodeBits));
  assert(kSymbolBits == 32 - __builtin_clz(kSymbols));
  assert(bytes == 8 + (kSymbols << (kCodeBits <= 8 ? 1 : 2)));

  InitDec();
}

const uint8_t* Dict::GetEnc(uint32_t elems) {
  *reinterpret_cast<uint32_t*>(enc.data()) = elems;
  return enc.data();
}
const uint8_t* Dict::GetDec(uint32_t elems) {
  *reinterpret_cast<uint32_t*>(enc.data()) = elems;
  return enc.data();
}

std::pair<uint8_t, uint32_t> Dict::Encode(uint32_t sid) const {
  assert(sid < kSymbols);
  uint32_t x =
      kCodeBits <= 8
          ? *reinterpret_cast<const uint16_t*>(enc.data() + 8 + (sid << 1))
          : *reinterpret_cast<const uint32_t*>(enc.data() + 8 + (sid << 2));
  return std::make_pair<uint8_t, uint32_t>(x & 127, x >> 8);
}

std::pair<uint8_t, uint32_t> Dict::Decode(uint32_t code) const {
  assert(!(code >> kCodeBits));
  uint32_t x =
      kSymbolBits <= 8
          ? *reinterpret_cast<const uint16_t*>(dec.data() + 8 + (code << 1))
          : *reinterpret_cast<const uint32_t*>(dec.data() + 8 + (code << 2));
  return std::make_pair<uint8_t, uint32_t>(x & 127, x >> 8);
}

void Dict::save(const char* outpath) {
  FILE* out = fopen(outpath, "wb");
  const uint8_t* data = GetEnc(kSymbols);
  uint32_t bytes = (kSymbols << (kCodeBits <= 8 ? 1 : 2)) + 8;
  fwrite(data, 1, bytes, out);
  fclose(out);
}

}  // namespace huff