#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "debug.h"
#include "lz.h"

void decompress(const meta_t* meta, char* data, const uint8_t* ctrl,
                const char* comp) {
  bool flip = 0;
  bool mnl = 0;
  int offset = 0;
  const unsigned int ctrls = meta->ctrls;
  const char* comp_end = comp + meta->comps;
  const char* comp_start = comp;
  const char* data_start = data;

#ifndef NDEBUG
  static const int B = 1 << CTRL_BITS;
  unsigned int llen[B];
  unsigned int mlen[B];
  unsigned int clen[B];
  unsigned int contm = 0;
  for (int i = 0; i < B; i++) {
    llen[i] = 0;
    mlen[i] = 0;
    clen[i] = 0;
  }
#endif

  for (int i = 0; i < ctrls; i++, ctrl += flip, flip = !flip) {
    int len = flip ? ((*ctrl) >> CTRL_BITS) : ((*ctrl) & CTRL_MAX);
    if (mnl) {
#ifndef NDEBUG
      mlen[len]++;
      contm++;
#endif
      if (!offset) {
        offset = ((pointer_t*)comp)->b16;
        comp += 2;
      }
      const char* data_offset = data - offset;
      for (int j = 0; j < len; j++) {
        *data++ = *data_offset++;
      }
    } else {
#ifndef NDEBUG
      llen[len]++;
      if (len) {
        contm = contm < B ? contm : B;
        if (contm) {
          clen[contm - 1]++;
        }
        contm = 0;
      }
#endif
      for (int j = 0; j < len; j++) {
        *data++ = *comp++;
      }
    }
    if (len != CTRL_MAX) {
      mnl = !mnl;
      offset = 0;
    }

    int data_bytes = data - data_start;
    int comp_bytes = comp - comp_start;
    dprintf("%d %d\n", comp_bytes, data_bytes);

    if (!(data_bytes & 0xfff)) {
      iprintf("%d %d\n", comp_bytes, data_bytes);
    }
  }
  dprintf("\ncomp_end - comp = %d\n", (int)(comp_end - comp));

  while (comp != comp_end) {
    *data++ = *comp++;
  }

#ifndef NDEBUG
  printf("\n");
  for (int i = 0; i < B; i++) {
    printf("%d\t%8u\t%8u\t%8u\n", i, llen[i], mlen[i], clen[i]);
  }
#endif
}
