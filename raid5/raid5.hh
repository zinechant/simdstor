#ifndef RAID4_HH_
#define RAID4_HH_

#include <cstdint>

namespace raid5 {

void Encode(int size, int kind, const int32_t m0[], const int32_t m1[],
            const int32_t m2[], int32_t mp[]);

}  // namespace raid5

#endif  // RAID4_HH_
