#include "base_meta_writer.h"

#include <immintrin.h>
#include <climits>

namespace {
/// Encodes a value in [0, 2^28 - 1] to radix 128 bytes.
///
/// The high bit of each byte must still be filled.
uint32_t radix_expand_32(uint32_t x) {
  assert(x < (1UL << 28));
#ifdef __BMI2__
  return _pdep_u32(x, 127 * (UINT32_MAX / 255));
#endif
  uint32_t high_half = (x & (-1UL << 14));
  uint32_t low_half = x & ((1UL << 14) - 1);

  x = (high_half << 2) | low_half;

  uint32_t low_mask = (127UL << 16) | 127;
  uint32_t high_quarters = x & ~low_mask;
  /* Shift the high quarters left by one bit. */
  x += high_quarters;
  return x;
}

/// Encodes a value in [0, 2^56 - 1] to radix 128 bytes.
///
/// The high bit of each byte must still be filled.
uint64_t radix_expand_64(uint64_t x) {
  assert(x < (1UL << 56));
#ifdef __BMI2__
  return _pdep_u64(x, 127 * (UINT64_MAX / 255));
#endif
  uint64_t high_half = x & (-1ULL << 28);

  if (__builtin_expect(high_half == 0, 1)) {
    return radix_expand_32(x);
  }

  uint64_t low_half = x & ((1ULL << 28) - 1);
  high_half = radix_expand_32(high_half);
  low_half = radix_expand_32(low_half);
  return (high_half << 32) | low_half;
}
}  // namespace

void BaseMetaWriter::imm_width(Opcode op, uint8_t imm1, uint32_t literal) {
  static const struct {
    uint8_t imm;
    uint8_t width; /* plus one for the opcode byte */
  } kWidthImm[33] = {
      [0] = {0, 1},

      [1] = {1, 2},  [2] = {1, 2},  [3] = {1, 2},  [4] = {1, 2},  [5] = {1, 2},
      [6] = {1, 2},

      [7] = {2, 3},  [8] = {2, 3},  [9] = {2, 3},  [10] = {2, 3}, [11] = {2, 3},
      [12] = {2, 3}, [13] = {2, 3},

      [14] = {3, 5}, [15] = {3, 5}, [16] = {3, 5}, [17] = {3, 5}, [18] = {3, 5},
      [19] = {3, 5}, [20] = {3, 5}, [21] = {3, 5}, [22] = {3, 5}, [23] = {3, 5},
      [24] = {3, 5}, [25] = {3, 5}, [26] = {3, 5}, [27] = {3, 5},
  };

  assert(imm1 < 4);
  assert(literal < (1UL << 28));

  uint8_t imm2;
  size_t width;

  if ((literal - 1) < 127) {
    imm2 = 1 << 3;
    width = 2;
  } else {
    size_t width_idx = (literal == 0) ? 0 : 32 - __builtin_clz(literal);
    imm2 = kWidthImm[width_idx].imm << 3;
    width = kWidthImm[width_idx].width;
  }

  // Expand literal to have 0s in the high bit of each byte.
  literal = radix_expand_32(literal) | (128UL * (UINT32_MAX / 255));

  uint8_t encoded = (uint8_t)op | imm2 | (imm1 << 5);
  uint64_t merged = encoded | ((uint64_t)literal << 8);
  void *dst = buf.reserve(sizeof(merged));
  memcpy(dst, &merged, sizeof(merged));
  buf.commit(width);
  return;
}

void BaseMetaWriter::imm_nonzero_width(Opcode op, uint8_t imm1,
                                       uint64_t literal) {
  static const struct {
    uint8_t imm;
    uint8_t width; /* Includes the opcode byte */
  } kWidthImm[64] = {
      [0] = {0, 2},  [1] = {0, 2},  [2] = {0, 2},  [3] = {0, 2},  [4] = {0, 2},
      [5] = {0, 2},  [6] = {0, 2},

      [7] = {1, 2},  [8] = {1, 3},  [9] = {1, 3},  [10] = {1, 3}, [11] = {1, 3},
      [12] = {1, 3}, [13] = {1, 3},

      [14] = {2, 3}, [15] = {2, 5}, [16] = {2, 5}, [17] = {2, 5}, [18] = {2, 5},
      [19] = {2, 5}, [20] = {2, 5}, [21] = {2, 5}, [22] = {2, 5}, [23] = {2, 5},
      [24] = {2, 5}, [25] = {2, 5}, [26] = {2, 5}, [27] = {2, 5},

      [28] = {3, 5}, [29] = {3, 9}, [30] = {3, 9}, [31] = {3, 9}, [32] = {3, 9},
      [33] = {3, 9}, [34] = {3, 9}, [35] = {3, 9}, [36] = {3, 9}, [37] = {3, 9},
      [38] = {3, 9}, [39] = {3, 9}, [40] = {3, 9}, [41] = {3, 9}, [42] = {3, 9},
      [43] = {3, 9}, [44] = {3, 9}, [45] = {3, 9}, [46] = {3, 9}, [47] = {3, 9},
      [48] = {3, 9}, [49] = {3, 9}, [50] = {3, 9}, [51] = {3, 9}, [52] = {3, 9},
      [53] = {3, 9}, [54] = {3, 9}, [55] = {3, 9}, [56] = {3, 9},
  };

  assert(imm1 < 4);
  assert(literal < (1UL << 56));

  uint8_t imm2;
  size_t width;

  if (literal < 128) {
    imm2 = 0;
    width = 2;
  } else {
    size_t width_idx = (literal == 0) ? 0 : 64 - __builtin_clzll(literal);
    imm2 = kWidthImm[width_idx].imm << 3;
    width = kWidthImm[width_idx].width;
  }

  literal = radix_expand_64(literal) | (128ULL * (UINT64_MAX / 255));

  void *dst = buf.reserve(1 + sizeof(literal));
  uint8_t encoded = (uint8_t)op | imm2 | (imm1 << 5);
  memcpy(dst, &encoded, 1);
  memcpy((uint8_t *)dst + 1, &literal, sizeof(literal));

  buf.commit(width);
  return;
}
