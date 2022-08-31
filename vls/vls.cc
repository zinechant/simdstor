#include <arm_sve.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

const int VB = svcntb();
const int BS = 1 << 11;
const char* VECFP = "./vecs.";

inline svint32_t fixvec(int64_t x, int64_t y) {
  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);

  svint32_t v = svvcpz(svcmpeq(svptrue_b8(), index, 1), x);
  return svvcpy_m(v, svcmpeq(svptrue_b8(), index, 3), y);
}

void vstore_perf(int n, int8_t* pdata, int8_t* pmeta) {
  const int64_t x = 0x7FFFFFFFFFFFFFFF;
  const int64_t y = 0x8000000000000001;
  svint32_t v = fixvec(x, y);
  for (int i = 0; i < n; i++, pdata += VB, pmeta += VB + 2) {
    int nn = svvstore(v, pdata, pmeta);
    assert(VB <= 8 * nn);
  }
}

int main(int argc, char* argv[]) {
  assert(argc == 3);
  int shift = std::atoi(argv[1]);
  const int FS = (VB + VB + 2) << shift;

  int8_t* streambuffer = nullptr;
  if (argv[2][0] == '0') {
    streambuffer = new int8_t[FS];
  } else {
    streambuffer = reinterpret_cast<int8_t*>(std::atol(argv[2]));
  }

  vstore_perf(1 << shift, streambuffer, streambuffer + (VB << shift));

  int ans = 0;
  for (int i = 0, j = 0; i < FS; i += BS, j++) {
    j = j & (BS - 1);
    ans &= streambuffer[i + j];
  }

  if (argv[2][0] == '0') delete[] streambuffer;

  return ans;
}