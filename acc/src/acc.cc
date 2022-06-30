#include "acc.hh"

#include "debug.h"

namespace acc {

long long acc(uint8_t data[], int rbytes, int size) {
  long long ans = 0;

  for (uint8_t* i = data; i - data < size; i += rbytes) {
    ans = ans + *i;
    dprintf("%d %lld\n", *i, ans);
  }

  return ans;
}

}  // namespace acc
