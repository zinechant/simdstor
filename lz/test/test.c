#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "lz.h"

int main(int argc, char* argv[]) {
  const char* data = NULL;
  char* data_buf;
  int input_bytes = 256;
  if (argc == 2) {
    FILE* fi = fopen(argv[1], "r");
    fseek(fi, 0, SEEK_END);
    input_bytes = ftell(fi) + 1;
    rewind(fi);
    data_buf = (char*)malloc(input_bytes);
    unsigned int bytes = fread(data_buf, 1, input_bytes, fi);
    assert(bytes == input_bytes - 1);
    _unused(bytes);
    data_buf[input_bytes - 1] = 0;
    data = data_buf;
  } else {
    data =
        "aaaabcdefghijklmn"
        "abcdefghijklmn"
        "abcdefghijklmn"
        "abcdefghijklmn"
        "mnopqrstuvwx abcdefghijklmnopqrstuvwxy abcdefghijklmnopqrstuvwxyz "
        "mnopqrstuvwxyzABCDEFGHIJKLMN  "
        "mnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ "
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    data_buf = (char*)malloc(sizeof(int));
    input_bytes = strlen(data) + 1;
  }

  meta_t meta;
  meta.bytes = input_bytes - 1;

  uint8_t* ctrl = (uint8_t*)malloc(input_bytes);
  char* comp = (char*)malloc(input_bytes);
  char* output = (char*)malloc(input_bytes);

  compress(&meta, data, ctrl, comp);
  printf("%u\t%u\t%u\t%.2f%%\n", meta.bytes, meta.ctrls, meta.comps,
         1e2 * (meta.comps + ((meta.ctrls + 1) >> 1)) / meta.bytes);

  decompress(&meta, output, ctrl, comp);
  output[meta.bytes] = 0;

  if (memcmp(data, output, meta.bytes)) {
    printf("!!!Round Mismatch!!!\n%s\n%s\n", data, output);
  }

  free(ctrl);
  free(comp);
  free(output);
  free(data_buf);
  return 0;
}
