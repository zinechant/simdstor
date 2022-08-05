#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>

#include "arm_neon.h"
#include "arm_sve.h"
#include "gtest/gtest.h"
#include "vsbytes.hh"

#define INFO(fmt, ...) printf("[   INFO   ] " fmt, __VA_ARGS__);

std::mt19937 mt(testing::UnitTest::GetInstance()->random_seed());
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
 protected:
  data_t data;
  meta_t meta;
  inline svint32_t fixvec(int64_t x, int64_t y) {
    svbool_t at = svptrue_b8();
    svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);

    svint32_t v = svvcpz(svcmpeq(at, index, 1), x);
    return svvcpy_m(v, svcmpeq(at, index, 3), y);
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

  svbool_t at = svptrue_b8();
  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t v = svvcpz(svcmpeq(at, index, 1), x);
  v = svvcpy_m(v, svcmpeq(at, index, 2), y);

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

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  v = svvadd_n_s32_m(at, v, z);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vadd_zi2) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = 11 << 8;
  const int64_t a = z, b = x + z, c = z, d = y + z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_n_s32_m(at, v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vadd_zi3) {
  const int64_t x = -528166666 - 12;
  const int64_t y = 1281412838 - 12;
  const int64_t z = 12;
  const int64_t a = z, b = x + z, c = z, d = y + z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  v = svvadd_n_s32_m(at, v, z);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vadd_zzz) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = z, b = x + z, c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmplt(at, index, 2), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_s32_m(at, v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_s32_m(svcmplt(at, index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vcmpeqi) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = 0, b = x, c = z, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t index = svvand_m(at, svvindex(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_s32_m(svvcmpeq(at, index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vcmpnei) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = z, b = x, c = z, d = y + z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t index = svvand_m(at, svvindex(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_s32_m(svvcmpne(at, index, 1), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vcmplti) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = z, b = x + z, c = z, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t index = svvand_m(at, svvindex(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_s32_m(svvcmplt(at, index, 3), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vcmplei) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = z, b = x + z, c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t index = svvand_m(at, svvindex(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvadd_s32_m(svvcmple(at, index, 1), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vsub_zi1) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = 78;
  const int64_t a = -z, b = x - z, c = -z, d = y - z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  v = svvsub_n_s32_m(at, v, z);

  int n = svvstore(v, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vsub_zi2) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = 121 << 8;
  const int64_t a = -z, b = x - z, c = -z, d = y - z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvsub_n_s32_m(at, v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vsub_zzz) {
  const int64_t x = RAND62();
  const int64_t y = RAND62();
  const int64_t z = RAND62();
  const int64_t a = -z, b = x - z, c = 0, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmplt(at, index, 2), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvsub_s32_m(at, v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvsub_s32_m(svcmplt(at, index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmul_zi) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = 101;
  const int64_t a = 0, b = x * z, c = 0, d = y * z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmul_n_s32_m(at, v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmul_zzz) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = RAND32();
  const int64_t a = 0, b = x * z, c = 0, d = y * z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmpne(at, index, 2), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmul_s32_m(at, v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmpne(at, index, 2), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmul_s32_m(at, v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmul_s32_m(svcmpeq(at, index, 3), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vdiv_zpmz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND20();
  const int64_t a = 0, b = x / z, c = 0, d = y / z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvdiv_m(svcmpne(at, index, 0), v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmax_s32_m(svcmpne(at, index, 0), v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmin_s32_m(svcmpne(at, index, 0), v, u);

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

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmax_n_s32_m(at, v, z);

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

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvmin_n_s32_m(at, v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vand_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 0xf1f1f1f1f1f1f1f1L;
  const int64_t a = 0, b = x & z, c = 0, d = y & z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvand_n_m(at, v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vand_zin) {
  const int64_t x = -750286509;
  const int64_t y = -1918015668;
  const int64_t z = 0x00000000003fff80L;
  const int64_t a = 0, b = x & z, c = 0, d = y & z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvand_n_m(at, v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vand_zzz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = 0, b = x & z, c = 0, d = y & z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmple(at, index, 3), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvand_m(at, v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvand_m(svcmplt(at, index, 10), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vorr_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 0xfc0ffc0ffc0ffc0fL;
  const int64_t a = z, b = x | z, c = z, d = y | z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvorr_n_m(at, v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vorr_zzz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = z, b = x | z, c = z, d = y;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmplt(at, index, 3), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvorr_m(at, v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvorr_m(svcmpeq(at, index, 0), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, veor_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 0xc0ffc0ffc0ffc0ffL;
  const int64_t a = z, b = x ^ z, c = z, d = y ^ z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svveor_n_m(at, v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, veor_zzz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = z, b = x ^ z, c = z, d = y ^ z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmpne(at, index, 125), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svveor_m(at, v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svveor_m(svcmpeq(at, index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vbic_zi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 0x807f807f807f807fL;
  const int64_t a = 0, b = x & (~z), c = 0, d = y & (~z);
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvbic_n_m(at, v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vbic_zzz) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = RAND64();
  const int64_t a = 0, b = x, c = 0, d = y & (~z);
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvcpz(svcmpne(at, index, 1), z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvbic_m(at, v, u);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvbic_m(svcmple(at, index, 2), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vasr_zpi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 18;
  const int64_t a = 0, b = x, c = 0, d = y >> z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t v = fixvec(x, y);
  svint32_t w = svvasr_n_s32_m(svcmpne(at, index, 1), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vasr_zzi) {
  const int64_t x = RAND64();
  const int64_t y = RAND64();
  const int64_t z = 21;
  const int64_t a = 0, b = x >> z, c = 0, d = y >> z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvasr_n_s32_m(at, v, z);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvasr_s32_m(svcmpne(at, index, 0), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vasl_zpi) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = 18;
  const int64_t a = 0, b = x, c = 0, d = y << z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t v = fixvec(x, y);
  svint32_t w = svvasl_n_s32_m(svcmpne(at, index, 1), v, z);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vasl_zzi) {
  const int64_t x = RAND32();
  const int64_t y = RAND32();
  const int64_t z = 21;
  const int64_t a = 0, b = x << z, c = 0, d = y << z;
  INFO("x = %ld, y = %ld, z = %ld, %ld, %ld, %ld, %ld\n", x, y, z, a, b, c, d);

  svbool_t at = svptrue_b8();

  svint32_t v = fixvec(x, y);
  svint32_t w = svvasl_n_s32_m(at, v, z);

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

  svbool_t at = svptrue_b8();

  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);
  svint32_t u = svvdup(z);

  svint32_t v = fixvec(x, y);
  svint32_t w = svvasl_s32_m(svcmpne(at, index, 0), v, u);

  int n = svvstore(w, (int8_t*)data.data(), (int8_t*)meta.data());
  expect(n, -1, a, b, c, d);
}

TEST_F(VarSIMDTest, vmaxv_rv) {
  svbool_t at = svptrue_b8();
  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);
  int64_t w = svvmaxv(svcmple(at, index, 2), v);

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
  svbool_t at = svptrue_b8();
  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);
  int64_t w = svvminv(svcmpne(at, index, 1), v);

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
  svbool_t at = svptrue_b8();
  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);
  int64_t w = svvaddv(svcmpne(at, index, 0), v);

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
  svbool_t at = svptrue_b8();
  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);

  svint32_t w = svvabs_m(v, svcmple(at, index, 3), v);
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
  svbool_t at = svptrue_b8();
  svuint8_t index = svand_m(at, svindex_u8(0, 1), 3);

  svint32_t v = randvec();
  int n = BHSD_RH(meta.data());
  EXPECT_LE(VB / 8, n);
  int32_t w = svvnum(svcmpne(at, index, 2), v);
  int32_t m = n - (n / 4) * 4;
  int32_t u = (n / 4) * 3 + (m > 2 ? m - 1 : m);
  EXPECT_EQ(u, w);
}
