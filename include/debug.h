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

#ifdef RVV
#include <riscv_vector.h>
enum { MAX_VL_BYTES = 32768 };
void _print_vu8m1(const char *prefix, vuint8m1_t vx, unsigned vl);
void _print_vu16m1(const char *prefix, vuint16m1_t vx, unsigned vl);
void _print_vu16m2(const char *prefix, vuint16m2_t vx, unsigned vl);
void _print_vu32m4(const char *prefix, vuint32m4_t vx, unsigned vl);
void _print_vb8(const char *prefix, vbool8_t vm, unsigned vl);

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

#endif  // RVV

#endif  // DEBUG_H_