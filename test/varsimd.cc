#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>

#include "arm_sve.h"
#include "gtest/gtest.h"
#include "salloc.hh"
#include "util.hh"
#include "vsbytes.hh"

#define INFO(fmt, ...)                      \
  printf("[   INFO   ] " fmt, __VA_ARGS__); \
  fflush(stdout);

std::mt19937 mt(testing::UnitTest::GetInstance()->random_seed());
inline uint32_t URAND8() { return mt() & ((1 << 8) - 1); }
inline uint32_t URAND20() { return mt() & ((1 << 20) - 1); }
inline uint32_t URAND32() { return mt(); }
inline uint64_t URAND62() { return (((uint64_t)mt()) << 30) + mt(); }
inline uint64_t URAND64() { return (((uint64_t)mt()) << 32) + mt(); }

inline int32_t RAND20() { return URAND20() - (1 << 19); }
inline int32_t RAND32() { return URAND32() - (1L << 31); }
inline int64_t RAND62() { return URAND62() - (1L << 61); }
inline int64_t RAND64() { return URAND64(); }

const int VB = svcntb();
const int N = 512;

typedef std::array<uint8_t, N << 2> data_t;
typedef std::array<uint8_t, N << 2> meta_t;

class VarSIMDTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() {
    INFO("RANDOM_SEED: %d\n", testing::UnitTest::GetInstance()->random_seed());
  }

 protected:
  const char* kDictFmt = "../huff/test/dict%d.save";

  data_t data;
  meta_t meta;
  inline svint32_t fixvec(int64_t x, int64_t y) {
    svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);

    svint32_t v = svvcpz(svcmpeq(svptrue_b8(), index, 1), x);
    return svvcpy_m(v, svcmpeq(svptrue_b8(), index, 3), y);
  }

  inline svint32_t randvec() {
    uint8_t* pdata = data.data();
    uint16_t bytes = 0;
    int i = 0;
    for (; bytes <= VB; i++) {
      uint64_t x = URAND64();
      uint16_t y = URAND32() % 128;
      bool neg = y > 63;
      uint16_t bits = neg ? y - 64 : y;

      x &= ((1UL << bits) - 1);
      int64_t z = neg ? -x : x;

      meta[i + 2] = VSPackBytes(pdata, z);
      bytes += BITS2BYTES(meta[i + 2]);
    }
    BHSD_WH(meta.data()) = i - 1;
    return svvload((int8_t*)data.data(), (int8_t*)meta.data());
  }

  inline int8_t* randFixWidth(uint8_t width, uint64_t bits) {
    uint32_t bytes = (bits + 7) >> 3;
    int8_t* ans = sballoc(bytes);

    const uint32_t elems = bits / width;
    uint8_t* pdata = (uint8_t*)ans;
    assert(!(bits % width));

    uint8_t pos = 0;
    for (uint32_t i = 0; i < elems; i++) {
      uint64_t x = RAND64() & MASK(width - 1 + (URAND32() & 1));
      PackBits(pdata, pos, width, x);
      AdvanceBits(pdata, pos, width);
    }
    return ans;
  }

  inline std::pair<int8_t*, int8_t*> randVarWidth(uint64_t bits,
                                                  std::vector<uint64_t>* pvec) {
    uint32_t bytes = (bits + 7) >> 3;
    std::pair<int8_t*, int8_t*> ans(sballoc(bytes), nullptr);
    uint8_t* pdata = (uint8_t*)ans.first;
    std::vector<int8_t> meta;

    uint8_t pos = 0;
    for (uint8_t b = 0; bits; bits -= b) {
      b = std::min(URAND32() & 63, (uint32_t)bits);
      uint64_t x =
          (b == 0) ? 0 : ((1UL << (b - 1)) + (URAND64() & MASK(b - 1)));
      EXPECT_EQ(b, x ? 64 - __builtin_clzl(x) : 0);
      PackBits(pdata, pos, b, x);
      AdvanceBits(pdata, pos, b);
      meta.push_back(b);
      if (pvec) pvec->push_back(x);
    }
    if (pos) {
      EXPECT_EQ(ans.first + bytes, (int8_t*)pdata + 1);
    } else {
      EXPECT_EQ(ans.first + bytes, (int8_t*)pdata);
    }

    ans.second = sballoc(meta.size() + 4);
    BHSD_WS(ans.second) = meta.size();
    memcpy(ans.second + 4, meta.data(), meta.size());
    return ans;
  }

  inline std::tuple<int8_t*, int8_t*, int8_t*> randHuff(uint64_t& bits) {
    const int kMaxDSize = 0x5000000;
    static int8_t* enc = sballoc(kMaxDSize);

    int d = URAND32() & 15;
    char dicp[100];
    sprintf(dicp, kDictFmt, d);
    int abytes = filesize(dicp);
    assert(abytes && "Need to run ../huff first which generates huff dicts.");
    assert(abytes <= kMaxDSize);

    FILE* di = fopen(dicp, "rb");
    fread(enc, 1, abytes, di);
    int8_t* dec = enc + BHSD_RS(enc);
    const uint32_t kSymbols = BHSD_RS(dec);
    uint8_t* data = (uint8_t*)sballoc((bits + 7) >> 3);
    auto ans = std::make_tuple((int8_t*)data, enc, dec);
    enc += 8;

    INFO("Huff dict path: %s, sbits = %u, cbits = %u\n", dicp, dec[4], dec[5]);

    uint32_t elems = 0;
    uint64_t i = 0;
    for (uint8_t pos = 0;; elems++) {
      uint32_t x = URAND32() % kSymbols;
      uint32_t e =
          (dec[5] <= 8 ? BHSD_RH(enc + (x << 1)) : BHSD_RS(enc + (x << 2)));
      uint8_t b = e & MASK(8);
      if (i + b > bits) break;
      PackBits(data, pos, b, e >> 8);
      AdvanceBits(data, pos, b);
      i += b;
    }
    bits = i;
    BHSD_WS(dec) = elems;

    return ans;
  }

  inline void expect(int n, int m, int64_t a, int64_t b, int64_t c, int64_t d) {
    EXPECT_LE(VB / 8, n);
    EXPECT_EQ(n, BHSD_RH(meta.data()));
    INFO("n = %d\n", n);

    if (m != -1) {
      EXPECT_LE(VB / 8, m);
      EXPECT_EQ(m, BHSD_RH(meta.data() + VB + 2));
    }

    const uint8_t* pdata = data.data();
    const uint8_t* pmeta = meta.data() + 2;

    for (int i = 0; i < n; i++) {
      int64_t t = VSUnpackBytes(pdata, pmeta[i]);
      switch (i & 3) {
        case 0:
          EXPECT_EQ(a, t) << "  i: " << i << "  m: " << (int)pmeta[i];
          break;
        case 1:
          EXPECT_EQ(b, t) << "  i: " << i << "  m: " << (int)pmeta[i];
          break;
        case 2:
          EXPECT_EQ(c, t) << "  i: " << i << "  m: " << (int)pmeta[i];
          break;
        case 3:
          EXPECT_EQ(d, t) << "  i: " << i << "  m: " << (int)pmeta[i];
          break;
      }
    }
  }
};

