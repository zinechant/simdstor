#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lz.h"

int xxhash32(int x) {
  int h = PRIME5 + x * PRIME3;
  h = ((h << 17) | (h >> 15));
  h *= PRIME4;
  h ^= h >> 15;
  h *= PRIME2;
  h ^= h >> 13;
  h *= PRIME3;
  h ^= h >> 16;

  return h;
}

static inline int check_additional(const char* q, const char* p,
                                   const char* qend, const char* pend) {
  const char* p_start = p;
  while (p != pend && q != qend && *p == *q) {
    p++;
    q++;
  }
  return p - p_start;
}

uint8_t* append_ctrl(uint8_t* ctrl, int* ctrls, int val) {
  assert(val < (1 << CTRL_BITS));
  uint8_t* new_ctrl;
  if (*ctrls & 1) {
    new_ctrl = ctrl + 1;
    *ctrl += val << CTRL_BITS;
  } else {
    new_ctrl = ctrl;
    *ctrl = val;
  }
  *ctrls += 1;
  return new_ctrl;
}

void compress(meta_t* meta, const char* data, uint8_t* ctrl, char* comp) {
  uint32_t* dictionary =
      (uint32_t*)malloc((HASH_TABLE_BEAN_SIZE * sizeof(uint32_t)) << HASH_BITS);
  uint8_t* heads = (uint8_t*)malloc(sizeof(uint8_t) << HASH_BITS);
  uint8_t* tails = (uint8_t*)malloc(sizeof(uint8_t) << HASH_BITS);
  memset(heads, 0, sizeof(uint8_t) << HASH_BITS);
  memset(tails, 0, sizeof(uint8_t) << HASH_BITS);

  const char* data_end = data + meta->bytes;
  const char* data_finished = data_end - 3;
  const char* data_start = data;
  const char* comp_start = comp;
  int ctrls = 0;
  const char* data_literal = data;
  for (; data < data_finished; data++) {
    int x = ((pointer_t*)data)->b32;
    int h = xxhash32(x) & HASH_MASK;
    int pos = data - data_start;
#ifndef NDEBUG
    if (pos && ((pos & 0xffff) == 0)) {
      printf("%d\t%d\t%d\t%.2f%%\n", pos, ctrls, (int)(comp - comp_start),
             1e2 * (comp - comp_start + ((ctrls + 1) >> 1)) / pos);
    }
#endif

    int match_off = 0;
    int match_len = 0;

    {
      int i = heads[h];
      for (; i != tails[h]; i = (i + 1) & HASH_TABLE_BEAN_MASK) {
        int k = dictionary[h * HASH_TABLE_BEAN_SIZE + i];
        if (!((pos - k) >> HISTORY_OFFSET_BITS)) {
          break;
        }
      }
      heads[h] = i;
    }

    if (heads[h] != tails[h]) {
      int k = dictionary[h * HASH_TABLE_BEAN_SIZE + heads[h]];
      int y = ((pointer_t*)(data_start + k))->b32;
      int pos_aligned = pos - (pos & SIMD_MASK);
      if (pos_aligned - k >= 4 && x == y) {
        int len = 4 + check_additional(data_start + k + 4, data + 4,
                                       data_start + pos_aligned, data_end);
        if (len > match_len) {
          match_len = len;
          match_off = pos - k;
          assert(match_off < (1 << HISTORY_OFFSET_BITS));
        }
      }
    }

    dictionary[h * HASH_TABLE_BEAN_SIZE + tails[h]] = pos;
    tails[h] += 1;
    tails[h] &= HASH_TABLE_BEAN_MASK;
    if (heads[h] == tails[h]) {
      heads[h] += 1;
      heads[h] &= HASH_TABLE_BEAN_MASK;
    }

    if (match_len) {
      int z = CTRL_MAX;
      while (data_literal < data) {
        z = CTRL_MAX;
        z = z < data - data_literal ? z : data - data_literal;
        memcpy(comp, data_literal, z);
        comp += z;
        data_literal += z;
        ctrl = append_ctrl(ctrl, &ctrls, z);
      }
      if (z == CTRL_MAX) {
        ctrl = append_ctrl(ctrl, &ctrls, 0);
      }
      data_literal = data + match_len;
      data = data_literal - 1;

      z = 0;
      while (match_len) {
        *comp++ = match_off & 0xFF;
        *comp++ = match_off >> 8;

        z = CTRL_MAX - 1;
        z = z < match_len ? z : match_len;
        match_len -= z;
        ctrl = append_ctrl(ctrl, &ctrls, z);
        if (match_len) {
          ctrl = append_ctrl(ctrl, &ctrls, 0);
        }
      }
    }
  }

  int extra = data_end - data_literal;
  memcpy(comp, data_literal, extra);
  comp += extra;

  meta->ctrls = ctrls;
  meta->comps = comp - comp_start;

  free(dictionary);
  free(heads);
  free(tails);
}
