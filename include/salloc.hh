#ifndef SALLOC_HH_
#define SALLOC_HH_

#include <cassert>
#include <cinttypes>
#include <fstream>

#include "debug.h"
#include "util.hh"

inline int8_t* spalloc(unsigned size) {
  static int8_t* spaddr = (int8_t*)(1024UL << 30);
  int8_t* ans = spaddr;
  spaddr += size;
  assert(((unsigned long)spaddr) < (1025UL << 30));
  return ans;
}

inline int8_t* sballoc(unsigned size) {
  static int8_t* sbaddr = (int8_t*)(1025UL << 30);
  int8_t* ans = sbaddr;
  sbaddr += size;
  assert(((unsigned long)sbaddr) < (1028UL << 30));
  return ans;
}

inline int8_t* filedata(const char* filepath, bool sbNsp = true) {
  int fs = filesize(filepath);
  int8_t* data = sbNsp ? sballoc(fs) : spalloc(fs);
  iprintf("%s: %d %p\n", filepath, fs, data);
  std::ifstream is(filepath, std::ifstream::binary);
  is.read(reinterpret_cast<char*>(data), fs);
  return data;
}

inline void filewrite(const char* filepath, unsigned bits, const int8_t* data) {
  int bytes = (bits + 7) >> 3;
  std::ofstream os(filepath, std::ofstream::binary);
  os.write(reinterpret_cast<const char*>(data), bytes);
}

#endif  // SALLOC_HH_
