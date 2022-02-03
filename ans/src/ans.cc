#include "ans.hh"

#include <cassert>

namespace ans {

static inline void Push(uint8_t code[], int& pos, uint8_t width, uint16_t x) {
  for (uint8_t w = width; w;) {
    uint8_t avail = 8 - (pos & 7);
    avail = avail < w ? avail : w;

    uint8_t y = x & ((1 << avail) - 1);
    x >>= avail;

    code[pos >> 3] += y << (pos & 7);

    pos += avail;
    w -= avail;
  }
}

// bits [pos, 0] are available
static inline void Pop(const uint8_t code[], int& pos, uint8_t width,
                       uint16_t& x) {
  for (uint8_t w = width; w;) {
    uint8_t avail = (pos & 7) + 1;
    uint8_t y = code[pos >> 3] & ((1 << avail) - 1);
    uint8_t rest = avail < w ? 0 : avail - w;
    uint8_t used = avail - rest;
    x = (x << used) + (y >> rest);
    w -= used;
    pos -= used;
  }
}

int Encode(const Table* t, int bytes, const uint8_t data[], uint8_t code[]) {
  for (int i = 0; i < bytes; i++) code[i] = 0;

  int bits = 0;
  uint16_t x = START;
  for (int i = 0; i < bytes; i++) {
    uint16_t f = t->freq(data[i]);
    uint16_t y = (sizeof(int) << 3) - __builtin_clz(x / f) - 1;
    Push(code, bits, y, x & ((1 << y) - 1));
    x = x >> y;
    assert(x >= f && x < f + f);
    x = t->encode(data[i], x - f);
    assert(t->decode(x - (1 << t->ll())).bits == y);
    assert(t->decode(x - (1 << t->ll())).symbol == data[i]);
    assert(1 == (x >> t->ll()));
  }
  assert(1 == (x >> t->ll()));
  Push(code, bits, 16, x);

  return bits;
}

int Decode(const Table* t, int bits, const uint8_t code[], uint8_t data[]) {
  bits--;
  int s = 0;

  uint16_t x = 0;
  Pop(code, bits, 16, x);
  assert(1 == (x >> t->ll()));

  while (bits > -1) {
    x -= (1 << t->ll());
    data[s++] = t->decode(x).symbol;
    uint8_t b = t->decode(x).bits;
    x = t->decode(x).state;
    Pop(code, bits, b, x);
    assert(1 == (x >> t->ll()));
  }

  assert(x == (1 << t->ll()));

  for (int i = 0; i < (s >> 1); i++) {
    uint8_t tmp = data[i];
    data[i] = data[s - 1 - i];
    data[s - 1 - i] = tmp;
  }

  return s;
}

Table* Init(uint8_t*& scratchpad, int bytes, uint8_t data[]) {
  int s = ((sizeof(Table) >> 10) + 1) << 10;
  Table* ans = reinterpret_cast<Table*>(scratchpad);
  scratchpad += s;
  ans->Init(scratchpad, bytes, data);
  return ans;
}

}  // namespace ans
