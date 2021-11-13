#pragma once

#include <cstdint>
#include <string>

/// Opcodes for the metadata stream.  These integer values aren't
/// optimised for SIMD processing.
enum class Opcode : uint8_t {
  /// Skip multiple fields (or no-op if the count is 0).
  ///
  /// The first immediate is a skip count in [0, 3], and the second
  /// immediate is the zeroable width (0, 1, 2, 4 literal bytes mapped
  /// to [0, 3]) of the remaining count.  The actual count is encoded
  /// as that many radix-128 literal bytes in the metadata stream.
  SkipN = 0,

  /// Skip and a machine word field.
  ///
  /// The first immediate is the number of fields to skip, in [0, 3],
  /// and the second is the non-zero width of the data (1, 2, 4, or 8
  /// bytes mapped to 0, 1, 2, 3).
  OneField = 1,

  /// Two machine word fields.
  ///
  /// The first immediate is the non-zero width of the first field's
  /// data (1, 2, 4, or 8 bytes mapped to 0, 1, 2, 3), and the second
  /// immediate the non-zero width of the second field's data.
  TwoFields = 2,

  /// Open a run of submessages, and an optional machine word field.
  /// Each run is optionally tagged with a size hint, for the number
  /// of submessages in the run (0 bytes for the size hint means no
  /// size hint).
  ///
  /// The first immediate is a nullable width of the data (None, 1, 2,
  /// or 4 bytes mapped to 0, 1, 2, 3), and the second is the zeroable
  /// width of the size hint (0, 1, 2, or 4 mapped to [0, 3]).  The
  /// size hint is encoded in little-endian, in the data stream.
  OpenField = 3,

  /// A small machine word field, followed by the end of a run of
  /// submessages (and the end of the last submessage in that run).
  /// Each run is tagged with the total size of the run's submessages
  /// in the data stream.
  ///
  /// The first immediate is the zeroable width of the final
  /// submessage's last field (0, 1, 2, or 4 bytes mapped to [0, 3]),
  /// and the second the non-zero width for the total size of the
  /// run's data (1, 2, 4, or 8 bytes mapped to [0, 3]).  The total
  /// size is encoded as that many radix-128 literal bytes in the
  /// metadata stream.
  FieldClose = 4,

  /// A small machine word field, followed by the end of the current
  /// submessages and the beginning of a new one, in the same run of
  /// submessages.  This separator is tagged with the size of the
  /// submessage that just ended.
  ///
  /// The first immediate is the zeroable width of the previous (just
  /// closed) submessage's last field (0, 1, 2, or 4 bytes mapped to
  /// [0, 3]), and the second the non-zero width for the size of the
  /// data for the previous submessage (1, 2, 4, or 8 bytes mapped to
  /// [0, 3]).  The submessage's size is encoded as that many
  /// radix-128 literal bytes in the metadata stream.
  FieldSeparate = 5,

  /// An optional machine word field, followed by an arbitrary size
  /// field.
  ///
  /// The first immediate is the nullable width of the machine word
  /// field (None, 1, 2, or 4 bytes mapped to [0, 3]), and the second
  /// is the non-zero width of the *size* of the field (1, 2, 4, or 8
  /// bytes mapped to [0, 3]).  The field's size is encoded as that
  /// many radix-128 literal bytes in the metadata stream, and the
  /// field takes up exactly that many bytes in the data stream.
  FieldN = 6,
};

std::string OpcodeName(Opcode);
