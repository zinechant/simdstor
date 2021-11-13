#include <assert.h>
#include <malloc.h>
#include <riscv_vector.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "lz.h"

const int LLSIMD_WIDTH = 4;
const int CINV = 0xFFFF;

void compress(meta_t* meta, const char* data, uint8_t* ctrl, char* comp) {
  uint32_t* dictionary = (uint32_t*)malloc(sizeof(uint32_t) << HASH_BITS);
  memset(dictionary, 0, sizeof(uint32_t) << HASH_BITS);
  const unsigned int vbytes = vsetvlmax_e8m1();
  const unsigned int vmbytes = vbytes >> 3;
  uint16_t* fs = (uint16_t*)calloc(vbytes << LLSIMD_WIDTH, sizeof(uint16_t));
  uint8_t* ms = (uint8_t*)calloc(vbytes << (LLSIMD_WIDTH - 3), sizeof(uint8_t));

  const int REPLACE_THRESHOLD = vbytes << CTRL_BITS;

  uint8_t* const ctrl_start = ctrl;

  const char* const data_start = data;
  const char* const comp_start = comp;

  unsigned int ctrls = 0;

  for (int bytes = meta->bytes - SIMD_BACKOFF; bytes > 0;) {
    unsigned int pos = data - data_start;
    unsigned int vl = vsetvl_e8m1(bytes);
    if (pos && ((pos & 0xfff) == 0)) {
      iprintf("%d\t%d\t%ld\t%.2f%%\n", pos, ctrls, comp - comp_start,
              1e2 * (comp - comp_start + ((ctrls + 1) >> 1)) / pos);
    }

    dprintf("bytes =%4d, ctrls =%4d, comps = %4d, pos =%4u, vl =%4u, ctrl: ",
        bytes, ctrls, (int)(comp - comp_start), pos, vl);
    for (const uint8_t* p = ctrl_start; p <= ctrl; p++) {
      dprintf("%02x", *p);
    }
    dprintf("%s", "\ncomp: ");
    for (const char* p = comp_start; p < comp; p++) {
      dprintf("%02x", (uint8_t)*p);
    }
    dprintf("%s", "\n");

    vuint32m4_t vx = vlse32_v_u32m4((const uint32_t*)data, 1, vl);
    vuint32m4_t vh = vmul_vx_u32m4(vx, PRIME3, vl);
    vh = vadd_vx_u32m4(vh, PRIME5, vl);
    vh = vor_vv_u32m4(vsll_vx_u32m4(vh, 17, vl), vsrl_vx_u32m4(vh, 15, vl), vl);
    vh = vmul_vx_u32m4(vh, PRIME4, vl);
    vh = vxor_vv_u32m4(vh, vsrl_vx_u32m4(vh, 15, vl), vl);
    vh = vmul_vx_u32m4(vh, PRIME2, vl);
    vh = vxor_vv_u32m4(vh, vsrl_vx_u32m4(vh, 13, vl), vl);
    vh = vmul_vx_u32m4(vh, PRIME3, vl);
    vh = vxor_vv_u32m4(vh, vsrl_vx_u32m4(vh, 16, vl), vl);
    vh = vand_vx_u32m4(vh, HASH_MASK, vl);
    vh = vsrl_vx_u32m4(vh, 2, vl);

    vuint32m4_t vk = vluxei32_v_u32m4(dictionary, vh, vl);
    vuint32m4_t vj = vadd_vx_u32m4(vid_v_u32m4(vl), pos, vl);
    vuint32m4_t vi = vsub_vv_u32m4(vj, vk, vl);

    vbool8_t pvalid = vmsltu_vx_u32m4_b8(vi, 1 << HISTORY_OFFSET_BITS, vl);
    vbool8_t ivalid = vmsltu_vx_u32m4_b8(vi, 1 << CTRL_BITS, vl);

    vuint32m4_t vy = vluxei32_v_u32m4_m(pvalid, vundefined_u32m4(),
                                        (const uint32_t*)data_start, vk, vl);
    vk = vadd_vx_u32m4(vk, sizeof(uint32_t), vl);
    vbool8_t uvalid = vmseq_vv_u32m4_b8(vy, vx, vl);
    vbool8_t vvalid = vmsgeu_vx_u32m4_b8(vi, 4, vl);

    vuint8m1_t vp = vnsrl_wx_u8m1(vnsrl_wx_u16m2(vx, 24, vl), 0, vl);

    pvalid = vmand_mm_b8(pvalid, uvalid, vl);
    pvalid = vmand_mm_b8(pvalid, vvalid, vl);
    vuint8m1_t vm =
        vmerge_vxm_u8m1(pvalid, vmv_v_x_u8m1(0, vl), sizeof(uint32_t), vl);
    const char* data_last = data + vl + sizeof(uint32_t) - 1;

    vbool8_t alive = pvalid;
    vbool8_t hvalid =
        vmnand_mm_b8(pvalid, vmsltu_vx_u32m4_b8(vi, REPLACE_THRESHOLD, vl), vl);
    for (uint32_t i = vpopc_m_b8(alive, vl), j = 4; i && (j + 1 < CTRL_MAX);
         i = vpopc_m_b8(alive, vl), j++) {
      vp = vslide1down_vx_u8m1(vp, *data_last++, vl);
      vuint8m1_t vq = vluxei32_v_u8m1_m(alive, vundefined_u8m1(),
                                        (uint8_t*)data_start, vk, vl);
      alive = vmand_mm_b8(alive, vmseq_vv_u8m1_b8(vp, vq, vl), vl);
      vm = vadd_vx_u8m1_m(alive, vm, vm, 1, vl);
      vk = vadd_vx_u32m4(vk, 1, vl);
    }
    vuint8m1_t vii = vnsrl_wx_u8m1(vnsrl_wx_u16m2(vi, 0, vl), 0, vl);
    vm = vminu_vv_u8m1_m(ivalid, vm, vm, vii, vl);

    dprintf_vu8m1("vm", vm, vl);

    vuint16m2_t f = vmerge_vvm_u16m2(pvalid, vmv_v_x_u16m2(1, vl),
                                     vwaddu_vx_u16m2(vm, 0, vl), vl);
    f = vadd_vv_u16m2(f, vid_v_u16m2(vl), vl);
    vbool8_t m = vmset_m_b8(vl);
    int k = 0;

    for (; !vfirst_m_b8(m, vl); k++, fs += vbytes, ms += vmbytes) {
      // dprintf_vu16m2("f", f, vl);
      // dprintf_vb8("m", m, vl);
      vse16_v_u16m2(fs, f, vl);
      vse1_v_b8(ms, m, vl);
      m = vmand_mm_b8(m, vmsltu_vx_u16m2_b8(f, vl, vl), vl);
      f = vrgather_vv_u16m2_m(m, vmv_v_x_u16m2(CINV, vl), f, f, vl);
      m = vmsltu_vx_u16m2_b8(f, CINV, vl);
    }

    vbool8_t mask = vmseq_vx_u16m2_b8(vid_v_u16m2(vl), 0, vl);
    uint8_t* mm = (uint8_t*)fs;
    vse8_v_u8m1(mm, vmerge_vxm_u8m1(mask, vmv_v_x_u8m1(0, vl), 1, vl), vl);

    for (; k; k--) {
      ms -= vmbytes;
      fs -= vbytes;

      f = vle16_v_u16m2(fs, vl);
      m = vle1_v_b8(ms, vl);

      vsuxei16_v_u8m1_m(vmand_mm_b8(m, mask, vl), mm, f, vmv_v_x_u8m1(1, vl),
                        vl);
      mask = vmseq_vx_u8m1_b8(vle8_v_u8m1(mm, vl), 1, vl);
      // dprintf_vb8("mask", mask, vl);
    }

    dprintf_vu16m2("fend", f, vl);

    vbool8_t mvalid = vmand_mm_b8(mask, pvalid, vl);
    dprintf_vb8("mvalid", mvalid, vl);

    int mbytes = (vpopc_m_b8(mvalid, vl) << 1);

    if (mbytes == 0) {
      int vc = (vl >> 4) + (vl >> 8);
      if (vc == 0) break;

      int l = (vc << 4) - vc;
      if (ctrls & 1) {
        *ctrl++ += 240;
        ctrls += 1;
        vc -= 1;
      }

      ctrls += vc;
      vse8_v_u8m1((uint8_t*)ctrl, vmv_v_x_u8m1(255, vl), vc >> 1);
      if (vc & 1) {
        *ctrl = 15;
      }

      vse8_v_u8m1((uint8_t*)comp,
                  vnsrl_wx_u8m1(vnsrl_wx_u16m2(vx, 0, vl), 0, vl), l);

      comp += l;
      bytes -= l;
      data += l;

      vsuxei32_v_u32m4_m(hvalid, dictionary, vh, vj, l);

      continue;
    }

    vbool8_t lvalid = vmxor_mm_b8(mask, mvalid, vl);
    int vlast = vmv_x_s_u16m1_u16(vredmaxu_vs_u16m2_u16m1(
        vmv_v_x_u16m1(0, vl),
        vmerge_vvm_u16m2(mvalid, vmv_v_x_u16m2(0, vl), vid_v_u16m2(vl), vl),
        vmv_v_x_u16m1(0, vl), vl));
    int dbytes = fs[vlast];
    lvalid =
        vmand_mm_b8(vmsltu_vx_u16m2_b8(vid_v_u16m2(vl), vlast, vl), lvalid, vl);
    dprintf_vb8("lvalid", lvalid, vl);
    dprintf("vlast = %d, dbytes = %d\n", vlast, dbytes);

    bytes -= dbytes;
    data += dbytes;
    vsuxei32_v_u32m4_m(hvalid, dictionary, vh, vj, vl < dbytes ? vl : dbytes);

    int lbytes = vpopc_m_b8(lvalid, vl);
    uint32_t comp_bytes = lbytes + mbytes;

    vuint16m2_t va = viota_m_u16m2(mvalid, vl);
    vuint16m2_t vb = viota_m_u16m2(lvalid, vl);
    vuint16m2_t va2 = vsll_vx_u16m2(va, 1, vl);

    vp = vmerge_vxm_u8m1(lvalid, vmv_v_x_u8m1(0, vl), 1, vl);
    vuint8m1_t vq = vslide1up_vx_u8m1(vp, 0, vl);
    vbool8_t lvalid_u1 = vmseq_vx_u8m1_b8(vq, 1, vl);
    vq = vslide1down_vx_u8m1(vp, 0, vl);
    vbool8_t lvalid_d1 = vmseq_vx_u8m1_b8(vq, 1, vl);

    vbool8_t lb = vmand_mm_b8(lvalid, vmnot_m_b8(lvalid_u1, vl), vl);
    vbool8_t le = vmand_mm_b8(lvalid, vmnot_m_b8(lvalid_d1, vl), vl);
    dprintf_vb8("lb", lb, vbytes);
    dprintf_vb8("le", le, vbytes);

    vsuxei16_v_u16m2_m(lb, fs, va2, vb, vl);

    vuint16m2_t vu =
        vluxei16_v_u16m2_m(lvalid, vmv_v_x_u16m2(0, vl), fs, va2, vl);
    vu = vsub_vv_u16m2_m(lvalid, vu, vb, vu, vl);

    vu = vadd_vv_u16m2(vsrl_vx_u16m2(vu, 4, vl), vand_vx_u16m2(vu, 15, vl), vl);
    vu = vadd_vv_u16m2(vsrl_vx_u16m2(vu, 4, vl), vand_vx_u16m2(vu, 15, vl), vl);
    vu = vadd_vv_u16m2(vsrl_vx_u16m2(vu, 4, vl), vand_vx_u16m2(vu, 15, vl), vl);
    vu = vadd_vv_u16m2(vsrl_vx_u16m2(vu, 4, vl), vand_vx_u16m2(vu, 15, vl), vl);
    vbool8_t lc = vmor_mm_b8(vmseq_vx_u16m2_b8(vu, 0xf, vl), lb, vl);
    vbool8_t ld = vmand_mm_b8(vmseq_vx_u16m2_b8(vu, 0xe, vl), le, vl);
    dprintf_vb8("lc", lc, vl);
    dprintf_vb8("ld", ld, vl);

    vsuxei16_v_u16m2_m(le, fs, va2, vid_v_u16m2(vl), vl);
    vu = vluxei16_v_u16m2_m(lc, vundefined_u16m2(), fs, va2, vl);
    vuint16m2_t vc =
        vsub_vv_u16m2_m(lc, vundefined_u16m2(), vu, vid_v_u16m2(vl), vl);
    vc = vadd_vx_u16m2(vc, 1, vl);
    vc = vminu_vx_u16m2(vc, 15, vl);
    vm = vmerge_vvm_u8m1(lc, vm, vnsrl_wx_u8m1(vc, 0, vl), vl);

    uint8_t* fb = (uint8_t*)fs;
    vuint16m2_t ve =
        vadd_vv_u16m2(vid_v_u16m2(vl), vwaddu_vx_u16m2(vm, 0, vl), vl);
    vsuxei16_v_u8m1_m(mvalid, fb, ve, vmv_v_x_u8m1(1, vl), vl);

    vbool8_t lm = vmseq_vx_u8m1_b8(vle8_v_u8m1(fb, vl), 1, vl);
    lm = vmand_mm_b8(mvalid, lm, vl);
    lm = vmseq_vx_u8m1_b8(
        vslide1down_vx_u8m1(vmerge_vxm_u8m1(lm, vmv_v_x_u8m1(0, vl), 1, vl), 0,
                            vl),
        1, vl);
    vm = vmerge_vxm_u8m1(lm, vm, 0, vl);

    vbool8_t cvalid =
        vmor_mm_b8(vmor_mm_b8(vmor_mm_b8(mvalid, lc, vl), ld, vl), lm, vl);
    dprintf_vu8m1("vm", vm, vl);
    dprintf_vb8("cvalid", cvalid, vl);

    vuint16m2_t vd = vadd_vv_u16m2(vb, va, vl);
    vd = vadd_vv_u16m2(vd, va, vl);

    vsuxei16_v_u8m1_m(lvalid, (uint8_t*)comp, vd,
                      vnsrl_wx_u8m1(vnsrl_wx_u16m2(vx, 0, vl), 0, vl), vl);
    vuint16m2_t vo = vnsrl_wx_u16m2(vi, 0, vl);
    vsuxei16_v_u16m2_m(mvalid, (uint16_t*)comp, vd, vo, vl);

    comp += comp_bytes;

    if (!vfirst_m_b8(mvalid, vl)) {
      if (ctrls & 1) {
        *ctrl &= 15;
        ctrl++;
      } else {
        *ctrl = 0;
      }
      ctrls++;
    }

    uint8_t* cs = (uint8_t*)fs;
    if (ctrls & 1) {
      cs[0] = *ctrl;
    }
    vc = viota_m_u16m2(cvalid, vl);
    vsuxei16_v_u8m1_m(cvalid, cs + (ctrls & 1), vc, vm, vl);
    int nc = vpopc_m_b8(cvalid, vl) + (ctrls & 1);
    cs[nc] = 0;
    dprintf("vfirst_mvalid = %d, nc = %d\n", (int)vfirst_m_b8(mvalid, vl), nc);

    vl = (nc + 1) >> 1;
    vuint16m2_t vct = vle16_v_u16m2((const uint16_t*)cs, vl);
    vuint8m1_t vctrl =
        vadd_vv_u8m1(vnsrl_wx_u8m1(vct, 4, vl), vnsrl_wx_u8m1(vct, 0, vl), vl);
    vse8_v_u8m1(ctrl, vctrl, vl);

    ctrls += nc - (ctrls & 1);
    ctrl = ctrl_start + (ctrls >> 1);
  }

  int dbytes = data - data_start;
  dprintf("dbytes = %d, ctrls =%4d, ctrl: ", dbytes, ctrls);
  for (const uint8_t* p = ctrl_start; p <= ctrl; p++) {
    dprintf("%02x", *p);
  }
  dprintf("%s", "\ncomp: ");
  for (const char* p = comp_start; p < comp; p++) {
    dprintf("%02x", (uint8_t)*p);
  }
  dprintf("%s", "\n");

  const char* data_end = data_start + meta->bytes;
  for (; data < data_end;) {
    *comp++ = *data++;
  }

  meta->comps = comp - comp_start;
  meta->ctrls = ctrls;

  free(dictionary);
  free(fs);
  free(ms);
}
