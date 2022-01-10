#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "acc.hh"

int main(int argc, char* argv[]) {
  assert(argc == 3);
  FILE* f = fopen(argv[1], "r");
  fseek(f, 0, SEEK_END);
  uint32_t fs = ftell(f);
  fclose(f);

  uint8_t* streambuffer = nullptr;

  if (argv[2][0] == '0') {
    streambuffer = new uint8_t[fs];
  } else {
    streambuffer = reinterpret_cast<uint8_t*>(std::atol(argv[2]));
  }

  uint8_t* data = streambuffer;

  {
    std::ifstream is(argv[1], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(data), fs);
  }

  long long ans = acc::acc(data, 16, 5, fs);

  if (argv[2][0] == '0') delete[](streambuffer);
  return ans;
}
