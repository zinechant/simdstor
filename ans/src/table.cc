#include "table.hh"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <utility>

namespace ans {

static_assert(LL < 15);

// Precise quantization - minimizing dH~sum (p_i-q_i)^2/p_i
void Table::Quantize(uint8_t scratchpad[], double ip[]) {
  int l = 1 << ll_;

  double* pl = reinterpret_cast<double*>(scratchpad);
  scratchpad += sizeof(double) * l;

  int used = 0;
  for (int i = 0; i < symbols_; i++) {
    pl[i] = l / ip[i];
    freq_[i] = int(pl[i] + 0.5);
    if (!freq_[i]) freq_[i] = 1;
    used += freq_[i];
  }

  if (used == l) return;

  int sgn = (used > l) ? -1 : 1;
  auto v = reinterpret_cast<std::pair<double, uint16_t>*>(scratchpad);
  scratchpad += sizeof(v[0]) * symbols_;

  int u = 0;
  for (int i = 0; i < symbols_; i++) {
    if (freq_[i] + sgn) {
      v[u++] = {sgn * (2 * pl[i] - 2 * freq_[i] - sgn) * ip[i], i};
    }
  }

  std::make_heap(v, v + u);
  for (; used != l; used += sgn) {
    std::pop_heap(v, v + u);
    int i = v[--u].second;
    assert(i < symbols_);

    freq_[i] += sgn;
    if (freq_[i] + sgn) {
      v[u++] = {sgn * (2 * pl[i] - 2 * freq_[i] - sgn) * ip[i], i};
      std::push_heap(v, v + u);
    }
  }
}

// using i/p = i*ip as the approximation of 1/(p*ln(1+1/i))
void Table::Spread(uint8_t scratchpad[], double ip[]) {
  auto v = reinterpret_cast<std::pair<double, uint16_t>*>(scratchpad);
  scratchpad += sizeof(std::pair<double, uint16_t>) << ll_;
  auto f = reinterpret_cast<uint16_t*>(scratchpad);
  scratchpad += MAX_SYMBOLS * sizeof(uint16_t);

  int u = 0;
  for (int s = 0; s < symbols_; s++) {
    if (s == 0) {
      pencode_[s] = encode_;
    } else {
      pencode_[s] = pencode_[s - 1] + freq_[s - 1];
    }
    f[s] = 0;
    for (int i = freq_[s]; i < freq_[s] + freq_[s]; i++)
      v[u++] = {((double)i) * ip[s], s};
  }
  assert(u == (1 << ll_));

  std::sort(v, v + u);
  for (int i = 0; i < (1 << ll_); i++) {
    decode_[i].symbol = v[i].second;

    int x = f[decode_[i].symbol]++;
    pencode_[decode_[i].symbol][x] = i + (1 << ll_);

    x += freq_[decode_[i].symbol];
    decode_[i].bits =
        (sizeof(int) << 3) - 1 - __builtin_clz(((2 << ll_) - 1) / x);
    decode_[i].state = x;
    assert((x << decode_[i].bits) >= (1 << ll_) &&
           (x << decode_[i].bits) < (2 << ll_));
  }
}

void Table::Init(uint8_t*& scratchpad, int bytes, uint8_t data[], int ll) {
  ll_ = ll;

  freq_ = reinterpret_cast<uint16_t*>(scratchpad);
  scratchpad += sizeof(uint16_t) * MAX_SYMBOLS;

  encode_ = reinterpret_cast<uint16_t*>(scratchpad);
  scratchpad += sizeof(uint16_t) << ll_;

  pencode_ = reinterpret_cast<uint16_t**>(scratchpad);
  scratchpad += sizeof(uint16_t**) * MAX_SYMBOLS;

  decode_ = reinterpret_cast<decode_t*>(scratchpad);
  scratchpad += sizeof(decode_t) << ll_;

  double* ip = reinterpret_cast<double*>(scratchpad);
  scratchpad += MAX_SYMBOLS * sizeof(double);
  for (int i = 0; i < MAX_SYMBOLS; i++) ip[i] = 0;

  symbols_ = 0;
  for (int i = 0; i < bytes; i++) {
    ip[data[i]] += 1;
    symbols_ = data[i] > symbols_ ? data[i] : symbols_;
  }
  symbols_++;
  for (int i = 0; i < symbols_; i++) ip[i] = bytes / ip[i];

  Quantize(scratchpad, ip);
  Spread(scratchpad, ip);
}

}  //  namespace ans
