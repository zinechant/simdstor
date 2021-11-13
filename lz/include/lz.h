#ifndef LZ_H_
#define LZ_H_

#include <stdint.h>

#include "constants.h"

#define _unused(x) ((void)(x))

// currently we only accept input data up to 1 GiB
typedef struct {
  unsigned int bytes;
  unsigned int ctrls;
  unsigned int comps;
} meta_t;

typedef union pointer {
  uint32_t b32;
  uint16_t b16;
  uint8_t b08;
} pointer_t;

// meta.bytes should be set when called
// meta.ctrls, meta.comps would be set when returned
void compress(meta_t* meta, const char* data, uint8_t* ctrl, char* comp);

// All fields of meta should be set when called
void decompress(const meta_t* meta, char* data, const uint8_t* ctrl,
                const char* comp);

#endif  // LZ_H_
