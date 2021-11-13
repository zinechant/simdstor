#include <cassert>
#include <cstdio>
#include <fstream>

#include "aes.hh"
#include "debug.h"

int main(int argc, char* argv[]) {
  assert(argc == 4);
  aes::Init(argv[2][0] - '0');

  FILE* f = fopen(argv[1], "r");
  fseek(f, 0, SEEK_END);
  unsigned int fs = ftell(f);

  unsigned int ps = aes::GetPaddingLength(fs);
  unsigned char* data = new unsigned char[ps];
  unsigned char* encr = new unsigned char[ps];

  std::ifstream is(argv[1], std::ifstream::binary);
  is.read(reinterpret_cast<char*>(data), fs);

  unsigned char key[64];

  unsigned int es = aes::EncryptCTR(data, fs, key, encr);

  if (argv[3][0] == 't') {
    unsigned char* decr = new unsigned char[ps];

    unsigned int ds = aes::DecryptCTR(encr, ps, key, decr);

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
  delete[](data);
  delete[](encr);
}
