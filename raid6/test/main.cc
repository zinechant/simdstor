#include <cassert>
#include <cstdio>
#include <fstream>

#include "raid6.hh"

int main(int argc, char* argv[]) {
  assert(argc == 3);
  FILE* f = fopen(argv[1], "r");
  fseek(f, 0, SEEK_END);
  uint32_t fs = ftell(f);

  uint32_t bs = fs / M + 1;
  fs = bs * N;
  uint8_t* data = new uint8_t[fs];

  std::ifstream is(argv[1], std::ifstream::binary);
  is.read(reinterpret_cast<char*>(data), fs);

  uint8_t* message[M];
  uint8_t* parity[K];
  uint8_t* d = data;
  for (int i = 0; i < M; i++) {
    message[i] = d;
    d += bs;
  }
  for (int i = 0; i < K; i++) {
    parity[i] = d;
    d += bs;
  }

  raid6::Encode(bs, message, parity);
  if (argv[2][0] == 't') {
    raid6::Decode(bs, message, parity);
  }

  delete[](data);
}
