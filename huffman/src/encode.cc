#include "huffman.hh"

const bytes_t encode(const codeids_t& data, const codes_t& codes) {
  bytes_t ans;
  uint8_t byte = 0;
  uint8_t width = 0;
  const int BW = sizeof(uint8_t) << 3;

  for (auto& l : data) {
    int i = codes[l].first;
    int j = codes[l].second;
    while (i) {
      int k = i < BW - width ? i : BW - width;
      int x = j & ((1 << k) - 1);
      byte += x << width;

      j >>= k;
      width += k;
      i -= k;
      if (width == BW) {
        ans.push_back(byte);
        width = 0;
        byte = 0;
      }
    }
  }

  ans.push_back(byte);
  ans.push_back(0);
  ans.push_back(0);
  ans.push_back(0);
  ans.push_back(width);

  return ans;
}
