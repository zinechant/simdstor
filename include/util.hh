#ifndef UTIL_HH_
#define UTIL_HH_

#include <fstream>

inline int filesize(const char* filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

#endif  // UTIL_HH_