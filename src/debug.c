#include "debug.h"

#ifdef RVV
char _arr[MAX_VL_BYTES];

inline void _print_vu8m1(const char *prefix, vuint8m1_t vx, unsigned vl) {
  uint8_t* arr = (uint8_t*)_arr;
  vse8_v_u8m1(arr, vx, vl);
  printf("%s:\t", prefix);
  for (unsigned i = 0; i < vl; i++) {
    printf("%4x ", arr[i]);
  }
  printf("\n");
}

inline void _print_vu16m1(const char *prefix, vuint16m1_t vx, unsigned vl) {
  uint16_t* arr = (uint16_t*)_arr;
  vse16_v_u16m1(arr, vx, vl);
  printf("%s:\t", prefix);
  for (unsigned i = 0; i < vl; i++) {
    printf("%4x ", arr[i]);
  }
  printf("\n");
}

inline void _print_vu16m2(const char *prefix, vuint16m2_t vx, unsigned vl) {
  uint16_t* arr = (uint16_t*)_arr;
  vse16_v_u16m2(arr, vx, vl);
  printf("%s:\t", prefix);
  for (unsigned i = 0; i < vl; i++) {
    printf("%4x ", arr[i]);
  }
  printf("\n");
}

inline void _print_vu32m4(const char *prefix, vuint32m4_t vx, unsigned vl) {
  uint32_t* arr = (uint32_t*)_arr;
  vse32_v_u32m4(arr, vx, vl);
  printf("%s:\t", prefix);
  for (unsigned i = 0; i < vl; i++) {
    printf("%4x ", arr[i]);
  }
  printf("\n");
}

inline void _print_vb8(const char *prefix, vbool8_t vm, unsigned vl) {
  vuint8m1_t vx = vmerge_vxm_u8m1(vm, vmv_v_x_u8m1(0, vl), 1, vl);
  _print_vu8m1(prefix, vx, vl);
}

#endif
