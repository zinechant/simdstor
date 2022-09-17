#include <arm_sve.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "debug.h"
#include "salloc.hh"

const std::string tdir = std::string(getenv("HOME")) + "/Downloads/tpchv3_s1e0";

void q2(bool varsimd) {
  const int N = 200000;
  int8_t* p_size_i = filedata((tdir + "/part/p_size.bin").c_str());
  int8_t* p_partkey_i = filedata((tdir + "/part/p_partkey.bin").c_str());
  int8_t* p_mfgr_i = filedata((tdir + "/part/p_mfgr.bin").c_str());

  int8_t* p_partkey_o = sballoc(N << 2);
  int8_t* p_mfgr_o = sballoc(N);

  iprintf("p0=%p, p1=%p, p2=%p, p3=%p, p4=%p\n", p_size_i, p_partkey_i,
          p_mfgr_i, p_partkey_o, p_mfgr_o); fflush(stderr);

  unsigned p_size_si = frstream(p_size_i, 32, N << 8);
  unsigned p_partkey_si = frstream(p_partkey_i, 32, N << 8);
  unsigned p_mfgr_si = frstream(p_mfgr_i, 8, N << 8);

  unsigned p_partkey_so = fwstream(p_partkey_o, 32);
  unsigned p_mfgr_so = fwstream(p_mfgr_o, 8);

  unsigned elems = 0;

  if (varsimd) {
    for (int i = 0, n = 0; i < N; i += n) {
      svint32_t v_p_size = svvunpack(svptrue_b8(), p_size_si);
      svbool_t pg = svvcmpeq(svptrue_b8(), v_p_size, 15);

      svint32_t v_p_partkey = svvunpack(pg, p_partkey_si);
      svint32_t v_p_mfgr = svvunpack(pg, p_mfgr_si);

      unsigned e1 = svvpack(pg, v_p_partkey, p_partkey_so);
      unsigned e2 = svvpack(pg, v_p_mfgr, p_mfgr_so);

      n = svvrcnum();
      assert(e1 == e2 && e1 <= (unsigned)n);
      elems += e1;
    }
  } else {
    for (int i = 0, n = svcntw(); i < N; i += n) {
      svbool_t pg = svwhilele_b32_s32(i, n);
      svint32_t v_p_size = svunpack_s32(p_size_si);
      svint32_t v_p_partkey = svunpack_s32(p_partkey_si);
      svint32_t v_p_mfgr = svld1_gather_u32offset_s32(pg, (const int*)p_mfgr_i,
                                                      svindex_u32(0, 1));
      v_p_mfgr = svand_m(pg, v_p_mfgr, 255);
      pg = svcmpeq(pg, v_p_size, 15);

      unsigned e1 = svpack_s32(pg, v_p_partkey, p_partkey_so);
      unsigned e2 = svpack_s32(pg, v_p_mfgr, p_mfgr_so);
      assert(e1 == e2);
      elems += e1;
      p_mfgr_i += n;
    }
  }

  iprintf("Total #elems: %u\n", elems);

  unsigned bitsleft =
      erstream(p_size_si) + erstream(p_partkey_si) + erstream(p_mfgr_si);
  assert(!bitsleft);

  unsigned b1 = ewstream(p_partkey_so);
  unsigned b2 = ewstream(p_mfgr_si);
  assert(b1 == (b2 << 3));
  filewrite("./p_partkey.bin", b1, p_partkey_o);
  filewrite("./p_mfgr.bin", b2, p_mfgr_o);
}

int main(int argc, char* argv[]) {
  if (argc < 2 || argv[1][0] != 'v') {
    q2(false);
  } else {
    q2(true);
  }
}
