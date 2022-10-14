#ifndef INF_HH_
#define INF_HH_

#include <cstdint>

int inf(int kind, const uint8_t* image, const int32_t* cw, const int32_t* cb,
        const int32_t* dw, const int32_t* db, int ishift, int idiff);

#endif  // INF_HH_
