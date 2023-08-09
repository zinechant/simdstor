#include <gem5/m5ops.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>

#include "debug.h"
#include "jacobi2d.hh"
#include "salloc.hh"

constexpr int N = 128;
constexpr int S = 10;

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
      arr[i * N + j] = (rand() % M) - (M / 2);
    }
}

int main(int argc, char* argv[]) {
  if (argc != 2 || (argv[1][0] < '0' || argv[1][0] > '3')) {
    fprintf(stderr, "Usage: %s <kind>\n", argv[0]);
    return 1;
  }

  T* A = (T*)sballoc(N * N * sizeof(T));
  T* B = (T*)sballoc(N * N * sizeof(T));

  init_array(A, N, N);

  dprint_array(A, N, N, "A");

  iprintflush("%s\n", "[INFO] Finished Initialization");

  m5_reset_stats(0, 0);
  const T* O = jacobi2d<T>(argv[1][0] - '0', A, B, N, S);
  m5_dump_stats(0, 0);

  std::string fo = "jacobi2d_?.out";
  fo[9] = argv[1][0];
  filewrite(fo.c_str(), N * sizeof(T) * 8, (const int8_t*)O);

  dprint_array(O, N, N, "O");

  return 0;
}
