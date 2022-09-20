#include <arm_sve.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "debug.h"
#include "salloc.hh"

const std::string tdir = std::string("./data/");

void q2(int kind) {
  const int N = 200000;
  unsigned elems = 0;
  int8_t* p_size_i = filedata((tdir + "part/p_size.bin").c_str());
  int8_t* p_partkey_i = filedata((tdir + "part/p_partkey.bin").c_str());
  int8_t* p_mfgr_i = filedata((tdir + "part/p_mfgr.bin").c_str());

  int8_t* p_partkey_o = sballoc(N << 2);
  int8_t* p_mfgr_o = sballoc(N);
  iprintf("p0=%p, p1=%p, p2=%p, p3=%p, p4=%p\n", p_size_i, p_partkey_i,
          p_mfgr_i, p_partkey_o, p_mfgr_o);

  if (kind == 0) {
    int8_t* p_partkey_op = p_partkey_o;
    int8_t* p_mfgr_op = p_mfgr_o;
    for (int i = 0, n = svcntw(); i < N; i += n) {
      svbool_t pg = svwhilelt_b32_s32(i, N);
      svint32_t v_p_size = svld1_s32(pg, (const int32_t*)p_size_i);
      svint32_t v_p_partkey = svld1_s32(pg, (const int32_t*)p_partkey_i);
      svint32_t v_p_mfgr = svld1ub_s32(pg, (const uint8_t*)p_mfgr_i);

      pg = svcmpeq(pg, v_p_size, 15);
      unsigned e = svcntp_b32(pg, pg);
      elems += e;
      assert(e <= (unsigned)n);

      svst1_s32(svwhilelt_b32_s32(0, e), (int32_t*)p_partkey_op,
                svcompact_s32(pg, v_p_partkey));
      svst1b_s32(svwhilelt_b32_s32(0, e), p_mfgr_op,
                 svcompact_s32(pg, v_p_mfgr));

      p_size_i += n << 2;
      p_partkey_i += n << 2;
      p_mfgr_i += n;

      p_partkey_op += e << 2;
      p_mfgr_op += e;
    }
  } else {
    unsigned p_size_si = frstream(p_size_i, 32, N << 5);
    unsigned p_partkey_si = frstream(p_partkey_i, 32, N << 5);
    unsigned p_mfgr_si = frstream(p_mfgr_i, 8, N << 3);

    unsigned p_partkey_so = fwstream(p_partkey_o, 32);
    unsigned p_mfgr_so = fwstream(p_mfgr_o, 8);

    iprintf("s0=%x, s1=%x, s2=%x, s3=%x, s4=%x\n", p_size_si, p_partkey_si,
            p_mfgr_si, p_partkey_so, p_mfgr_so);
    fflush(nullptr);

    if (kind == 1) {
      for (int i = 0, n = svcntw(); i < N; i += n) {
        svbool_t pg = svwhilelt_b32_s32(i, N);
        svint32_t v_p_size = svunpack_s32(p_size_si);
        svint32_t v_p_partkey = svunpack_s32(p_partkey_si);
        svint32_t v_p_mfgr = svld1ub_s32(pg, (const uint8_t*)p_mfgr_i);
        pg = svcmpeq(pg, v_p_size, 15);

        unsigned e1 = svpack_s32(pg, v_p_partkey, p_partkey_so);
        unsigned e2 = svpack_s32(pg, v_p_mfgr, p_mfgr_so);
        assert(e1 == e2);
        elems += e1;
        p_mfgr_i += n;
      }
    } else if (kind == 2) {
      for (int i = 0, n = 0; i < N; i += n) {
        svint32_t v_p_size = svvunpack(svptrue_b8(), p_size_si);
        dprintf_vsv("vps", v_p_size);
        svbool_t pg = svvcmpeq(svptrue_b8(), v_p_size, 15);
        dprintf_svbool("pg", pg);

        svint32_t v_p_partkey = svvunpack(pg, p_partkey_si);
        dprintf_vsv("vpk", v_p_partkey);
        svint32_t v_p_mfgr = svvunpack(pg, p_mfgr_si);


        unsigned e1 = svvpack(pg, v_p_partkey, p_partkey_so);
        unsigned e2 = svvpack(pg, v_p_mfgr, p_mfgr_so);

        n = svvrcnum();
        crstream(p_size_si, n);
        crstream(p_partkey_si, n);
        crstream(p_mfgr_si, n);
        assert(e1 == e2 && e1 <= (unsigned)n);
        elems += e1;
        dprintf("elems: %u, e: %u, n: %d\n\n", elems, e1, n);
      }
    } else {
      fprintf(stderr, "%s", "No this kind!\n");
      return;
    }

    unsigned bl0 = erstream(p_size_si);
    unsigned bl1 = erstream(p_partkey_si);
    unsigned bl2 = erstream(p_mfgr_si);
    iprintf("bl0: %u, bl1: %u, bl2: %u\n", bl0, bl1, bl2);

    unsigned b1 = ewstream(p_partkey_so);
    unsigned b2 = ewstream(p_mfgr_so);
    iprintf("b1: %u, b2: %u\n", b1, b2);
    assert(b1 == (elems << 5));
    assert(b2 == (elems << 3));
  }

  iprintf("Total #elems: %u\n", elems);
  std::string p_partkey_path("./p_partkey_0.bin");
  std::string p_mfgr_path("./p_mfgr_0.bin");
  p_partkey_path[p_partkey_path.length() - 5] += kind;
  p_mfgr_path[p_mfgr_path.length() - 5] += kind;
  filewrite(p_partkey_path.c_str(), elems << 5, p_partkey_o);
  filewrite(p_mfgr_path.c_str(), elems << 3, p_mfgr_o);
}

int main(int argc, char* argv[]) {
  if (argc != 2 || argv[1][0] > '2' || argv[1][0] < '0') {
    fprintf(stderr, "Wrong Input! Usage: %s <0/1/2>\n", argv[0]);
    return 1;
  }
  q2(argv[1][0] - '0');

  return 0;
}
