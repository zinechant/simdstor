#include "inf.hh"

#include <arm_sve.h>
#include <gem5/m5ops.h>

#include <cassert>
#include <cstring>

#include "debug.h"
#include "salloc.hh"

const int M = 28;
const int F = 3;
const int N = M - F + 1;
const int P = N / 2;
const int C = 32;
const int D = P * P * C;
const int A = 10;

int inf(int kind, const uint8_t* image, const int32_t* cw, const int32_t* cb,
        const int32_t* dw, const int32_t* db, int ishift, int idiff) {
  // conv.shape == 26 * 26 * 32 == N * N * C
  // filter.shape == 3 * 3 * 32 == F * F * C
  // pool.shape == 13 * 13 * 32 == P * P * C
  // weight.shape == D * A;

  int32_t* pconv = (int32_t*)sballoc(N * N * C * 4);
  memset(pconv, 0, N * N * C * 4);
  int32_t* ppool = (int32_t*)sballoc(P * P * C * 4);
  int32_t* pprob = (int32_t*)sballoc(A * 4);

  m5_reset_stats(0, 0);

  for (int i = 0; i < M; i++)
    for (int j = 0; j < M; j++) {
      int x = image[i * M + j] << ishift;
      for (int k = 0; k < F; k++) {
        int p = i - k;
        if (p < 0 || p >= N) continue;
        for (int l = 0; l < (j + 1 < F ? j + 1 : F); l++) {
          int q = j - l;
          if (q < 0 || q >= N) continue;
          int32_t* conv = pconv + (p * N + q) * C;
          const int32_t* filter = cw + (k * F + l) * C;
          if (kind == 0) {
            for (int u = 0; u < C; u++) {
              conv[u] += x * filter[u];
            }
          } else if (kind == 1) {
            for (int u = 0; u < C; u += svcntw()) {
              svbool_t pg = svwhilelt_b32_s32(u, C);
              svint32_t vf = svld1_s32(pg, filter);
              svint32_t vc = svld1_s32(pg, conv);
              vf = svmul_n_s32_m(pg, vf, x);
              vc = svadd_m(pg, vc, vf);
              svst1(pg, conv, vc);
              filter += svcntw();
              conv += svcntw();
            }
          } else if (kind == 2) {
            int sf = frstream((const int8_t*)filter, 32, C << 5);
            int sc = frstream((const int8_t*)conv, 32, C << 5);
            int so = fwstream((int8_t*)conv, 32);
            for (int k = 0; k < C; k += svcntw()) {
              svint32_t vf = svunpack_s32(sf);
              svint32_t vc = svunpack_s32(sc);
              vf = svmul_n_s32_m(svptrue_b32(), vf, x);
              vc = svadd_m(svptrue_b32(), vc, vf);
              svpack_s32(svptrue_b32(), vc, so);
            }
            uint64_t bf = erstream(sf);
            assert(!bf);
            uint64_t bc = erstream(sc);
            assert(!bc);
            uint64_t bw = ewstream(so);
            assert(bw == C << 5);
          } else {
            assert(kind == 3);
            int sf = frstream((const int8_t*)filter, 32, C << 5);
            int sc = frstream((const int8_t*)conv, 32, C << 5);
            int so = fwstream((int8_t*)conv, 32);
            for (int k = 0, n = 0; k < C; k += n) {
              svint32_t vf = svvunpack(svptrue_b8(), sf);
              svint32_t vc = svvunpack(svptrue_b8(), sc);
              vf = svvmul_m(svptrue_b8(), vf, svvdup(x));
              vc = svvadd_m(svptrue_b8(), vc, vf);
              int nn = svvnum(svptrue_b8(), vc);
              crstream(sf, nn);
              crstream(sc, nn);
              int nnn = svvpack(svptrue_b8(), vc, so);
              n = svvrcnum();
              assert(n == nn && n == nnn);
            }
            uint64_t bf = erstream(sf);
            assert(!bf);
            uint64_t bc = erstream(sc);
            assert(!bc);
            uint64_t bw = ewstream(so);
            assert(bw == C << 5);
          }
        }
      }
    }

  dprintf("%s", "\n");
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      dprintf("i=%d,j=%d: ", i, j);
      int32_t* conv = pconv + (i * N + j) * C;
      for (int k = 0; k < C; k++) {
        dprintf("\t%d", conv[k]);
      }
      dprintf("%s", "\n");
    }
  dprintf("%s", "\n");

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      int32_t* conv = pconv + (i * N + j) * C;
      if (kind == 0) {
        for (int k = 0; k < C; k++) {
          conv[k] += cb[k];
        }
      } else if (kind == 1) {
        const int32_t* bias = cb;
        for (int k = 0; k < C; k += svcntw()) {
          svbool_t pg = svwhilelt_b32_s32(k, C);
          svint32_t v = svld1_s32(pg, bias);
          v = svadd_m(pg, v, svld1_s32(pg, conv));
          svst1_s32(pg, conv, v);
          conv += svcntw();
          bias += svcntw();
        }
      } else if (kind == 2) {
        int si = frstream((const int8_t*)cb, 32, C << 5);
        int sc = frstream((const int8_t*)conv, 32, C << 5);
        int so = fwstream((int8_t*)conv, 32);
        for (int k = 0; k < C; k += svcntw()) {
          svint32_t v = svunpack_s32(si);
          v = svadd_m(svptrue_b32(), v, svunpack_s32(sc));
          svpack_s32(svptrue_b32(), v, so);
        }
        uint64_t bl = erstream(si);
        assert(!bl);
        uint64_t bc = erstream(sc);
        assert(!bc);
        uint64_t bw = ewstream(so);
        assert(bw == C << 5);
      } else {
        assert(kind == 3);
        int si = frstream((const int8_t*)cb, 32, C << 5);
        int sc = frstream((const int8_t*)conv, 32, C << 5);
        int so = fwstream((int8_t*)conv, 32);
        for (int k = 0, n = 0; k < C; k += n) {
          svint32_t v = svvunpack(svptrue_b8(), si);
          v = svvadd_m(svptrue_b8(), v, svvunpack(svptrue_b8(), sc));
          int nn = svvnum(svptrue_b8(), v);
          crstream(si, nn);
          crstream(sc, nn);
          int nnn = svvpack(svptrue_b8(), v, so);
          n = svvrcnum();
          assert(n == nn && n == nnn);
        }
        uint64_t bl = erstream(si);
        assert(!bl);
        uint64_t bc = erstream(sc);
        assert(!bc);
        uint64_t bw = ewstream(so);
        assert(bw == C << 5);
      }
    }
  }

  dprintf("%s", "\n");
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      dprintf("i=%d,j=%d: ", i, j);
      int32_t* conv = pconv + (i * N + j) * C;
      for (int k = 0; k < C; k++) {
        dprintf("\t%d", conv[k]);
      }
      dprintf("%s", "\n");
    }
  dprintf("%s", "\n");

  for (int p = 0; p < P; p++)
    for (int q = 0; q < P; q++) {
      int i = p + p;
      int j = q + q;
      const int32_t* conv0 = pconv + i * N * C + j * C;
      const int32_t* conv1 = conv0 + C;
      const int32_t* conv2 = conv0 + N * C;
      const int32_t* conv3 = conv2 + C;
      int32_t* pool = ppool + p * P * C + q * C;

      if (kind == 0) {
        for (int k = 0; k < C; k++) {
          pool[k] = 0;
          pool[k] = pool[k] < conv0[k] ? conv0[k] : pool[k];
          pool[k] = pool[k] < conv1[k] ? conv1[k] : pool[k];
          pool[k] = pool[k] < conv2[k] ? conv2[k] : pool[k];
          pool[k] = pool[k] < conv3[k] ? conv3[k] : pool[k];
          pool[k] >>= idiff;
        }
      } else if (kind == 1) {
        for (int k = 0; k < C; k += svcntw()) {
          svbool_t pg = svwhilelt_b32_s32(k, C);
          svint32_t v = svdup_s32(0);
          v = svmax_m(pg, v, svld1_s32(pg, conv0));
          v = svmax_m(pg, v, svld1_s32(pg, conv1));
          v = svmax_m(pg, v, svld1_s32(pg, conv2));
          v = svmax_m(pg, v, svld1_s32(pg, conv3));
          v = svasr_m(pg, v, idiff);
          svst1_s32(pg, pool, v);
          conv0 += svcntw();
          conv1 += svcntw();
          conv2 += svcntw();
          conv3 += svcntw();
          pool += svcntw();
        }
      } else if (kind == 2) {
        int s0 = frstream((const int8_t*)conv0, 32, C << 5);
        int s1 = frstream((const int8_t*)conv1, 32, C << 5);
        int s2 = frstream((const int8_t*)conv2, 32, C << 5);
        int s3 = frstream((const int8_t*)conv3, 32, C << 5);
        int sp = fwstream((int8_t*)pool, 32);
        for (int k = 0; k < C; k += svcntw()) {
          svint32_t v = svdup_s32(0);
          v = svmax_m(svptrue_b32(), v, svunpack_s32(s0));
          v = svmax_m(svptrue_b32(), v, svunpack_s32(s1));
          v = svmax_m(svptrue_b32(), v, svunpack_s32(s2));
          v = svmax_m(svptrue_b32(), v, svunpack_s32(s3));
          v = svasr_m(svptrue_b32(), v, idiff);
          svpack_s32(svptrue_b32(), v, sp);
        }
        uint64_t b0 = erstream(s0);
        assert(!b0);
        uint64_t b1 = erstream(s1);
        assert(!b1);
        uint64_t b2 = erstream(s2);
        assert(!b2);
        uint64_t b3 = erstream(s3);
        assert(!b3);
        uint64_t bw = ewstream(sp);
        assert(bw == C << 5);
      } else {
        assert(kind == 3);
        int s0 = frstream((const int8_t*)conv0, 32, C << 5);
        int s1 = frstream((const int8_t*)conv1, 32, C << 5);
        int s2 = frstream((const int8_t*)conv2, 32, C << 5);
        int s3 = frstream((const int8_t*)conv3, 32, C << 5);
        int sp = fwstream((int8_t*)pool, 32);
        for (int k = 0, n = 0; k < C; k += n) {
          svint32_t v = svvdup(0);
          v = svvmax_m(svptrue_b8(), v, svvunpack(svptrue_b8(), s0));
          v = svvmax_m(svptrue_b8(), v, svvunpack(svptrue_b8(), s1));
          v = svvmax_m(svptrue_b8(), v, svvunpack(svptrue_b8(), s2));
          v = svvmax_m(svptrue_b8(), v, svvunpack(svptrue_b8(), s3));
          v = svvasr_m(svptrue_b8(), v, svvdup(idiff));
          int nn = svvpack(svptrue_b8(), v, sp);
          n = svvrcnum();
          assert(n == nn);
          crstream(s0, n);
          crstream(s1, n);
          crstream(s2, n);
          crstream(s3, n);
        }
        uint64_t b0 = erstream(s0);
        assert(!b0);
        uint64_t b1 = erstream(s1);
        assert(!b1);
        uint64_t b2 = erstream(s2);
        assert(!b2);
        uint64_t b3 = erstream(s3);
        assert(!b3);
        uint64_t bw = ewstream(sp);
        assert(bw == C << 5);
      }
    }

  dprintf("%s", "\n");
  for (int i = 0; i < P; i++)
    for (int j = 0; j < P; j++) {
      dprintf("i=%d,j=%d: ", i, j);
      int32_t* pool = ppool + i * P * C + j * C;
      for (int k = 0; k < C; k++) {
        dprintf("\t%d", pool[k]);
      }
      dprintf("%s", "\n");
    }
  dprintf("%s", "\n");

  int ans = 0;
  for (int i = 0; i < A; i++) {
    pprob[i] = db[i];
    const int32_t* px = ppool;
    const int32_t* weight = dw + i * D;
    if (kind == 0) {
      for (int j = 0; j < D; j++) {
        pprob[i] += ppool[j] * weight[j];
      }
    } else if (kind == 1) {
      for (int j = 0; j < D; j += svcntw()) {
        svbool_t pg = svwhilelt_b32_s32(j, D);
        svint32_t v = svld1_s32(pg, px);
        svint32_t w = svld1_s32(pg, weight);
        v = v = svmul_m(pg, v, w);
        pprob[i] += svaddv(pg, v);
        px += svcntw();
        weight += svcntw();
      }
    } else if (kind == 2) {
      int sp = frstream((const int8_t*)ppool, 32, D << 5);
      int sw = frstream((const int8_t*)weight, 32, D << 5);
      for (int j = 0; j < D; j += svcntw()) {
        svint32_t v = svunpack_s32(sp);
        svint32_t w = svunpack_s32(sw);
        v = svmul_m(svptrue_b32(), v, w);
        pprob[i] += svaddv(svptrue_b32(), v);
      }
      uint64_t bp = erstream(sp);
      assert(!bp);
      uint64_t bw = erstream(sw);
      assert(!bw);
    } else {
      assert(kind == 3);
      int sp = frstream((const int8_t*)ppool, 32, D << 5);
      int sw = frstream((const int8_t*)weight, 32, D << 5);
      for (int j = 0, n = 0; j < D; j += n) {
        svint32_t v = svvunpack(svptrue_b8(), sp);
        svint32_t w = svvunpack(svptrue_b8(), sw);
        v = svvmul_m(svptrue_b8(), v, w);
        pprob[i] += svvaddv(svptrue_b8(), v);
        n = svvrcnum();
        crstream(sp, n);
        crstream(sw, n);
      }
      uint64_t bp = erstream(sp);
      assert(!bp);
      uint64_t bw = erstream(sw);
      assert(!bw);
    }

    if (pprob[i] > pprob[ans]) ans = i;
  }

  m5_dump_stats(0, 0);

  for (int i = 0; i < A; i++) printf("%d\t", pprob[i]);
  printf("%s", "\n");

  return ans;
}
