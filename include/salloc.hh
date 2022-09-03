#include <cassert>
#include <cinttypes>

inline int8_t* spalloc(unsigned size) {
  static int8_t* sbaddr = (int8_t*)(1024UL << 30);
  int8_t* ans = sbaddr;
  sbaddr += size;
  assert(((unsigned long)sbaddr) < (1025UL << 30));
  return ans;
}

inline int8_t* sballoc(unsigned size) {
  static int8_t* spaddr = (int8_t*)(1025UL << 30);
  int8_t* ans = spaddr;
  spaddr += size;
  assert(((unsigned long)spaddr) < (1026UL << 30));
  return ans;
}
