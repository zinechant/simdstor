#ifndef ANS_TABLE_HH_
#define ANS_TABLE_HH_

#include <cstdint>

namespace ans {

const int LL = 12;
const int MAX_SYMBOLS = 1 << 8;
const int START = 1 << LL;

typedef struct {
  uint8_t symbol;
  uint8_t bits;
  // state doesn't contain renormalization shifts.
  uint16_t state;
} decode_t; /* total 8 bytes */

class Table {
 private:
  int symbols_;
  int ll_;
  uint16_t* freq_;
  uint16_t* encode_;
  decode_t* decode_;
  uint16_t** pencode_;

  void Quantize(uint8_t scratchpad[], double ip[]);
  void Spread(uint8_t scratchpad[], double ip[]);

 public:
  void Init(uint8_t*& scratchpad, int bytes, uint8_t data[], int ll = LL);

  int symbols() const { return symbols_; }
  int ll() const { return ll_; }
  uint16_t freq(uint8_t s) const { return freq_[s]; }
  uint16_t encode(uint8_t s, uint16_t x) const { return pencode_[s][x]; }
  const decode_t& decode(uint16_t x) const { return decode_[x]; }
};

}  // namespace ans

#endif  // ANS_TABLE_HH_
