#ifndef SALLOC_HH_
#define SALLOC_HH_

#include <cassert>
#include <cinttypes>

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
  assert(((unsigned long)sbaddr) < (1026UL << 30));
  return ans;
}

inline int8_t* filedata(const char* filepath, bool sbNsp = true) {
  int fs = filesize(filepath);
  int8_t* data = sbNsp ? sballoc(fs) : spalloc(fs);
  iprintf("%s: %d %p\n", filepath, fs, data);
  fflush(stderr);
  FILE* fi = fopen(filepath, "rb");
  const int BS = 4 << 10;  // gem5 strange
  int8_t* p = data;
  for (int i = 0; i < fs; i += BS) {
    fread(p, 1, i + BS < fs ? BS : fs - i, fi);
    p += BS;
  }
  fclose(fi);
  return data;
}

void filewrite(const char* filepath, unsigned bits, int8_t* data) {
  int bytes = (bits + 7) >> 3;
  FILE* fo = fopen(filepath, "wb");
  const int BS = 4 << 10;  // gem5 strange
  int8_t* p = data;
  for (int i = 0; i < bytes; i += BS) {
    fwrite(p, 1, i + BS < bytes ? BS : bytes - i, fo);
    p += BS;
  }
  fclose(fo);
}

#endif  // SALLOC_HH_
