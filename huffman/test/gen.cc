#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "huffman.hh"

const codes_t codes_gen(int num, int seed) {
  srand(seed);

  int count[MAX_DIGITS + 1];
  std::vector<double> arr;

  for (int s = 1 << MAX_DIGITS; (s >> MAX_DIGITS);) {
    arr.clear();
    for (int i = 0; i <= MAX_DIGITS; i++) {
      count[i] = 0;
    }
    for (int i = 0; i < num; i++) {
      arr.push_back(((double)rand()) / RAND_MAX);
    }
    std::sort(arr.begin(), arr.end());
    s = 0;
    for (int i = 0; i < num; i++) {
      double d = i ? arr[i] - arr[i - 1] : arr[i];
      int x = 0.5 - log(d) / log(2);
      if (x > MAX_DIGITS) {
        s = 1 << MAX_DIGITS;
        break;
      } else {
        s += 1 << (MAX_DIGITS - x);
        count[x]++;
      }
    }
  }


  codes_t ans;
  for (int x = 0, i = 1; i <= MAX_DIGITS; i++) {
    while (count[i]--) {
      int y = 0;
      for (int k = 0, z = x++; k < i; k++) {
        y <<= 1;
        y += z & 1;
        z >>= 1;
      }
      ans.emplace_back(i, y);
    }
    x <<= 1;
  }
  return ans;
}

const codeids_t seq_gen(int num, int len, int seed) {
  srand(seed);
  codeids_t ans;

  for (int i = 0; i < len; i++) {
    ans.push_back(rand() % num);
  }

  return ans;
}

const dictionary_t dic_gen(const codes_t& codes) {
  dictionary_t ans;
  ans.resize(1 << MAX_DIGITS, -1);

  for (codeword_t c = 0; c < codes.size(); c++) {
    int i = codes[c].first;
    int j = codes[c].second;
    int k = MAX_DIGITS - i;
    for (int l = 0; l < (1 << k); l++) {
      ans[(l << i) | j] = c;
    }
  }

  return ans;
}
