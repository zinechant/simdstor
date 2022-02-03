#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "ans.hh"

int main(int argc, char* argv[]) {
  assert(argc == 5);
  FILE* f = fopen(argv[1], "r");
  fseek(f, 0, SEEK_END);
  const int fs = ftell(f);
  fclose(f);

  const int bs = ((fs >> 10) + 1) << 10;
  const int ss = (sizeof(ans::decode_t) << (2 + (sizeof(uint16_t) << 3)));

  uint8_t* scratchpad = nullptr;
  uint8_t* streambuffer = nullptr;

  if (argv[3][0] == '0') {
    scratchpad = new uint8_t[ss];
  } else {
    scratchpad = reinterpret_cast<uint8_t*>(std::atol(argv[3]));
  }

  if (argv[4][0] == '0') {
    streambuffer = new uint8_t[bs + bs + bs];
  } else {
    streambuffer = reinterpret_cast<uint8_t*>(std::atol(argv[4]));
  }

  uint8_t* data = streambuffer;
  {
    std::ifstream is(argv[1], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(data), fs);
  }
  uint8_t* code = streambuffer + bs;
  uint8_t* comp = streambuffer + bs + bs;

  ans::Table* t = ans::Init(scratchpad, fs, data);

  int bits = ans::Encode(t, fs, data, code);
  printf("coded/raw: %.4lf\n", (((double)bits) / (fs << 3)));
  if (argv[2][0] == 't') {
    int bytes = ans::Decode(t, bits, code, comp);
    assert(bytes == fs);
    for (int i = 0; i < bytes; i++) {
      assert(comp[i] == data[i]);
    }
  }

  if (argv[3][0] != '0') delete[](scratchpad);
  if (argv[4][0] != '0') delete[](streambuffer);
}
