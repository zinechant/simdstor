#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "raid4.hh"

using raid4::K;
using raid4::M;
using raid4::N;

int main(int argc, char* argv[]) {
  assert(argc == 4);
  FILE* f = fopen(argv[1], "r");
  fseek(f, 0, SEEK_END);
  uint32_t fs = ftell(f);
  fclose(f);

  uint32_t bs = fs / M + 1;
  fs = bs * N;

  uint8_t* streambuffer = nullptr;

  if (argv[3][0] == '0') {
    streambuffer = new uint8_t[fs];
  } else {
    streambuffer = reinterpret_cast<uint8_t*>(std::atol(argv[3]));
  }

  uint8_t* data = streambuffer;

  {
    std::ifstream is(argv[1], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(data), fs);
  }

  uint8_t* m0 = data;
  uint8_t* m1 = m0 + bs;
  uint8_t* m2 = m1 + bs;
  uint8_t* parity = m2 + bs;

  static_assert(K == 1);
  dprintf("m0 = %p\n", m0);
  dprintf("m1 = %p\n", m1);
  dprintf("m2 = %p\n", m2);
  dprintf("parity = %p\n", parity);
  dprintf("bs = %d\n", bs);

  raid4::Encode(bs, m0, m1, m2, parity);
  if (argv[2][0] == 't') {
    raid4::Decode(bs, m0, m1, m2, parity);
  }

  if (argv[3][0] == '0') delete[](streambuffer);
}
