#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// Offset would take a fixed-sized(two bytes) field
enum { HISTORY_OFFSET_BITS = 16 };
enum { HASH_BITS = 16 };
enum { HASH_MASK = ((1 << HASH_BITS) - 1) };
enum { HASH_TABLE_ENTRIES = (1 << HASH_BITS) };
enum { HASH_TABLE_BEAN_SIZE = 16 };
enum { HASH_TABLE_BEAN_MASK = 15 };
enum { CTRL_BITS = 4 };
enum { CTRL_MAX = ((1 << CTRL_BITS) - 1) };
enum { CTRL_W_MAX = (1 << (CTRL_BITS + CTRL_BITS)) - 1 };
enum { CTRL_THRESHOLD = CTRL_MAX + CTRL_MAX };

enum { PRIME2 = (int)2246822519L };
enum { PRIME3 = (int)3266489917L };
enum { PRIME4 = 668265263 };
enum { PRIME5 = 374761393 };

enum { SIMD_WIDTH_BITS = 0 };
enum { SIMD_MAX_BITS = 10 };
enum { SIMD_MASK = (1 << SIMD_WIDTH_BITS) - 1 };
enum { SIMD_BACKOFF = sizeof(unsigned int) + (1 << CTRL_BITS) - 1 };

#endif  // CONSTANTS_H_