TEST_F(VarSIMDTest, vstore) {
  const int64_t x = 0x7FFFFFFFFFFFFFFF;
  const int64_t y = 0x8000000000000001;
  const int64_t a = 0, b = x, c = 0, d = y;
  INFO("x = %ld, y = %ld  %ld, %ld, %ld, %ld\n", x, y, a, b, c, d);

  svint32_t v = fixvec(x, y);
  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vload) {
  data_t dataout;
  meta_t metaout;

  svint32_t v = randvec();
  int n = svvstore(v, (int8_t*)dataout.data(), (int8_t*)metaout.data());
  EXPECT_LE(VB / 8, n);
  INFO("n = %d\n", n);

  int bytes = 0;
  EXPECT_EQ(meta[0], metaout[0]);
  EXPECT_EQ(meta[1], metaout[1]);
  for (int i = 2; i < n + 2; i++) {
    EXPECT_EQ(meta[i], metaout[i]) << "  i = " << i;
    bytes += BITS2BYTES(metaout[i]);
  }
  EXPECT_GE(VB, bytes);

  for (int i = 0; i < bytes; i++) {
    EXPECT_EQ(data[i], dataout[i]) << "  i = " << i;
  }
}

TEST_F(VarSIMDTest, vdup_zr) {
  const int64_t x = RAND64();
  const int bits = (x < 0)   ? (192 - __builtin_clzl(-x))
                   : (x > 0) ? (64 - __builtin_clzl(x))
                             : 0;
  INFO("bits = %d, x = %ld\n", bits, x);

  svint32_t v = svvdup(x);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  EXPECT_LE(VB / 8, n);
  EXPECT_EQ(n, BHSD_RH(meta.data()));

  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;

  for (int i = 0; i < n; i++) {
    EXPECT_EQ(bits, pmeta[i]) << "  i: " << i << "  m: " << (int)pmeta[i];
    int64_t t = VSUnpackBytes(pdata, pmeta[i]);
    EXPECT_EQ(x, t) << "  i: " << i << "  m: " << (int)pmeta[i];
  }
}

