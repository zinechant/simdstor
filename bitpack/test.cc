#include <gem5/m5ops.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "bitpack.hh"
#include "salloc.hh"

int main(int argc, char* argv[]) {
  if (argc != 3 ||
      (argv[1][0] != '5' && (argv[1][0] != '1' || argv[1][1] != '5')) ||
      argv[2][0] < '0' || argv[2][0] > '3') {
    fprintf(stderr, "Usage: %s <5/15> <kind>\n", argv[0]);
    return 1;
  }

  bool b5 = (argv[1][0] == '5');
  int kind = argv[2][0] - '0';

  const void* input = b5 ? filedata("./b5.data") : filedata("./b15.data");
  int n = b5 ? filesize("./b5.data") : (filesize("./b15.data") >> 2);

  uint8_t* output = (uint8_t*)sballoc(n << 2);

  m5_reset_stats(0, 0);
  if (b5) {
    bitpack<int8_t>(kind, (int8_t*)input, n, output, 5);
  } else {
    bitpack<int32_t>(kind, (int32_t*)input, n, output, 15);
  }
  m5_dump_stats(0, 0);

  std::string fo = b5 ? "o5_0.data" : "o1_0.data";
  fo[3] = argv[2][0];
  filewrite(fo.c_str(), b5 ? (5 * n) : (15 * n), (const int8_t*)output);

  return 0;
}
