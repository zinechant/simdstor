#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG
#define VERBOSE_LEVEL 2
#elif NDEBUG
#define VERBOSE_LEVEL 0
#else
#define VERBOSE_LEVEL 1
#endif

#include <stdio.h>

#define dprintf(fmt, ...)                                     \
  do {                                                        \
    if (VERBOSE_LEVEL > 1) fprintf(stderr, fmt, __VA_ARGS__); \
  } while (0)

#define iprintf(fmt, ...)                                 \
  do {                                                    \
    if (VERBOSE_LEVEL) fprintf(stderr, fmt, __VA_ARGS__); \
  } while (0)

#define dprintflush(fmt, ...)                                 \
  do {                                                        \
    if (VERBOSE_LEVEL > 1) fprintf(stderr, fmt, __VA_ARGS__); \
    if (VERBOSE_LEVEL > 1) fflush(stderr);                    \
  } while (0)

#define iprintflush(fmt, ...)                             \
  do {                                                    \
    if (VERBOSE_LEVEL) fprintf(stderr, fmt, __VA_ARGS__); \
    if (VERBOSE_LEVEL) fflush(stderr);                    \
  } while (0)

#ifdef __riscv_vector
#include <riscv_vector.h>
enum { MAX_VL_BYTES = 32768 };

static char _arr[MAX_VL_BYTES];

