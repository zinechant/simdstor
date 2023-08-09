#include <gem5/m5ops.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>

#include "debug.h"
#include "salloc.hh"
#include "trisolv.hh"

constexpr int N = 128;

using T = int32_t;

void dprint_array(const T* arr, const int M, const int N, const char* text) {
  dprintf("\n%s\n", text);
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      dprintf("\t%x", arr[i * N + j]);
    }
    dprintf("%s", "\n");
  }
}

void init_array(T* arr, const int M, const int N) {
  for (int i = 0; i < M; i++)
    for (int j = 0; j < N; j++) {
      arr[i * N + j] = (rand() % 32) - 16;
    }
}

int main(int argc, char* argv[]) {
  if (argc != 2 || (argv[1][0] < '0' || argv[1][0] > '3')) {
    fprintf(stderr, "Usage: %s <kind>\n", argv[0]);
    return 1;
  }

  T* A = (T*)sballoc(N * N * sizeof(T));
  T* C = (T*)sballoc(N * sizeof(T));
  T* D = (T*)sballoc(N * sizeof(T));

  init_array(A, N, N);
  init_array(C, 1, N);
  init_array(D, 1, N);

  dprint_array(A, N, N, "A");
  dprint_array(C, 1, N, "C");
  dprint_array(D, 1, N, "D");

  iprintflush("%s\n", "[INFO] Finished Initialization");

  m5_reset_stats(0, 0);
  trisolv<T>(argv[1][0] - '0', A, C, D, N);
  m5_dump_stats(0, 0);

  std::string fo = "trisolv_?.out";
  fo[8] = argv[1][0];
  filewrite(fo.c_str(), N * sizeof(T) * 8, (const int8_t*)D);

  dprint_array(D, 1, N, "D");

  return 0;
}
