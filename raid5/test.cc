#include <gem5/m5ops.h>

#include <cstdio>
#include <cstdlib>

#include "raid5.hh"
#include "salloc.hh"

int main(int argc, char* argv[]) {
  if (argc != 3 || (argv[1][0] != 'e' && argv[1][0] != 'd') ||
      argv[2][0] < '0' || argv[2][0] > '3') {
    fprintf(stderr, "Usage: %s <e/d> <kind>\n", argv[0]);
    return 1;
  }

  bool encode = (argv[1][0] == 'e');
  int kind = argv[2][0] - '0';

  const int32_t* data = (const int32_t*)filedata("./data");
  int bs = filesize("./data") / 12;

  const int32_t* m0 = data;
  const int32_t* m1 = m0 + bs;
  const int32_t* m2 = m1 + bs;

  int32_t* mp = (int32_t*)sballoc(bs << 2);
  int32_t* mo = (int32_t*)sballoc(bs << 2);
  svvrcnum();

  if (encode) m5_reset_stats(0, 0);
  raid5::Encode(bs, kind, m0, m1, m2, mp);
  if (encode)
    m5_dump_stats(0, 0);
  else
    m5_reset_stats(0, 0);
  raid5::Encode(bs, kind, mp, m1, m2, mo);
  if (!encode) m5_dump_stats(0, 0);

  for (int i = 0; i < bs; i++) assert(mo[i] == m0[i]);

  return 0;
}