inline void _print_vu8m1(const char *prefix, vuint8m1_t vx, unsigned vl) {
  uint8_t *arr = (uint8_t *)_arr;
  vse8_v_u8m1(arr, vx, vl);
  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < vl; i++) {
    fprintf(stderr, "%4x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void _print_vu16m1(const char *prefix, vuint16m1_t vx, unsigned vl) {
  uint16_t *arr = (uint16_t *)_arr;
  vse16_v_u16m1(arr, vx, vl);
  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < vl; i++) {
    fprintf(stderr, "%4x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void _print_vu16m2(const char *prefix, vuint16m2_t vx, unsigned vl) {
  uint16_t *arr = (uint16_t *)_arr;
  vse16_v_u16m2(arr, vx, vl);
  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < vl; i++) {
    fprintf(stderr, "%4x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void _print_vu32m4(const char *prefix, vuint32m4_t vx, unsigned vl) {
  uint32_t *arr = (uint32_t *)_arr;
  vse32_v_u32m4(arr, vx, vl);
  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < vl; i++) {
    fprintf(stderr, "%4x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void _print_vb8(const char *prefix, vbool8_t vm, unsigned vl) {
  vuint8m1_t vx = vmerge_vxm_u8m1(vm, vmv_v_x_u8m1(0, vl), 1, vl);
  _print_vu8m1(prefix, vx, vl);
}

#define dprintf_vu8m1(prefix, vx, vl)                    \
  do {                                                   \
    if (VERBOSE_LEVEL > 1) _print_vu8m1(prefix, vx, vl); \
  } while (0)

#define dprintf_vu16m1(prefix, vx, vl)                    \
  do {                                                    \
    if (VERBOSE_LEVEL > 1) _print_vu16m1(prefix, vx, vl); \
  } while (0)

#define dprintf_vu16m2(prefix, vx, vl)                    \
  do {                                                    \
    if (VERBOSE_LEVEL > 1) _print_vu16m2(prefix, vx, vl); \
  } while (0)

#define dprintf_vu32m4(prefix, vx, vl)                    \
  do {                                                    \
    if (VERBOSE_LEVEL > 1) _print_vu32m4(prefix, vx, vl); \
  } while (0)

#define dprintf_vb8(prefix, vx, vl)                    \
  do {                                                 \
    if (VERBOSE_LEVEL > 1) _print_vb8(prefix, vx, vl); \
  } while (0)

#define iprintf_vu8m1(prefix, vx, vl)                \
  do {                                               \
    if (VERBOSE_LEVEL) _print_vu8m1(prefix, vx, vl); \
  } while (0)

#define iprintf_vu16m1(prefix, vx, vl)                \
  do {                                                \
    if (VERBOSE_LEVEL) _print_vu16m1(prefix, vx, vl); \
  } while (0)

#define iprintf_vu16m2(prefix, vx, vl)                \
  do {                                                \
    if (VERBOSE_LEVEL) _print_vu16m2(prefix, vx, vl); \
  } while (0)

#define iprintf_vu32m4(prefix, vx, vl)                \
  do {                                                \
    if (VERBOSE_LEVEL) _print_vu32m4(prefix, vx, vl); \
  } while (0)

#define iprintf_vb8(prefix, vx, vl)                \
  do {                                             \
    if (VERBOSE_LEVEL) _print_vb8(prefix, vx, vl); \
  } while (0)

#endif  // __riscv_vector

#ifdef __ARM_FEATURE_SVE
#include <arm_sve.h>

#include "vsbytes.hh"
static int8_t __arr[32768];

inline void __print_svs8(const char *prefix, svint8_t v) {
  int8_t *arr = (int8_t *)__arr;
  svst1_s8(svptrue_b8(), arr, v);

  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < svcntb(); i++) {
    fprintf(stderr, "%4x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void __print_svu8(const char *prefix, svuint8_t v) {
  uint8_t *arr = (uint8_t *)__arr;
  svst1_u8(svptrue_b8(), arr, v);

  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < svcntb(); i++) {
    fprintf(stderr, "%4x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void __print_svs16(const char *prefix, svint16_t v) {
  int16_t *arr = (int16_t *)__arr;
  svst1_s16(svptrue_b16(), arr, v);

  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < svcnth(); i++) {
    fprintf(stderr, "%4x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void __print_svu16(const char *prefix, svuint16_t v) {
  uint16_t *arr = (uint16_t *)__arr;
  svst1_u16(svptrue_b16(), arr, v);

  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < svcnth(); i++) {
    fprintf(stderr, "%4x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void __print_svs32(const char *prefix, svint32_t v) {
  int32_t *arr = (int32_t *)__arr;
  svst1_s32(svptrue_b32(), arr, v);

  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < svcntw(); i++) {
    fprintf(stderr, "%5x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void __print_svu32(const char *prefix, svuint32_t v) {
  uint32_t *arr = (uint32_t *)__arr;
  svst1_u32(svptrue_b32(), arr, v);

  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < svcntw(); i++) {
    fprintf(stderr, "%5x ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void __print_svs64(const char *prefix, svint64_t v) {
  int64_t *arr = (int64_t *)__arr;
  svst1_s64(svptrue_b64(), arr, v);

  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < svcntd(); i++) {
    fprintf(stderr, "%9lx ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void __print_svu64(const char *prefix, svuint64_t v) {
  uint64_t *arr = (uint64_t *)__arr;
  svst1_u64(svptrue_b64(), arr, v);

  fprintf(stderr, "%s:\t", prefix);
  for (unsigned i = 0; i < svcntd(); i++) {
    fprintf(stderr, "%9lx ", arr[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

inline void __print_svbool(const char *prefix, svbool_t pg) {
  __print_svu8(prefix, svdup_n_u8_z(pg, 1));
}

inline void __print_vsv(const char *prefix, svint32_t v) {
  int8_t *data = __arr;
  int8_t *meta = __arr + (svcntb() << 1);
  int n = svvstore(v, data, meta);
  int nn = BHSD_RH(meta);
  if (n != nn) {
    fprintf(stderr, "ERROR! error! : n=%d, BHSD_RH(meta)=%d\n", n, nn);
  }
  fprintf(stderr, "%s:\tn=%d\t", prefix, n);

  const uint8_t *pdata = (const uint8_t *)data;
  const uint8_t *pmeta = (const uint8_t *)meta + 2;
  for (int i = 0; i < n; i++) {
    int64_t t = VSUnpackBytes(pdata, pmeta[i]);
    fprintf(stderr, "%lx(%db) ", t, pmeta[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

#define dprintf_svs8(prefix, vx)                     \
  do {                                               \
    if (VERBOSE_LEVEL > 1) __print_svs8(prefix, vx); \
  } while (0)

#define dprintf_svu8(prefix, vx)                     \
  do {                                               \
    if (VERBOSE_LEVEL > 1) __print_svu8(prefix, vx); \
  } while (0)

#define dprintf_svs16(prefix, vx)                     \
  do {                                                \
    if (VERBOSE_LEVEL > 1) __print_svs16(prefix, vx); \
  } while (0)

#define dprintf_svu16(prefix, vx)                     \
  do {                                                \
    if (VERBOSE_LEVEL > 1) __print_svu16(prefix, vx); \
  } while (0)

#define dprintf_svs32(prefix, vx)                     \
  do {                                                \
    if (VERBOSE_LEVEL > 1) __print_svs32(prefix, vx); \
  } while (0)

#define dprintf_svu32(prefix, vx)                     \
  do {                                                \
    if (VERBOSE_LEVEL > 1) __print_svu32(prefix, vx); \
  } while (0)

#define dprintf_svs64(prefix, vx)                     \
  do {                                                \
    if (VERBOSE_LEVEL > 1) __print_svs64(prefix, vx); \
  } while (0)

#define dprintf_svu64(prefix, vx)                     \
  do {                                                \
    if (VERBOSE_LEVEL > 1) __print_svu64(prefix, vx); \
  } while (0)

#define dprintf_svbool(prefix, pg)                     \
  do {                                                 \
    if (VERBOSE_LEVEL > 1) __print_svbool(prefix, pg); \
  } while (0)

#define dprintf_vsv(prefix, vx)                     \
  do {                                              \
    if (VERBOSE_LEVEL > 1) __print_vsv(prefix, vx); \
  } while (0)

#define iprintf_svs8(prefix, vx)                 \
  do {                                           \
    if (VERBOSE_LEVEL) __print_svs8(prefix, vx); \
  } while (0)

#define iprintf_svu8(prefix, vx)                 \
  do {                                           \
    if (VERBOSE_LEVEL) __print_svu8(prefix, vx); \
  } while (0)

#define iprintf_svs16(prefix, vx)                 \
  do {                                            \
    if (VERBOSE_LEVEL) __print_svs16(prefix, vx); \
  } while (0)

#define iprintf_svu16(prefix, vx)                 \
  do {                                            \
    if (VERBOSE_LEVEL) __print_svu16(prefix, vx); \
  } while (0)

#define iprintf_svs32(prefix, vx)                 \
  do {                                            \
    if (VERBOSE_LEVEL) __print_svs32(prefix, vx); \
  } while (0)

#define iprintf_svu32(prefix, vx)                 \
  do {                                            \
    if (VERBOSE_LEVEL) __print_svu32(prefix, vx); \
  } while (0)

#define iprintf_svs64(prefix, vx)                 \
  do {                                            \
    if (VERBOSE_LEVEL) __print_svs64(prefix, vx); \
  } while (0)

#define iprintf_svu64(prefix, vx)                 \
  do {                                            \
    if (VERBOSE_LEVEL) __print_svu64(prefix, vx); \
  } while (0)

#define iprintf_svbool(prefix, pg)                 \
  do {                                             \
    if (VERBOSE_LEVEL) __print_svbool(prefix, pg); \
  } while (0)

#define iprintf_vsv(prefix, vx)                 \
  do {                                          \
    if (VERBOSE_LEVEL) __print_vsv(prefix, vx); \
  } while (0)

#endif  // __ARM_FEATURE_SVE

#endif  // DEBUG_H_
