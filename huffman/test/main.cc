#include <cassert>
#include <cstdio>
#include <ctime>

#include "debug.h"
#include "huffman.hh"

int main() {
  const int seed = 3;
  const int num = 32;
  const int len = 1048576;

  printf("seed %d\n", seed);

  printf("codes_gen ...\n");
  const codes_t& codes = codes_gen(num, seed);

  printf("seq_gen ...\n");
  const codeids_t& seq_ref = seq_gen(num, len, seed);

  printf("dic_gen ...\n");
  const dictionary_t& dictionary = dic_gen(codes);

  printf("encode ...\n");
  const bytes_t& encoded = encode(seq_ref, codes);

  printf("decode ...\n");
  const codeids_t& seq = decode(encoded, dictionary, codes);

  printf("seq == seq_ref: %d\n", seq == seq_ref);
  dprintf("\nseq_ref:  %s", "\t");
  for (auto& x : seq_ref) {
    dprintf("%3d", x);
  }
  dprintf("\nseq    :%s", "\t");
  for (auto& x : seq) {
    dprintf("%3d", x);
  }
  dprintf("\n%s", "");
  assert(seq == seq_ref);
}
