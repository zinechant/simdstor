#include <cassert>
#include <cstdio>
#include <fstream>

#include "lrc.hh"

int main(int argc, char* argv[]) {
  assert(argc == 2);
  FILE* f = fopen(argv[1], "r");
  fseek(f, 0, SEEK_END);
  uint32_t fs = ftell(f);

  uint32_t bs = fs / (M + M) + 1;
  fs = bs * (N + N);
  uint8_t* data = new uint8_t[fs];

  std::ifstream is(argv[1], std::ifstream::binary);
  is.read(reinterpret_cast<char*>(data), fs);

  uint8_t* message[M + M];
  uint8_t* parity[K + K];
  uint8_t* d = data;
  for (int i = 0; i < M + M; i++) {
    message[i] = d;
    d += bs;
  }
  for (int i = 0; i < K + K; i++) {
    parity[i] = d;
    d += bs;
  }

  lrc::Encode(bs, message, parity);
  lrc::Decode(bs, message, parity);

  delete[](data);
}
