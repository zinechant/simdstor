#include <cassert>
#include <cstdio>
#include <fstream>

#include "aes.hh"
#include "debug.h"

int main(int argc, char* argv[]) {
  using aes::W;
  using aes::W2Power;

  assert(argc == 9);
  aes::Init(argv[1][0] - '0');

  FILE* f = fopen(argv[2], "r");
  fseek(f, 0, SEEK_END);
  unsigned int fs = ftell(f);

  unsigned int ps = aes::GetPaddingLength(fs);

  uint8_t* streambuffer = nullptr;
  uint8_t* scratchpad = nullptr;

  if (argv[7][0] == '0') {
    scratchpad = new uint8_t[(W2Power + 2) << W];
  } else {
    scratchpad = reinterpret_cast<uint8_t*>(std::atol(argv[7]));
  }

  if (argv[8][0] == '0') {
    streambuffer = new uint8_t[ps << 1];
  } else {
    streambuffer = reinterpret_cast<uint8_t*>(std::atol(argv[8]));
  }

  unsigned char* data = streambuffer;
  unsigned char* encr = streambuffer + ps;
  uint8_t(*GF_MUL)[W2Power] = reinterpret_cast<uint8_t(*)[W2Power]>(scratchpad);
  uint8_t* sbox = scratchpad + (W2Power << W);
  uint8_t* ibox = sbox + W2Power;

  {
    std::ifstream is(argv[2], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(data), fs);
  }
  {
    std::ifstream is(argv[3], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(GF_MUL), W2Power << W);
  }
  {
    std::ifstream is(argv[4], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(sbox), W2Power);
  }
  {
    std::ifstream is(argv[5], std::ifstream::binary);
    is.read(reinterpret_cast<char*>(ibox), W2Power);
  }

  unsigned char key[64];

  unsigned int es = aes::EncryptCTR(GF_MUL, sbox, data, fs, key, encr);

  if (argv[6][0] == 't') {
    unsigned char* decr = new unsigned char[ps];

    unsigned int ds = aes::DecryptCTR(GF_MUL, sbox, ibox, encr, ps, key, decr);

    dprintf("ps = %d, es = %d\nfs = %d, ds = %d\n", ps, es, fs, ds);

    for (int i = 0; i < 16; i++) {
      dprintf("0x%02x ", int(decr[fs + i]));
    }
    dprintf("%s", "\n");

    assert(ps == es);
    assert(fs == ds);

    for (unsigned int i = 0; i < fs; i++) {
      assert(data[i] == decr[i]);
    }

    delete[](decr);
  }

  if (argv[7][0] == '0') delete[](scratchpad);
  if (argv[8][0] == '0') delete[](streambuffer);
}
