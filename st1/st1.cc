#include <arm_sve.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

const int VB = svcntb();
const int BS = 2048;
const char* VECFP = "./vecs.";

#ifdef NDEBUG
#define INFO(fmt, ...)
#else
#define INFO(fmt, ...) printf("[ INFO ] " fmt, __VA_ARGS__); fflush(stdout);
#endif

inline svint64_t fixvec(int64_t x, int64_t y) {
  svuint64_t index = svand_m(svptrue_b64(), svindex_u64(0, 1), 3);

  svint64_t v = svdup_n_s64_z(svcmpeq(svptrue_b64(), index, 1), x);
  return svdup_n_s64_m(v, svcmpeq(svptrue_b64(), index, 3), y);
}

void vstore_perf(int n, int8_t* pdata) {
  const int64_t x = 0x7FFFFFFFFFFFFFFF;
  const int64_t y = 0x8000000000000001;
  svint64_t v = fixvec(x, y);
  for (int i = 0; i < n; i++, pdata += VB) {
    svst1(svptrue_b64(), (int64_t*)pdata, v);
  }
}

int main(int argc, char* argv[]) {
  INFO("%s", "main.\n");

  assert(argc == 3);
  int shift = std::atoi(argv[1]);
  const int FS = VB << shift;

  int8_t* streambuffer = nullptr;
  if (argv[2][0] == '0') {
    streambuffer = (int8_t*)malloc(FS + 512);
  } else {
    streambuffer = reinterpret_cast<int8_t*>(std::atol(argv[2]));
  }

  INFO("streambuffer=%p size=%lu set\n", streambuffer, sizeof(streambuffer));

  vstore_perf(1 << shift, streambuffer);

  INFO("%s", "vstore perf.\n");

  int ans = 0;
  for (int i = 0, j = 0; i < FS; i += BS, j++) {
    j = j & (BS - 1);
    ans &= streambuffer[i + j];
  }

  if (argv[2][0] == '0') free(streambuffer);

  return ans;
}