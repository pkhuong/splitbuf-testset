#pragma once

#include <assert.h>
#include <algorithm>
#include <cstdint>
#include <cstring>

#include "opcode.h"
#include "write_buffer.h"

/// `BaseMetaWriter`s wrap a `WriteBuffer` with utility methods to
/// emit metadata "instructions".  Directly go through the `buf`
/// member to access the underlying `WriteBuffer`.
struct BaseMetaWriter {
  BaseMetaWriter() = delete;

  explicit BaseMetaWriter(WriteBuffer buf_) : buf(std::move(buf_)) {}
  explicit BaseMetaWriter(size_t capacity) : buf(capacity) {}

  BaseMetaWriter(const BaseMetaWriter &) = delete;
  BaseMetaWriter(BaseMetaWriter &&) = default;
  BaseMetaWriter &operator=(const BaseMetaWriter &) = delete;
  BaseMetaWriter &operator=(BaseMetaWriter &&) = delete;

  ~BaseMetaWriter() = default;

  /// Emits a skip opcode for `num_skipped` fields. 0 is a no-op.
  inline void skip(uint32_t num_skipped);

  /// Emits a single-field opcode: skip `num_skipped` fields, and
  /// followed by a non-zero field width.
  inline void one_field(uint8_t num_skipped, uint8_t data_width);

  /// Emits two non-zero field widths.
  inline void two_fields(uint8_t data_width1, uint8_t data_width2);

  /// Emits a submessage open, with `len_hint` for the number of
  /// repeated submessages, and a nullable field width.
  inline void open_field(uint32_t len_hint, uint8_t optional_data_width);

  /// Emits a field width (0 for skip) and marks the end of a submessage
  /// sequence, with the total size of the submessages' data.
  inline void field_close(uint8_t data_width, uint64_t sequence_size);

  /// Emits a field width (0 for skip) and separates two submessages in a
  /// sequence, with the size of the submessage that was just closed.
  inline void field_separate(uint8_t data_width, uint64_t message_size);

  /// Emits a nullable field width (0 for None), and an arbitrary field
  /// size.
  inline void field_n(uint8_t optional_data_width, uint64_t data_size);

  /// Helpers for the high-level emitters above.

  /// Returns the immediate value for a machine word width in {0, 1,
  /// 2, 4}.
  static inline uint8_t immediate_for_zeroable_width(uint8_t data_width);

  /// Returns the immediate value for a machine word width in {1, 2,
  /// 4, 8}.
  static inline uint8_t immediate_for_nonzero_width(uint8_t data_width);

  /// Emits an opcode with two immediates in [0, 3]
  inline void imm_imm(Opcode op, uint8_t imm1, uint8_t imm2);

  /// Emits an opcode with one immediate in [0, 3], and a literal
  /// payload in [0, 2^28 - 1].
  void imm_width(Opcode op, uint8_t imm1, uint32_t literal);

  /// Emits an opcode with one immediate in [0, 3], and a literal
  /// payload in [0, 2^56 - 1].
  void imm_nonzero_width(Opcode op, uint8_t imm1, uint64_t literal);

  WriteBuffer buf;
};

inline void BaseMetaWriter::skip(uint32_t num_skipped) {
  assert(num_skipped < (1UL << 28));

  uint8_t low = (num_skipped < 4) ? num_skipped : 3;
  num_skipped -= low;
  imm_width(Opcode::SkipN, low, num_skipped);
  return;
}

inline void BaseMetaWriter::one_field(uint8_t num_skipped, uint8_t data_width) {
  assert(num_skipped < 4);
  assert(data_width > 0 && data_width <= 8 &&
         (data_width & (data_width - 1)) == 0);

  imm_imm(Opcode::OneField, num_skipped,
          immediate_for_nonzero_width(data_width));
  return;
}

inline void BaseMetaWriter::two_fields(uint8_t data_width1,
                                       uint8_t data_width2) {
  assert(data_width1 > 0 && data_width1 <= 8 &&
         (data_width1 & (data_width1 - 1)) == 0);
  assert(data_width2 > 0 && data_width2 <= 8 &&
         (data_width2 & (data_width2 - 1)) == 0);

  imm_imm(Opcode::TwoFields, immediate_for_nonzero_width(data_width1),
          immediate_for_nonzero_width(data_width2));
  return;
}

inline void BaseMetaWriter::open_field(uint32_t len_hint,
                                       uint8_t optional_data_width) {
  assert(len_hint < (1UL << 28));
  assert(optional_data_width <= 4 &&
         (optional_data_width & (optional_data_width - 1)) == 0);

  imm_width(Opcode::OpenField,
            immediate_for_zeroable_width(optional_data_width), len_hint);
  return;
}

inline void BaseMetaWriter::field_close(uint8_t data_width,
                                        uint64_t sequence_size) {
  assert(data_width <= 4 && (data_width & (data_width - 1)) == 0);
  assert(sequence_size < (1UL << 56));

  imm_nonzero_width(Opcode::FieldClose,
                    immediate_for_zeroable_width(data_width), sequence_size);
  return;
}

inline void BaseMetaWriter::field_separate(uint8_t data_width,
                                           uint64_t message_size) {
  assert(data_width <= 4 && (data_width & (data_width - 1)) == 0);
  assert(message_size < (1UL << 56));

  imm_nonzero_width(Opcode::FieldSeparate,
                    immediate_for_zeroable_width(data_width), message_size);
  return;
}

inline void BaseMetaWriter::field_n(uint8_t optional_data_width,
                                    uint64_t data_size) {
  assert(optional_data_width <= 4 &&
         (optional_data_width & (optional_data_width - 1)) == 0);
  assert(data_size < (1UL << 56));

  imm_nonzero_width(Opcode::FieldN,
                    immediate_for_zeroable_width(optional_data_width),
                    data_size);
  return;
}

inline uint8_t BaseMetaWriter::immediate_for_zeroable_width(
    uint8_t data_width) {
  assert(data_width <= 4 && (data_width & (data_width - 1)) == 0);
  // We want the identify for [0, 2], and 4 -> 3.
  return data_width - (data_width / 4);
}

inline uint8_t BaseMetaWriter::immediate_for_nonzero_width(uint8_t data_width) {
  assert(data_width <= 8 && data_width > 0 &&
         (data_width & (data_width - 1)) == 0);
  //  1 -> 0
  //  2 -> 1
  //  4 -> 2
  //  8 -> 3 = 4 - 1
  return (data_width / 2) - (data_width / 8);
}

inline void BaseMetaWriter::imm_imm(Opcode op, uint8_t imm1, uint8_t imm2) {
  assert(imm1 < 4);
  assert(imm2 < 4);

  uint8_t encoded = (uint8_t)op | (imm2 << 3) | (imm1 << 5);
  memcpy(buf.reserve(1), &encoded, 1);

  buf.commit(1);
  return;
}
