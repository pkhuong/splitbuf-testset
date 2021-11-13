#pragma once

#include <cstddef>
#include <cstring>
#include <string_view>
#include <vector>

#include "write_buffer.h"

/// `DataWriter`s wrap a `WriteBuffer` with utility methods to easily
/// populate a data stream.  Directly go through the `buf` member to
/// access the underlying `WriteBuffer`.
///
/// Writing actual data to the data stream is trivial: just copy the
/// bytes and remember how much you wrote, to encode that in the
/// metadata stream.
///
/// The one exception is variable-length integers, which can be
/// written as 1, 2, 4, or 8 byte values.
struct DataWriter {
  DataWriter() = delete;

  /// Wraps this `buf`.
  explicit DataWriter(WriteBuffer buf_) : buf(std::move(buf_)) {}

  /// Wraps a `WriteBuffer` with initial `capacity`.
  explicit DataWriter(size_t capacity) : buf(capacity) {}

  /// `DataWriter`s are move-only, like `WriteBuffer`s.
  DataWriter(const DataWriter &) = delete;
  DataWriter(DataWriter &&) = default;
  DataWriter &operator=(const DataWriter &) = delete;
  DataWriter &operator=(DataWriter &&) = default;

  ~DataWriter() = default;

  static void SelfTest();

  /// Writes a vector of values
  ///
  /// Returns the number of bytes written.
  template <typename T>
  size_t vec(const std::vector<T> &vec) {
    return this->bytes(&vec.front(), vec.size());
  }

  /// Writes a span of values.
  ///
  /// Returns the number of bytes written.
  template <typename T>
  size_t bytes(const T *ptr, size_t count) {
    size_t size = sizeof(T) * count;
    void *dst = buf.reserve(size);

    memcpy(dst, ptr, size);
    return buf.commit(size);
  }

  /// Writes a string.
  ///
  /// Returns the number of bytes written.
  size_t string(std::string_view value) {
    size_t size = value.size();
    void *dst = buf.reserve(size);

    memcpy(dst, value.data(), size);
    return buf.commit(size);
  }

  /// Writes a variable-size unsigned integer value to
  /// the buf buf.
  ///
  /// Returns the number of bytes written; the byte count
  /// is always 1, 2, 4, or 8.
  size_t varint(uint64_t value) {
    void *dst = buf.reserve(sizeof(value));
    size_t count = (63 - __builtin_clzll(value | 1)) / 8;

    memcpy(dst, &value, sizeof(value));
    /*
     * `count` is biased at one less than the byte count.  Round up to
     * the next power of 2.
     */
    count |= count >> 1;
    count |= count >> 2;
    count += 1;

    return buf.commit(count);
  }

  /// Writes a fixed-size value.
  ///
  /// Returns the number of bytes written.
  template <typename T>
  size_t fixed(T value) {
    void *dst = buf.reserve(sizeof(T));

    memcpy(dst, &value, sizeof(T));
    return buf.commit(sizeof(T));
  }

  WriteBuffer buf;
};
