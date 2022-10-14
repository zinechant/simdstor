#include <cstdio>
#include <cstdlib>

#include "inf.hh"
#include "salloc.hh"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <imageid> <kind>\n", argv[0]);
    return 1;
  }

  int iid = atoi(argv[1]);
  if (iid >= 100) {
    fprintf(stderr, "<imageid> < 100\n");
    return 2;
  }

  int kind = argv[2][0] - '0';
  bool prune = kind > 3;
  kind = kind & 3;
  const uint8_t* image = (const uint8_t*)filedata("./data/images.data");
  const int8_t* labels = filedata("./data/labels.data");
  image += iid * 28 * 28;
  const int32_t* cw = prune ? (const int32_t*)filedata("./data/prune/cw.data")
                            : (const int32_t*)filedata("./data/quant/cw.data");
  const int32_t* cb = prune ? (const int32_t*)filedata("./data/prune/cb.data")
                            : (const int32_t*)filedata("./data/quant/cb.data");
  const int32_t* dw = prune ? (const int32_t*)filedata("./data/prune/dw.data")
                            : (const int32_t*)filedata("./data/quant/dw.data");
  const int32_t* db = prune ? (const int32_t*)filedata("./data/prune/db.data")
                            : (const int32_t*)filedata("./data/quant/db.data");
  const int ishift = 2;
  const int idiff = 10;

  int ans = inf(kind, image, cw, cb, dw, db, ishift, idiff);
  printf("ans = %d, label = %d\n", ans, labels[iid]);
  return 0;
}
