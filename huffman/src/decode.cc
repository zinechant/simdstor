

#include <cassert>

#include "debug.h"
#include "huffman.hh"

#ifdef RVV

#include <riscv_vector.h>

const int MAX_LOG_SIMD_WIDTH = 13;
const int LLSIMD_WIDTH = 4;
// for the cushion at the end of the encoded data.
static_assert(MAX_DIGITS <= (sizeof(byte_t) << 4));
static_assert(MAX_LOG_SIMD_WIDTH < (sizeof(uint16_t) << 3));

const int BW = 8;
const int BM = BW - 1;
// const int BINV = (1 << BW) - 1;
const int CINV = (1 << MAX_DIGITS) - 1;

int rvv_decode(const codes_t &codes, const bytes_t &encoded,
               const codeword_t dictionary[], const byte_t codebits[],
               codeid_t ans[], codeword_t fs[], byte_t ms[]) {
  int num = 0;
  long long bits = (sizeof(byte_t) << 3) * (encoded.size() - 5) +
                   encoded[encoded.size() - 1];

  int vbytes = vsetvlmax_e8m1();
  int vmbytes = vbytes >> 3;

  for (long long i = 0; i < bits;) {
    dprintf("\ni = %lld, bits = %lld\n", i, bits);
    uint8_t o = i & BM;
    const uint32_t *data = (uint32_t *)(&encoded.front() + (i >> 3));

    int vl = vsetvl_e16m2(bits - i);
    vuint16m2_t vid = vid_v_u16m2(vl);
    vuint16m2_t vi = vadd_vx_u16m2(vid, o, vl);
    vuint32m4_t vx = vluxei16_v_u32m4(data, vsrl_vx_u16m2(vi, 3, vl), vl);
    vi = vand_vx_u16m2(vi, BM, vl);

    vuint16m2_t vz;
    for (uint8_t j = 0; j < BW; j++) {
      vuint16m2_t vy = vnsrl_wx_u16m2(vand_vx_u32m4(vx, CINV, vl), 0, vl);
      vbool8_t mask = vmseq_vx_u16m2_b8(vi, j, vl);
      vz = vmerge_vvm_u16m2(mask, vz, vy, vl);
      vx = vsrl_vx_u32m4(vx, 1, vl);
      dprintf_vu16m2("vz", vz, vl);
    }

    vuint8m1_t vy = vluxei16_v_u8m1(codebits, vz, vl);
    dprintf_vu8m1("vy", vy, vl);
    vbool8_t m = vmsleu_vx_u8m1_b8(vy, MAX_DIGITS, vl);
    vuint16m2_t f =
        vadd_vv_u16m2(vid_v_u16m2(vl), vwaddu_vx_u16m2(vy, 0, vl), vl);
    f = vmerge_vxm_u16m2(vmnot_m_b8(m, vl), f, CINV, vl);

    int k = 0;

    for (; !vfirst_m_b8(m, vl); k++, fs += vbytes, ms += vmbytes) {
      dprintf_vu16m2("f", f, vl);
      dprintf_vb8("m", m, vl);
      vse16_v_u16m2(fs, f, vl);
      vse1_v_b8(ms, m, vl);
      m = vmand_mm_b8(m, vmsltu_vx_u16m2_b8(f, vl, vl), vl);
      f = vrgather_vv_u16m2_m(m, vmv_v_x_u16m2(CINV, vl), f, f, vl);
      m = vmsltu_vx_u16m2_b8(f, CINV, vl);
    }

    vbool8_t mask = vmseq_vx_u16m2_b8(vid_v_u16m2(vl), 0, vl);
    byte_t *mm = reinterpret_cast<byte_t *>(fs);
    vse8_v_u8m1(mm, vmerge_vxm_u8m1(mask, vmv_v_x_u8m1(0, vl), 1, vl), vl);

    for (; k; k--) {
      ms -= vmbytes;
      fs -= vbytes;

      f = vle16_v_u16m2(fs, vl);
      m = vle1_v_b8(ms, vl);

      vsuxei16_v_u8m1_m(vmand_mm_b8(m, mask, vl), mm, f, vmv_v_x_u8m1(1, vl),
                        vl);
      mask = vmseq_vx_u8m1_b8(vle8_v_u8m1(mm, vl), 1, vl);
      dprintf_vb8("ma", mask, vl);
    }

    int numc = vpopc_m_b8(mask, vl);

    vuint16m2_t vo = vsll_vx_u16m2(viota_m_u16m2(mask, vl), 1, vl);
    vuint16m2_t vs = vluxei16_v_u16m2(dictionary, vsll_vx_u16m2(vz, 1, vl), vl);

    dprintf_vu16m2("vs", vs, vl);
    dprintf_vu16m2("vo", vo, vl);

    vsuxei16_v_u16m2_m(mask, ans + num, vo, vs, vl);
    vuint16m1_t v0 = vmv_v_x_u16m1(0, vl);
    vuint16m2_t vd =
        vmerge_vvm_u16m2(mask, vmv_v_x_u16m2(0, vl), vid_v_u16m2(vl), vl);
    dprintf_vu16m2("vd", vd, vl);
    vuint16m1_t vmax = vredmaxu_vs_u16m2_u16m1(v0, vd, v0, vl);

    num += numc;
    int inc = codes[ans[num - 1]].first + vmv_x_s_u16m1_u16(vmax);
    i += inc;

    dprintf("numc = %d, num = %d, inc = %d, i = %lld\n", numc, num, inc, i);
    dprintf_vu16m1("vmax", vmax, vl >> 1);
    dprintf("codes[ans[num - 1]].first = %d\n", codes[ans[num - 1]].first);
    dprintf("vmv_x_s_u16m1_u16(vmax) = %d\n", vmv_x_s_u16m1_u16(vmax));
    dprintf("ans[] = %s", "\t");
    for (int i = 0; i < num; i++) {
      dprintf("%3d", ans[i]);
    }
    dprintf("%s", "\n");
  }

  return num;
}