TEST_F(VarSIMDTest, vdup_zrn) {
  const int64_t x = -URAND62();
  const int bits = (x < 0)   ? (192 - __builtin_clzl(-x))
                   : (x > 0) ? (64 - __builtin_clzl(x))
                             : 0;
  INFO("bits = %d, x = %ld\n", bits, x);

  svint32_t v = svvdup(x);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  EXPECT_LE(VB / 8, n);
  EXPECT_EQ(n, BHSD_RH(meta.data()));

  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;

  for (int i = 0; i < n; i++) {
    EXPECT_EQ(bits, pmeta[i]) << "  i: " << i << "  m: " << (int)pmeta[i];
    int64_t t = VSUnpackBytes(pdata, pmeta[i]);
    EXPECT_EQ(x, t) << "  i: " << i << "  m: " << (int)pmeta[i];
  }
}

TEST_F(VarSIMDTest, vindex_ii) {
  int32_t x = 15;
  int32_t y = -16;
  svint32_t v = svvindex(x, y);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  EXPECT_LE(VB / 8, n);
  EXPECT_EQ(n, BHSD_RH(meta.data()));
  INFO("n = %d\n", n);

  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;

  for (int i = 0; i < n; i++) {
    int64_t t = VSUnpackBytes(pdata, pmeta[i]);
    EXPECT_EQ(x + y * i, t) << "  i: " << i << "  m: " << (int)pmeta[i];
  }
}

TEST_F(VarSIMDTest, vindex_ir) {
  int32_t x = -11;
  int32_t y = URAND32() & 127;
  svint32_t v = svvindex(x, y);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  EXPECT_LE(VB / 8, n);
  EXPECT_EQ(n, BHSD_RH(meta.data()));
  INFO("n = %d\n", n);

  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;

  for (int i = 0; i < n; i++) {
    int64_t t = VSUnpackBytes(pdata, pmeta[i]);
    EXPECT_EQ(x + y * i, t) << "  i: " << i << "  m: " << (int)pmeta[i];
  }
}

TEST_F(VarSIMDTest, vindex_ri) {
  int32_t x = URAND20();
  int32_t y = 15;
  svint32_t v = svvindex(x, y);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  EXPECT_LE(VB / 8, n);
  EXPECT_EQ(n, BHSD_RH(meta.data()));
  INFO("n = %d\n", n);

  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;

  for (int i = 0; i < n; i++) {
    int64_t t = VSUnpackBytes(pdata, pmeta[i]);
    EXPECT_EQ(x + y * i, t) << "  i: " << i << "  m: " << (int)pmeta[i];
  }
}

TEST_F(VarSIMDTest, vindex_rr) {
  int32_t x = URAND20();
  int32_t y = URAND32() & 127;
  svint32_t v = svvindex(x, y);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  EXPECT_LE(VB / 8, n);
  EXPECT_EQ(n, BHSD_RH(meta.data()));
  INFO("n = %d\n", n);

  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;

  for (int i = 0; i < n; i++) {
    int64_t t = VSUnpackBytes(pdata, pmeta[i]);
    EXPECT_EQ(x + y * i, t) << "  i: " << i << "  m: " << (int)pmeta[i];
  }
}

