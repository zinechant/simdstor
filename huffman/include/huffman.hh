#ifndef HUFF_HH_
#define HUFF_HH_

#include <cinttypes>
#include <vector>

typedef union pointer {
  uint32_t b32;
  uint16_t b16;
  uint8_t b08;
} pointer_t;

const int MAX_DIGITS = 16;
typedef uint16_t codeword_t;
typedef uint16_t codeid_t;
typedef uint8_t byte_t;
static_assert((sizeof(uint16_t) << 3) >= MAX_DIGITS);

typedef std::vector<std::pair<codeword_t, codeword_t>> codes_t;
typedef std::vector<codeword_t> dictionary_t;
typedef std::vector<codeword_t> codeids_t;
typedef std::vector<byte_t> bytes_t;

// const codes_t& codes_gen(int num, int seed);
const codes_t codes_gen(int num, int seed);
const codeids_t seq_gen(int num, int len, int seed);
const dictionary_t dic_gen(const codes_t& codes);

const bytes_t encode(const codeids_t& data, const codes_t& codes);
const codeids_t decode(const bytes_t& encoded, const dictionary_t& dictionary,
                      const codes_t& codes);

#endif  // HUFF_HH_
