#include <cstdio>
#include <cstring>
#include <string>

extern const std::string tdir = std::string("./data/");

extern void q02_part(int kind);

int main(int argc, char* argv[]) {
  if (argc != 3 || argv[2][0] > '3' || argv[2][0] < '0') {
    fprintf(stderr, "Wrong Input! Usage: %s <query> <0/1/2>\n", argv[0]);
    return 1;
  }
  if (!strcmp(argv[1], "q02_part")) {
    q02_part(argv[2][0] - '0');
  } else {
    fprintf(stderr, "Wrong Query: %s!\n", argv[1]);
  }

  return 0;
}
