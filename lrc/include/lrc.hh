#ifndef LRC_HH_
#define LRC_HH_

#include <cstdint>

#include "erasure.hh"

namespace lrc {

const int L = 15;
const uint8_t A[L] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
const uint8_t B[L] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                      0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0};
const uint8_t C[L] = {
    GF_MUL[0x01][0x01], GF_MUL[0x02][0x02], GF_MUL[0x03][0x03],
    GF_MUL[0x04][0x04], GF_MUL[0x05][0x05], GF_MUL[0x06][0x06],
    GF_MUL[0x07][0x07], GF_MUL[0x08][0x08], GF_MUL[0x09][0x09],
    GF_MUL[0x0a][0x0a], GF_MUL[0x0b][0x0b], GF_MUL[0x0c][0x0c],
    GF_MUL[0x0d][0x0d], GF_MUL[0x0e][0x0e], GF_MUL[0x0f][0x0f]};
const uint8_t D[L] = {
    GF_MUL[0x10][0x10], GF_MUL[0x20][0x20], GF_MUL[0x30][0x30],
    GF_MUL[0x40][0x40], GF_MUL[0x50][0x50], GF_MUL[0x60][0x60],
    GF_MUL[0x70][0x70], GF_MUL[0x80][0x80], GF_MUL[0x90][0x90],
    GF_MUL[0xa0][0xa0], GF_MUL[0xb0][0xb0], GF_MUL[0xc0][0xc0],
    GF_MUL[0xd0][0xd0], GF_MUL[0xe0][0xe0], GF_MUL[0xf0][0xf0]};

void Encode(int size, uint8_t* message[], uint8_t* parity[]);

void Decode(int size, uint8_t* message[], uint8_t* parity[]);

}  // namespace lrc

#endif  // LRC_HH_
