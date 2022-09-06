#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <random>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "huff.hh"

template <typename T>
huff::Dict dictgen(const uint8_t cbits, const uint32_t num, int seed) {
  assert(cbits && cbits <= 24);
  assert((std::is_same<uint16_t, T>::value && cbits <= 8) ||
         (std::is_same<uint32_t, T>::value && cbits > 8));

  std::mt19937 mt(seed);
  int count[cbits + 1];
  std::vector<uint32_t> arr;

  uint32_t s = 0;
  do {
    s = 0;
    std::unordered_set<uint32_t> f;
    f.clear();
    arr.clear();
    for (uint8_t i = 0; i <= cbits; i++) {
      count[i] = 0;
    }
    for (uint32_t i = 0; i < num; i++) {
      uint32_t x = mt();
      for (; f.count(x); x = mt())
        ;
      f.insert(x);
      arr.push_back(x);
    }
    std::sort(arr.begin(), arr.end());

    for (uint32_t i = 0; i < num; i++) {
      assert((!i) || (arr[i] != arr[i - 1]));
      double d = ((double)(i ? arr[i] - arr[i - 1] : arr[i])) / mt.max();
      int x = 0.54 - log(d) / log(2);
      x = x > cbits ? cbits : x;
      s += 1 << (cbits - x);
      count[x]++;
    }
    printf("new random dic, s = %x (%.8lf) .\n", s, (s + 0.0) / (1 << cbits));
  } while (s >> cbits);

  std::vector<T> ans;

  for (uint32_t x = 0, i = 1; i <= cbits; i++) {
    while (count[i]--) {
      uint32_t y = 0;
      for (uint32_t k = 0, z = x++; k < i; k++) {
        y <<= 1;
        y += z & 1;
        z >>= 1;
      }
      ans.emplace_back(i | (y << 8));
    }
    x <<= 1;
  }
  return huff::Dict(ans, cbits);
}

template <typename T>
const std::vector<T> seq_gen(int num, int len, int seed) {
  assert(!(num >> 24));
  assert((std::is_same<uint8_t, T>::value && num <= 256) ||
         (std::is_same<uint16_t, T>::value && num <= 65536) ||
         (std::is_same<uint32_t, T>::value));

  std::mt19937 mt(seed);
  std::vector<T> ans;

  for (int i = 0; i < len; i++) {
    ans.push_back(mt() % num);
  }

  return ans;
}

template <typename Symbol_t, typename Entry_t>
void test(uint32_t kSymbols, uint8_t cbits, const char* path) {
  assert(!(kSymbols >> cbits));
  assert(cbits && cbits <= 24);

  assert((std::is_same<uint8_t, Symbol_t>::value && kSymbols <= 256) ||
         (std::is_same<uint16_t, Symbol_t>::value && kSymbols <= 65536) ||
         (std::is_same<uint32_t, Symbol_t>::value));
  assert((std::is_same<uint16_t, Entry_t>::value && cbits <= 8) ||
         (std::is_same<uint32_t, Entry_t>::value));

  printf("data gening...\n");
  fflush(stdout);
  std::vector<Symbol_t> data = seq_gen<Symbol_t>(kSymbols, 1024 << 10, 0);
  printf("data gened, dict gening...\n");
  fflush(stdout);

  huff::Bytes encoded;
  huff::Dict dict = dictgen<Entry_t>(cbits, kSymbols, 0);
  printf("dict gened, encoding...\n");
  fflush(stdout);
  uint64_t bits = huff::Encode<Symbol_t>(data, dict, encoded);
  printf("encoded, decoding...\n");
  fflush(stdout);
  std::vector<Symbol_t> decoded = huff::Decode<Symbol_t>(dict, encoded, bits);
  printf("decoded, comparing...\n");
  fflush(stdout);
  assert(decoded == data);
  printf("compared... sling...\n");
  fflush(stdout);

  dict.save(path);
  huff::Dict dict2(path);

  huff::Bytes enc2;
  printf("dict loaded, encoding...\n");
  uint64_t bits2 = huff::Encode<Symbol_t>(data, dict2, enc2);
  assert(bits == bits2);
  assert(encoded == enc2);
  printf("encoded, decoding...\n");
  std::vector<Symbol_t> decoded2 = huff::Decode<Symbol_t>(dict2, enc2, bits);
  assert(decoded == decoded2);
  printf("decoded, correct...\n");
}

int main() {
  test<uint8_t, uint16_t>(1 << 5, 8, "dict0.save");
  test<uint8_t, uint32_t>(1 << 5, 16, "dict1.save");
  test<uint16_t, uint32_t>(1 << 12, 18, "dict2.save");
  test<uint32_t, uint32_t>(1 << 18, 24, "dict3.save");

  test<uint8_t, uint16_t>(1 << 5, 8, "dict4.save");
  test<uint8_t, uint32_t>(1 << 5, 16, "dict5.save");
  test<uint16_t, uint32_t>(1 << 12, 18, "dict6.save");
  test<uint32_t, uint32_t>(1 << 18, 24, "dict7.save");

  test<uint8_t, uint16_t>(1 << 5, 8, "dict8.save");
  test<uint8_t, uint32_t>(1 << 5, 16, "dict9.save");
  test<uint16_t, uint32_t>(1 << 12, 18, "dict10.save");
  test<uint32_t, uint32_t>(1 << 18, 24, "dict11.save");

  test<uint8_t, uint16_t>(1 << 5, 8, "dict12.save");
  test<uint8_t, uint32_t>(1 << 5, 16, "dict13.save");
  test<uint16_t, uint32_t>(1 << 12, 18, "dict14.save");
  test<uint32_t, uint32_t>(1 << 18, 24, "dict15.save");

  return 0;
}
