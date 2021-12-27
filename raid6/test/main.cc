#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "raid6.hh"

using raid6::K;
using raid6::M;
using raid6::N;
using raid6::W;
using raid6::W2Power;

int main(int argc, char* argv[]) {
  assert(argc == 7);
  FILE* f = fopen(argv[1], "r");
  fseek(f, 0, SEEK_END);
  uint32_t fs = ftell(f);
  fclose(f);

  uint32_t bs = fs / M + 1;
  fs = bs * N;

  uint8_t* scratchpad = nullptr;
  uint8_t* streambuffer = nullptr;

  if (argv[5][0] == '0') {
    scratchpad = new uint8_t[(W2Power << W) + W2Power];
  } else {
    scratchpad = reinterpret_cast<uint8_t*>(std::atol(argv[5]));
  }

  if (argv[6][0] == '0') {
    streambuffer = new uint8_t[fs];
  } else {
    streambuffer = reinterpret_cast<uint8_t*>(std::atol(argv[6]));
  }

  uint8_t* data = streambuffer;
  uint8_t(*GF_MUL)[W2Power] = reinterpret_cast<uint8_t(*)[W2Power]>(scratchpad);
  uint8_t* GF_INV = scratchpad + (W2Power << W);

  {
    std::ifstream is(argv[1], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(data), fs);
  }
  {
    std::ifstream is(argv[2], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(GF_MUL), (W2Power << W));
  }
  {
    std::ifstream is(argv[3], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(GF_INV), W2Power);
  }

  dprintf("!!! data = %p\n", data);
  for (int i = 0; i < W2Power; i++) {
    for (int j = 0; j < W2Power; j++) {
      dprintf("%d\t", GF_MUL[i][j]);
    }
    dprintf("%s", "\n");
  }
  dprintf("%s", "\n");
  for (int j = 0; j < W2Power; j++) {
    dprintf("%d\t", GF_INV[j]);
  }
  dprintf("%s", "\n");

  uint8_t* message[M];
  uint8_t* parity[K];
  uint8_t* d = data;
  for (int i = 0; i < M; i++) {
    message[i] = d;
    d += bs;
    dprintf("message[%d] = %p\n", i, message[i]);
  }
  for (int i = 0; i < K; i++) {
    parity[i] = d;
    d += bs;
    dprintf("parity[%d] = %p\n", i, parity[i]);
  }

  raid6::Encode(GF_MUL, GF_INV, bs, message, parity);
  if (argv[4][0] == 't') {
    raid6::Decode(GF_MUL, GF_INV, bs, message, parity);
  }

  if (argv[5][0] == '0') delete[](scratchpad);
  if (argv[6][0] == '0') delete[](streambuffer);
}
