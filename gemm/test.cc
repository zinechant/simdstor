#include <gem5/m5ops.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <random>

#include "debug.h"
#include "gemm.hh"
#include "salloc.hh"

constexpr int M = 32;
constexpr int N = 16;
constexpr int L = 128;

using T = int32_t;
constexpr T a = 3;
constexpr T b = 5;

void print_array(const T* arr, const int M, const int N, const char* text) {
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

  T* A = (T*)sballoc(M * N * sizeof(T));
  T* B = (T*)sballoc(N * L * sizeof(T));
  T* C = (T*)sballoc(M * L * sizeof(T));
  T* D = (T*)sballoc(M * L * sizeof(T));

  init_array(A, M, N);
  init_array(B, N, L);
  init_array(C, M, L);

  print_array(A, M, N, "A");
  print_array(B, N, L, "B");
  print_array(C, M, L, "C");

  iprintflush("%s\n", "[INFO] Finished Initialization");

  m5_reset_stats(0, 0);
  T* arr = gemm<T>(argv[1][0] - '0', A, B, C, D, a, b, M, N, L);
  m5_dump_stats(0, 0);

  std::string fo = "gemm_?.out";
  fo[5] = argv[1][0];
  filewrite(fo.c_str(), M * L * sizeof(T) * 8, (const int8_t*)arr);

  print_array(arr, M, L, "RES");

  return 0;
}