TEST_F(VarSIMDTest, vcpy_zpi) {
  int64_t x = -127, y = 1016;
  int64_t a = 0, b = x, c = y, d = 0;
  INFO("x = %ld, y = %ld.   %ld, %ld, %ld, %ld\n", x, y, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t v = svvcpz(svcmpeq(svptrue_b8(), index, 1), x);
  v = svvcpy_m(v, svcmpeq(svptrue_b8(), index, 2), y);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vdup_zi) {
  const int64_t x = 17;
  const int bits = (x < 0)   ? (192 - __builtin_clzl(-x))
                   : (x > 0) ? (64 - __builtin_clzl(x))
                             : 0;
  INFO("bits = %d, x = %ld\n", bits, x);

  svint32_t v = svvdup(x);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  EXPECT_LE(VB / 8, n);
  EXPECT_EQ(n, BHSD_RH(meta.data()));

  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;

  for (int i = 0; i < n; i++) {
    EXPECT_EQ(bits, pmeta[i]) << "  i: " << i << "  m: " << (int)pmeta[i];
    int64_t t = VSUnpackBytes(pdata, pmeta[i]);
    EXPECT_EQ(x, t) << "  i: " << i << "  m: " << (int)pmeta[i];
  }
}

TEST_F(VarSIMDTest, vadd_zi1) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = 12;
  const int64_t a = z, b = x + z, c = z, d = y + z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  v = svvadd_n_m(svptrue_b8(), v, z);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vadd_zi2) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = 11 << 8;
  const int64_t a = z, b = x + z, c = z, d = y + z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vadd_zi3) {
  const int64_t x = -528166666 - 12;
  const int64_t y = 1281412838 - 12;
  const int64_t z = 12;
  const int64_t a = z, b = x + z, c = z, d = y + z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  v = svvadd_n_m(svptrue_b8(), v, z);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vadd_zzz) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = z, b = x + z, c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmplt(svptrue_b8(), index, 2), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_m(svptrue_b8(), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);
  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, vadd_zpmz) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = z, b = x + z, c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_m(svcmplt(svptrue_b8(), index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vcmpeqi) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = 0, b = x, c = z, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t index = svvand_m(svptrue_b8(), svvindex(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_m(svvcmpeq(svptrue_b8(), index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vcmpnei) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = z, b = x, c = z, d = y + z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t index = svvand_m(svptrue_b8(), svvindex(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_m(svvcmpne(svptrue_b8(), index, 1), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vcmplti) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = z, b = x + z, c = z, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t index = svvand_m(svptrue_b8(), svvindex(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_m(svvcmplt(svptrue_b8(), index, 3), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vcmplei) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = z, b = x + z, c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t index = svvand_m(svptrue_b8(), svvindex(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_m(svvcmple(svptrue_b8(), index, 1), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vsub_zi1) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = 78;
  const int64_t a = -z, b = x - z, c = -z, d = y - z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  v = svvsub_n_m(svptrue_b8(), v, z);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vsub_zi2) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = 121 << 8;
  const int64_t a = -z, b = x - z, c = -z, d = y - z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvsub_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vsub_zzz) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = -z, b = x - z, c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmplt(svptrue_b8(), index, 2), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvsub_m(svptrue_b8(), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);
  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, vsub_zpmz) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = -z, b = x - z, c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvsub_m(svcmplt(svptrue_b8(), index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmul_zi) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = 101;
  const int64_t a = 0, b = x * z, c = 0, d = y * z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmul_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmul_zzz) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = RAND32();
  const int64_t a = 0, b = x * z, c = 0, d = y * z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmpne(svptrue_b8(), index, 2), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmul_m(svptrue_b8(), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);
  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, vmul_zzz1) {
  const int64_t x = 848819521;
  const int64_t y = -1721077785;
  const int64_t z = -1888817239;
  const int64_t a = 0, b = x * z, c = 0, d = y * z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmpne(svptrue_b8(), index, 2), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmul_m(svptrue_b8(), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);
  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, vmul_zpmz) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = RAND32();
  const int64_t a = 0, b = x, c = 0, d = y * z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmul_m(svcmpeq(svptrue_b8(), index, 3), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vdiv_zpmz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND20();
  const int64_t a = 0, b = x / z, c = 0, d = y / z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvdiv_m(svcmpne(svptrue_b8(), index, 0), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmax_zpmz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = (x + y + RAND64()) / 3;
  const int64_t a = 0, b = std::max(x, z), c = std::max(0L, z),
                d = std::max(y, z);
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmax_m(svcmpne(svptrue_b8(), index, 0), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmin_zpmz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = (x + y + RAND64()) / 3;
  const int64_t a = 0, b = std::min(x, z), c = std::min(0L, z),
                d = std::min(y, z);
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmin_m(svcmpne(svptrue_b8(), index, 0), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmax_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 127;
  const int64_t a = std::max(0L, z), b = std::max(x, z), c = std::max(0L, z),
                d = std::max(y, z);
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmax_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmin_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = -127;
  const int64_t a = std::min(0L, z), b = std::min(x, z), c = std::min(0L, z),
                d = std::min(y, z);
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmin_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vand_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 0xf1f1f1f1f1f1f1f1L;
  const int64_t a = 0, b = x & z, c = 0, d = y & z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvand_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vand_zin) {
  const int64_t x = -750286509;
  const int64_t y = -1918015668;
  const int64_t z = 0x00000000003fff80L;
  const int64_t a = 0, b = x & z, c = 0, d = y & z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvand_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vand_zzz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = 0, b = x & z, c = 0, d = y & z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmple(svptrue_b8(), index, 3), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvand_m(svptrue_b8(), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);

  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, vand_zpmz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = 0, b = x & z, c = 0, d = y & z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvand_m(svcmplt(svptrue_b8(), index, 10), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vorr_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 0xfc0ffc0ffc0ffc0fL;
  const int64_t a = z, b = x | z, c = z, d = y | z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvorr_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vorr_zzz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = z, b = x | z, c = z, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmplt(svptrue_b8(), index, 3), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvorr_m(svptrue_b8(), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);

  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, vorr_zpmz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = z, b = x, c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvorr_m(svcmpeq(svptrue_b8(), index, 0), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, veor_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 0xc0ffc0ffc0ffc0ffL;
  const int64_t a = z, b = x ^ z, c = z, d = y ^ z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svveor_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, veor_zzz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = z, b = x ^ z, c = z, d = y ^ z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmpne(svptrue_b8(), index, 125), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svveor_m(svptrue_b8(), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);

  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, veor_zpmz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = 0, b = x, c = z, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svveor_m(svcmpeq(svptrue_b8(), index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vbic_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 0x807f807f807f807fL;
  const int64_t a = 0, b = x & (~z), c = 0, d = y & (~z);
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvbic_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vbic_zzz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = 0, b = x, c = 0, d = y & (~z);
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmpne(svptrue_b8(), index, 1), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvbic_m(svptrue_b8(), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);

  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, vbic_zpmz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = 0, b = x & (~z), c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvbic_m(svcmple(svptrue_b8(), index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vasr_zpi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 18;
  const int64_t a = 0, b = x, c = 0, d = y >> z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t v = fixvec(x, y);
  svint32_t w = svvasr_n_m(svcmpne(svptrue_b8(), index, 1), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vasr_zzi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 21;
  const int64_t a = 0, b = x >> z, c = 0, d = y >> z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvasr_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);

  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, vasr_zpmz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND32() & 31;
  const int64_t a = 0, b = x >> z, c = 0, d = y >> z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvasr_m(svcmpne(svptrue_b8(), index, 0), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vasl_zpi) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = 18;
  const int64_t a = 0, b = x, c = 0, d = y << z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t v = fixvec(x, y);
  svint32_t w = svvasl_n_m(svcmpne(svptrue_b8(), index, 1), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vasl_zzi) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = 21;
  const int64_t a = 0, b = x << z, c = 0, d = y << z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvasl_n_m(svptrue_b8(), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  int nn =
      svvstore(v, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);

  expect(n, nn, a, b, c, d);
}

TEST_F(VarSIMDTest, vasl_zpmz) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = RAND32() & 31;
  const int64_t a = 0, b = x << z, c = 0, d = y << z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvasl_m(svcmpne(svptrue_b8(), index, 0), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmaxv_rv) {
  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);
  int64_t w = svvmaxv(svcmple(svptrue_b8(), index, 2), v);

  int64_t u = 0x8000000000000001L;
  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;
  for (int i = 0; i < n; i++) {
    int64_t x = VSUnpackBytes(pdata, pmeta[i]);
    if ((i & 3) <= 2) {
      u = x > u ? x : u;
    }
  }
  EXPECT_EQ(u, w);
}

TEST_F(VarSIMDTest, vminv_rv) {
  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);
  int64_t w = svvminv(svcmpne(svptrue_b8(), index, 1), v);

  int64_t u = 0x7FFFFFFFFFFFFFFFL;
  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;
  for (int i = 0; i < n; i++) {
    int64_t x = VSUnpackBytes(pdata, pmeta[i]);
    if ((i & 3) != 1) {
      u = x < u ? x : u;
    }
  }
  EXPECT_EQ(u, w);
}

TEST_F(VarSIMDTest, vaddv_rv) {
  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);
  int64_t w = svvaddv(svcmpne(svptrue_b8(), index, 0), v);

  int64_t u = 0;
  const uint8_t* pdata = data.data();
  const uint8_t* pmeta = meta.data() + 2;
  for (int i = 0; i < n; i++) {
    int64_t x = VSUnpackBytes(pdata, pmeta[i]);
    if (i & 3) {
      u += x;
    }
  }
  EXPECT_EQ(u, w);
}

TEST_F(VarSIMDTest, vabs_zpz) {
  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);

  svint32_t w = svvabs_m(v, svcmple(svptrue_b8(), index, 3), v);
  int nn =
      svvstore(w, (int8_t*)data.data() + VB, (int8_t*)meta.data() + VB + 2);
  EXPECT_EQ(n, nn);

  const uint8_t* pdata1 = data.data();
  const uint8_t* pmeta1 = meta.data() + 2;
  const uint8_t* pdata2 = data.data() + VB;
  const uint8_t* pmeta2 = meta.data() + VB + 4;
  for (int i = 0; i < n; i++) {
    int64_t x = VSUnpackBytes(pdata1, pmeta1[i]);
    int64_t y = VSUnpackBytes(pdata2, pmeta2[i]);
    EXPECT_EQ(std::abs(x), y);
  }
  int nnn = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  EXPECT_EQ(n, nnn);
}

TEST_F(VarSIMDTest, vnum_v) {
  svuint8_t index = svand_m(svptrue_b8(), svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);
  int32_t w = svvnum(svcmpne(svptrue_b8(), index, 2), v);
  int32_t m = n - (n / 4) * 4;
  int32_t u = (n / 4) * 3 + (m > 2 ? m - 1 : m);
  EXPECT_EQ(u, w);
}

TEST_F(VarSIMDTest, fixstream_fixvec) {
  uint8_t width = (URAND8() & 63) + 1;
  uint32_t bits = URAND20() + width;
  uint32_t elems = bits / width;
  bits = elems * width;
  uint32_t bytes = (bits + 7) >> 3;

  INFO("fixstream bits = %u, elems = %u\n", bits, elems);

  int8_t* encoded = randFixWidth(width, bits);
  int8_t* dump = sballoc(bytes);

  INFO("encoded = %p, dump = %p\n", encoded, dump);

  int rid = frstream(encoded, width, bits);
  int wid = fwstream(dump, width);
  INFO("rid = 0x%x, wid = 0x%x\n", rid, wid);

  if (width > 32) {
    for (uint32_t i = 0, n = 0; i < elems; i += n) {
      svbool_t pg = svwhilelt_b64_u32(i, elems);
      svint64_t v = svunpack_s64(rid);
      n = svpack_s64(pg, v, wid);
      EXPECT_EQ(n, i + svcntd() < elems ? svcntd() : elems - i);
    }
  } else if (width > 16) {
    for (uint32_t i = 0, n = 0; i < elems; i += n) {
      svbool_t pg = svwhilelt_b32_u32(i, elems);
      svint32_t v = svunpack_s32(rid);
      n = svpack_s32(pg, v, wid);
      EXPECT_EQ(n, i + svcntw() < elems ? svcntw() : elems - i);
    }
  } else if (width > 8) {
    for (uint32_t i = 0, n = 0; i < elems; i += n) {
      svbool_t pg = svwhilelt_b16_u32(i, elems);
      svint16_t v = svunpack_s16(rid);
      n = svpack_s16(pg, v, wid);
      EXPECT_EQ(n, i + svcnth() < elems ? svcnth() : elems - i);
    }
  } else {
    for (uint32_t i = 0, n = 0; i < elems; i += n) {
      svbool_t pg = svwhilelt_b8_u32(i, elems);
      svint8_t v = svunpack_s8(rid);
      n = svpack_s8(pg, v, wid);
      EXPECT_EQ(n, i + svcntb() < elems ? svcntb() : elems - i);
    }
  }

  uint32_t bitsleft = erstream(rid);
  EXPECT_EQ(0U, bitsleft);
  uint32_t bitsright = ewstream(wid);
  EXPECT_EQ(bits, bitsright);

  for (uint32_t i = 0; i < bytes; i++) {
    EXPECT_EQ(encoded[i], dump[i]) << i;
  }
}

TEST_F(VarSIMDTest, huffstream_fixvec) {
  uint64_t bits = URAND20();
  auto [encoded, enc, dec] = randHuff(bits);
  uint32_t bytes = (bits + 7) >> 3;
  uint32_t elems = BHSD_RS(dec);

  INFO("huffstream bits = %lu, elems = %u\n", bits, elems);
  int8_t* dump = sballoc(bytes);
  INFO("encoded = %p, dump = %p\n", encoded, dump);

  int rid = hrstream(encoded, dec, bits);
  int wid = hwstream(dump, enc);
  INFO("rid = 0x%x, wid = 0x%x, sbits = %d, cbits = %d\n", rid, wid, enc[4],
       enc[5]);

  if (enc[4] > 16) {
    for (uint32_t i = 0, n = 0; i < elems; i += n) {
      svbool_t pg = svwhilelt_b32_u32(i, elems);
      svint32_t v = svunpack_s32(rid);
      n = svpack_s32(pg, v, wid);
      EXPECT_EQ(n, i + svcntw() < elems ? svcntw() : elems - i);
    }
  } else if (enc[4] > 8) {
    for (uint32_t i = 0, n = 0; i < elems; i += n) {
      svbool_t pg = svwhilelt_b16_u32(i, elems);
      svint16_t v = svunpack_s16(rid);
      n = svpack_s16(pg, v, wid);
      EXPECT_EQ(n, i + svcnth() < elems ? svcnth() : elems - i);
    }
  } else {
    for (uint32_t i = 0, n = 0; i < elems; i += n) {
      svbool_t pg = svwhilelt_b8_u32(i, elems);
      svint8_t v = svunpack_s8(rid);
      n = svpack_s8(pg, v, wid);
      EXPECT_EQ(n, i + svcntb() < elems ? svcntb() : elems - i);
    }
  }

  uint32_t bitsleft = erstream(rid);
  EXPECT_EQ(0U, bitsleft);
  uint32_t bitsright = ewstream(wid);
  EXPECT_EQ(bits, bitsright);

  for (uint32_t i = 0; i < bytes; i++) {
    EXPECT_EQ(encoded[i], dump[i]);
  }
}

TEST_F(VarSIMDTest, varstream_fixvec) {
  uint32_t bits = URAND20();
  uint32_t bytes = (bits + 7) >> 3;
  INFO("varstream bits = %u\n", bits);
  std::vector<uint64_t> v;

  auto pair = randVarWidth(bits, &v);
  uint32_t elems = BHSD_RS(pair.second);
  auto qair = std::make_pair(sballoc(bytes), sballoc(elems + 4));

  INFO("pdata = %p, pmeta = %p\n", pair.first, pair.second);
  INFO("ddata = %p, dmeta = %p\n", qair.first, qair.second);
  EXPECT_EQ(elems, v.size());

  int rid = vrstream(pair.first, pair.second, bits);
  int wid = vwstream(qair.first, qair.second);

  INFO("rid = 0x%x, wid = 0x%x\n", rid, wid);

  for (uint32_t i = 0, n = 0; i < elems; i += n) {
    svbool_t pg = svwhilelt_b64_u32(i, elems);
    svint64_t v = svunpack_s64(rid);
    n = svpack_s64(pg, v, wid);
    EXPECT_EQ(n, i + svcntd() < elems ? svcntd() : elems - i);
  }

  uint32_t bitsleft = erstream(rid);
  EXPECT_EQ(0U, bitsleft);
  uint32_t bitsright = ewstream(wid);
  EXPECT_EQ(bits, bitsright);

  uint32_t abits = 0;
  for (uint32_t i = 0; i < elems + 4; i++) {
    int j = i - 4;
    if (j >= 0) {
      abits += pair.second[i];
      EXPECT_EQ(pair.second[i], qair.second[i])
          << "j = " << j << ": " << v[j] << "   abits: " << abits;
    } else {
      EXPECT_EQ(pair.second[i], qair.second[i]) << "i = " << i;
    }
  }
  EXPECT_EQ(bits, abits);

  for (uint32_t i = 0; i < bytes; i++) {
    EXPECT_EQ(pair.first[i], qair.first[i]) << "i = " << i;
  }
}

TEST_F(VarSIMDTest, fixstream_varvec) {
  uint8_t width = (URAND8() & 63) + 1;
  uint32_t bits = URAND20() + width;
  uint32_t elems = bits / width;
  bits = elems * width;
  uint32_t bytes = (bits + 7) >> 3;
  svvrcnum();

  INFO("fixstream bits = %u, elems = %u\n", bits, elems);

  int8_t* encoded = randFixWidth(width, bits);
  int8_t* dump = sballoc(bytes);

  INFO("encoded = %p, dump = %p\n", encoded, dump);

  int rid = frstream(encoded, width, bits);
  int wid = fwstream(dump, width);
  INFO("rid = 0x%x, wid = 0x%x\n", rid, wid);

  for (uint32_t i = 0, n = 0; i < elems; i += n) {
    svint32_t v = svvunpack(svptrue_b8(), rid);
    n = svvpack(svptrue_b8(), v, wid);
    crstream(rid, svvrcnum());
    EXPECT_NE(0U, n);
    EXPECT_GE(elems - i, n);
  }

  uint32_t bitsleft = erstream(rid);
  EXPECT_EQ(0U, bitsleft);
  uint32_t bitsright = ewstream(wid);
  EXPECT_EQ(bits, bitsright);

  for (uint32_t i = 0; i < bytes; i++) {
    EXPECT_EQ(encoded[i], dump[i]) << i;
  }
}

TEST_F(VarSIMDTest, huffstream_varvec) {
  uint64_t bits = URAND20();
  auto [encoded, enc, dec] = randHuff(bits);
  uint32_t bytes = (bits + 7) >> 3;
  uint32_t elems = BHSD_RS(dec);
  svvrcnum();

  INFO("huffstream bits = %lu, elems = %u\n", bits, elems);
  int8_t* dump = sballoc(bytes);
  INFO("encoded = %p, dump = %p\n", encoded, dump);

  int rid = hrstream(encoded, dec, bits);
  int wid = hwstream(dump, enc);
  INFO("rid = 0x%x, wid = 0x%x\n", rid, wid);

  for (uint32_t i = 0, n = 0; i < elems; i += n) {
    svint32_t v = svvunpack(svptrue_b8(), rid);
    n = svvpack(svptrue_b8(), v, wid);
    crstream(rid, svvrcnum());
    EXPECT_NE(0U, n);
    EXPECT_GE(elems - i, n);
  }

  uint32_t bitsleft = erstream(rid);
  EXPECT_EQ(0U, bitsleft);
  uint32_t bitsright = ewstream(wid);
  EXPECT_EQ(bits, bitsright);

  for (uint32_t i = 0; i < bytes; i++) {
    EXPECT_EQ(encoded[i], dump[i]);
  }
}

TEST_F(VarSIMDTest, varstream_varvec) {
  uint32_t bits = URAND20();
  uint32_t bytes = (bits + 7) >> 3;
  INFO("varstream bits = %u\n", bits);
  std::vector<uint64_t> v;
  svvrcnum();

  auto pair = randVarWidth(bits, &v);
  uint32_t elems = BHSD_RS(pair.second);
  auto qair = std::make_pair(sballoc(bytes), sballoc(elems + 4));

  INFO("pdata = %p, pmeta = %p\n", pair.first, pair.second);
  INFO("ddata = %p, dmeta = %p\n", qair.first, qair.second);
  EXPECT_EQ(elems, v.size());

  int rid = vrstream(pair.first, pair.second, bits);
  int wid = vwstream(qair.first, qair.second);

  INFO("rid = 0x%x, wid = 0x%x\n", rid, wid);

  for (uint32_t i = 0, n = 0; i < elems; i += n) {
    svint32_t v = svvunpack(svptrue_b8(), rid);
    n = svvpack(svptrue_b8(), v, wid);
    crstream(rid, svvrcnum());
    EXPECT_NE(0U, n);
    EXPECT_GE(elems - i, n);
  }

  uint32_t bitsleft = erstream(rid);
  EXPECT_EQ(0U, bitsleft);
  uint32_t bitsright = ewstream(wid);
  EXPECT_EQ(bits, bitsright);

  uint32_t abits = 0;
  for (uint32_t i = 0; i < elems + 4; i++) {
    int j = i - 4;
    if (j >= 0) {
      abits += pair.second[i];
      EXPECT_EQ(pair.second[i], qair.second[i])
          << "j = " << j << ": " << v[j] << "   abits: " << abits;
    } else {
      EXPECT_EQ(pair.second[i], qair.second[i]) << "i = " << i;
    }
  }
  EXPECT_EQ(bits, abits);

  for (uint32_t i = 0; i < bytes; i++) {
    EXPECT_EQ(pair.first[i], qair.first[i]) << "i = " << i;
  }
}
