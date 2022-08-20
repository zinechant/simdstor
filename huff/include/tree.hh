#ifndef HUFF_TREE_HH_
#define HUFF_TREE_HH_

#include <cstdint>
#include <type_traits>
#include <vector>

template <typename T>
const std::vector<T> &tree(const std::vector<double>& prob, const uint8_t cb) {
  static_assert(std::is_same<T, uint16_t> || std::is_same<T, uint32_t>,
                "Only uint16_t or uint32_t supported");
  uint32_t symbols = prob.size();
  assert(!(symbols >> cb));
  std::vector<std::pair<double, uint32_t>> ps;
  for (uint32_t i = 0; i < symbols; i++) {
    ps.emplace_back(prob[i], i);
  }
  std::sort

}

#endif