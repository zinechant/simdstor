#include <assert.h>
#include <riscv_vector.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lz.h"

#ifdef DEBUG
#include <stdlib.h>

void print_vector(const char* prefix, vuint16m2_t vx, unsigned vl) {
  uint16_t* arr = (uint16_t*)malloc(sizeof(uint16_t) * vl);
  vse16_v_u16m2(arr, vx, vl);
  printf("%s: ", prefix);
  for (int i = 0; i < vl; i++) {
    printf("%04x ", arr[i]);
  }
  printf("\n");
  free(arr);
}
#endif

static inline vuint16m2_t prefix_sum(vuint8m1_t vc, unsigned int vl) {
  vuint16m2_t vs = vmv_v_x_u16m2(0, vl);
  vbool8_t vm = vmsne_vx_u8m1_b8(vc, 0, vl);
  while (vpopc_m_b8(vm, vl)) {
    vs = vadd_vv_u16m2(vs, viota_m_u16m2(vm, vl), vl);
    vc = vsub_vx_u8m1_m(vm, vc, vc, 1, vl);
    vm = vmsne_vx_u8m1_b8(vc, 0, vl);
  }
  return vs;
}

void decompress(const meta_t* meta, char* data, const uint8_t* ctrl,
                const char* comp) {
  unsigned int ctrls = meta->ctrls;
  bool mnl = false;
  bool half_ctrl = false;

  const char* data_end = data + meta->bytes;
  const char* comp_end = comp + meta->comps;
#ifdef DEBUG
  const char* data_start = data;
  for (int i = 0; i < ctrls; i++) {
    int x = *(ctrl + (i >> 1));
    printf("%x", (i & 1) ? (x >> 4) : x & 0xf);
  }
  printf("\n");
#endif

  while (ctrls) {
    unsigned int vl = vsetvl_e8m1(ctrls);

    vuint16m2_t vi = vid_v_u16m2(vl);
    vuint8m1_t ve = vluxei16_v_u8m1(ctrl, vsrl_vx_u16m2(vi, 1, vl), vl);
    vuint8m1_t vc =
        vsll_vx_u8m1(vnsrl_wx_u8m1(vand_vx_u16m2(vi, 1, vl), 0, vl), 2, vl);
    vc = vand_vx_u8m1(vsrl_vv_u8m1(ve, vc, vl), 0xf, vl);
    if (half_ctrl) {
      int x = ctrl[vl >> 1];
      vc = vslide1down_vx_u8m1(vc, (vl & 1) ? (x >> 4) : (x & 0xf), vl);
    }
#ifdef DEBUG
    {
      int i = half_ctrl;
      printf("ctrl_arr :");
      for (int j = 0; j < vl; j++, i++) {
        int x = *(ctrl + (i >> 1));
        printf("%x", (i & 1) ? (x >> 4) : x & 0xf);
      }
      printf("\n");
    }
    print_vector("vc", vwaddu_vx_u16m2(vc, 0, vl), vl);
#endif  // DEBUG

    vbool8_t vf = vmseq_vx_u8m1_b8(vc, 0xf, vl);
    vuint16m2_t vd = viota_m_u16m2(vf, vl);
    vd = vadd_vv_u16m2(vd, vi, vl);
    vd = vand_vx_u16m2(vd, 1, vl);
    vbool8_t vm = vmseq(vd, mnl ? 0 : 1, vl);

    vuint8m1_t vb = vmerge_vxm_u8m1(vm, vc, 2, vl);

    vuint16m2_t vs = prefix_sum(vc, vl);
    vuint16m2_t vt = prefix_sum(vb, vl);
    vuint16m2_t vu = vadd_vv_u16m2(vs, vwaddu_vx_u16m2(vc, 0, vl), vl);
    vuint16m2_t vv = vadd_vv_u16m2(vt, vwaddu_vx_u16m2(vb, 0, vl), vl);

    vuint16m2_t vo =
        vluxei16_v_u16m2_m(vm, vundefined_u16m2(), (uint16_t*)comp, vt, vl);
    vbool8_t vn = vmsltu_vv_u16m2_b8_m(vm, vm, vo, vu, vl);
    int cs = vfirst_m_b8(vn, vl);
    cs = (cs == -1) ? vl : cs;

    vbool8_t va = vmsbf_m_b8(vn, vl);
    vm = vmand_mm_b8(va, vm, vl);
    vn = vmxor_mm_b8(va, vm, vl);
    unsigned int fs = vpopc_m_b8(vmand_mm_b8(vf, va, vl), vl);

#ifdef DEBUG
    print_vector("vm", vmerge_vxm_u16m2(vm, vmv_v_x_u16m2(0, vl), 1, vl), vl);
    print_vector("vn", vmerge_vxm_u16m2(vn, vmv_v_x_u16m2(0, vl), 1, vl), vl);
    print_vector("vc", vwaddu_vx_u16m2(vc, 0, vl), vl);
    print_vector("vb", vwaddu_vx_u16m2(vb, 0, vl), vl);
    print_vector("vs", vs, vl);
    print_vector("vt", vt, vl);
    print_vector("vu", vu, vl);
    print_vector("vv", vv, vl);
    print_vector("vo", vo, vl);
#endif

    unsigned int data_bytes = vmv_x_s_u16m1_u16(vredmaxu_vs_u16m2_u16m1_m(
        va, vundefined_u16m1(), vu, vmv_v_x_u16m1(0, vl), vl));
    unsigned int comp_bytes = vmv_x_s_u16m1_u16(vredmaxu_vs_u16m2_u16m1_m(
        va, vundefined_u16m1(), vv, vmv_v_x_u16m1(0, vl), vl));

    assert(HISTORY_OFFSET_BITS == 16);
    vo = vsub_vv_u16m2(vs, vo, vl);
    const char* data_p = data - (1 << HISTORY_OFFSET_BITS);

    vm = vmsne_vx_u8m1_b8_m(vm, vm, vc, 0, vl);
    while (vpopc_m_b8(vm, vl)) {
      vuint8m1_t vx = vluxei16_v_u8m1_m(vm, vundefined_u8m1(), data_p, vo, vl);
      vsuxei16_v_u8m1_m(vm, data, vs, vx, vl);
      vc = vsub_vx_u8m1_m(vm, vc, vc, 1, vl);
      vm = vmsne_vx_u8m1_b8_m(vm, vm, vc, 0, vl);

      vs = vadd_vx_u16m2_m(vm, vs, vs, 1, vl);
      vo = vadd_vx_u16m2_m(vm, vo, vo, 1, vl);
    }

    vn = vmsne_vx_u8m1_b8_m(vn, vn, vc, 0, vl);
    while (vpopc_m_b8(vn, vl)) {
      vuint8m1_t vx = vluxei16_v_u8m1_m(vn, vundefined_u8m1(), comp, vt, vl);
      vsuxei16_v_u8m1_m(vn, data, vs, vx, vl);
      vc = vsub_vx_u8m1_m(vn, vc, vc, 1, vl);
      vn = vmsne_vx_u8m1_b8_m(vn, vn, vc, 0, vl);

      vs = vadd_vx_u16m2_m(vn, vs, vs, 1, vl);
      vt = vadd_vx_u16m2_m(vn, vt, vt, 1, vl);
    }

    ctrls -= cs;
    ctrl += (half_ctrl + cs) >> 1;
    mnl ^= ((cs - fs) & 1);
    half_ctrl ^= (cs & 1);
    data += data_bytes;
    comp += comp_bytes;
#ifdef DEBUG
    printf(
        "ctrls = %d, cs = %d, fs = %d, half_ctrl = %d, mnl = %d, "
        "data_bytes = %d, comp_bytes = %d\n",
        ctrls, cs, fs, half_ctrl, mnl, data_bytes, comp_bytes);
    printf("data: ");
    for (const char* d = data_start; d != data; d++) {
      printf("%c", *d);
    }
    printf("\n\n");
#endif
  }

  if (comp_end - comp != data_end - data) {
    printf("%ld = comp_end - comp != data_end - data = %ld\n", comp_end - comp,
           data_end - data);
    assert(comp_end - comp == data_end - data);
  }

  while (data < data_end) {
    *data++ = *comp++;
  }
}