const codeids_t decode(const bytes_t &encoded, const dictionary_t &dictionary,
                       const codes_t &codes) {
  codeids_t ans(encoded.size() << 3);
  std::vector<byte_t> codebits;

  int vl = vsetvlmax_e8m1();
  std::vector<codeword_t> fs(vl << LLSIMD_WIDTH);
  std::vector<byte_t> ms(vl << (LLSIMD_WIDTH - 3));

  assert(dictionary.size() == (1 << MAX_DIGITS));
  for (int i = 0; i < (1 << MAX_DIGITS); i++) {
    assert(dictionary[i] == (codeword_t)-1 || dictionary[i] < codes.size());
    byte_t w =
        (dictionary[i] == (codeword_t)-1) ? -1 : codes[dictionary[i]].first;
    codebits.push_back(w);
  }
  assert(codebits.size() == (1 << MAX_DIGITS));

  int n = rvv_decode(codes, encoded, &dictionary.front(), &codebits.front(),
                     &ans.front(), &fs.front(), &ms.front());
  ans.resize(n);
  return ans;
}

#else

// for the cushion at the end of the encoded data.
static_assert(MAX_DIGITS <= (sizeof(byte_t) << 4));

const codeids_t decode(const bytes_t& encoded, const dictionary_t& dictionary,
                       const codes_t& codes) {
  codeids_t ans;

  // there is 5 byte cushion at the end of data. See the end of encode impl.
  int k = (sizeof(byte_t) << 3) * (encoded.size() - 5) +
          encoded[encoded.size() - 1];
  codeword_t x = 0;
  codeword_t w = 0;

  const int BYTE_DIGITS = sizeof(byte_t) << 3;
  const int BYTE_DIGITS_MASK = BYTE_DIGITS - 1;

  for (int i = 0, j = 0; j < k;) {
    while (w < MAX_DIGITS) {
      int l = i & BYTE_DIGITS_MASK;
      int u =
          MAX_DIGITS - w < BYTE_DIGITS - l ? MAX_DIGITS - w : BYTE_DIGITS - l;
      x += ((encoded[i >> 3] >> l) & ((1 << u) - 1)) << w;
      w += u;
      i += u;
    }
    unsigned int y = dictionary[x];
    assert(y < codes.size());
    int z = codes[y].first;
    x >>= z;
    w -= z;
    j += z;
    assert(j <= k);

    ans.push_back(y);
  }

  return ans;
}

#endif
