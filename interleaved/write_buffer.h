#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

/// A WriteBuffer is a growable linear array of payload bytes, with an
/// interface that makes it possible to write fixed size values (e.g.,
/// 8-byte uint64_t), and only commit a fraction of the bytes actually
/// written (e.g., only the low 2 bytes).
class WriteBuffer {
 public:
  /// The default constructor creates a buffer with zero capacity
  /// (which will grow on demand).
  WriteBuffer() {}

  /// Constructs a buffer with initial capacity for this many bytes.
  ///
  /// `WriteBuffer`s grow on demand.
  explicit WriteBuffer(size_t capacity);

  /// `WriteBuffer`s are move-only: we don't want to accidentally
  /// pay for duplicating a variable-length array of bytes.
  WriteBuffer(const WriteBuffer &) = delete;
  inline WriteBuffer(WriteBuffer &&);
  WriteBuffer &operator=(const WriteBuffer &) = delete;
  inline WriteBuffer &operator=(WriteBuffer &&);

  inline ~WriteBuffer();

  /// Reserves space at the write cursor for up to `count` bytes.
  ///
  /// Returns a pointer to the write location that is valid for
  /// up to `count` bytes, until the next call to `reserve`.
  inline void *reserve(size_t count) __restrict__ {
    if (__builtin_expect(count > remaining_, 0)) return reserve_slow(count);

    return write_cursor_;
  }

  /// Commits `actual` bytes after a `reserve` call.  The sum of
  /// `actual` bytes since the last call to `reserve` must not
  /// exceed the `count` value passed to that `reserve` call.
  ///
  /// Returns `actual`.
  inline size_t commit(size_t actual) __restrict__ {
    write_cursor_ += actual;
    remaining_ -= actual;
    return actual;
  }

  /// Resets the write buffer to an empty state (nothing written).
  inline void reset() {
    write_cursor_ = buf_;
    remaining_ = buf_end_ - buf_;
    return;
  }

  /// Returns the linear byte buffer for the data written so far.
  inline const void *data() const { return buf_; }

  /// Returns the number of bytes written (committed) to this write
  /// buffer.
  inline size_t written() const { return (size_t)(write_cursor_ - buf_); }

 private:
  __attribute__((noinline)) void *reserve_slow(size_t count) __restrict__;

  uint8_t *write_cursor_{nullptr};
  size_t remaining_{0};
  uint8_t *buf_{nullptr};
  uint8_t *buf_end_{nullptr};
};

WriteBuffer::WriteBuffer(WriteBuffer &&other)
    : write_cursor_(other.write_cursor_),
      remaining_(other.remaining_),
      buf_(other.buf_),
      buf_end_(other.buf_end_) {
  other.write_cursor_ = nullptr;
  other.remaining_ = 0;
  other.buf_ = nullptr;
  other.buf_end_ = nullptr;
  return;
}

WriteBuffer &WriteBuffer::operator=(WriteBuffer &&other) {
  using std::swap;

  swap(write_cursor_, other.write_cursor_);
  swap(remaining_, other.remaining_);
  swap(buf_, other.buf_);
  swap(buf_end_, other.buf_end_);
  return *this;
}

WriteBuffer::~WriteBuffer() {
  if (buf_ != nullptr) free(buf_);
  return;
}
