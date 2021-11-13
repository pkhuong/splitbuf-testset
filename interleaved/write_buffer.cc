#include "write_buffer.h"

#include <assert.h>
#include <climits>
#include <cstdlib>

WriteBuffer::WriteBuffer(size_t capacity) {
  buf_ = (uint8_t *)malloc(capacity);
  assert(buf_ != nullptr);
  remaining_ = capacity;
  buf_end_ = buf_ + capacity;
  write_cursor_ = buf_;
  return;
}

void *WriteBuffer::reserve_slow(size_t count) __restrict__ {
  size_t size = write_cursor_ - buf_;
  size_t capacity = buf_end_ - buf_;
  size_t new_capacity = capacity;
  size_t goal = size + count;

  assert(size <= SSIZE_MAX);
  assert(count < SSIZE_MAX - size);
  if (new_capacity < 64) new_capacity = 64;

  while (new_capacity < goal) new_capacity *= 2;

  uint8_t *ret = (uint8_t *)realloc(buf_, new_capacity);
  assert(ret != nullptr);

  buf_ = ret;
  buf_end_ = ret + new_capacity;
  remaining_ = new_capacity - size;
  write_cursor_ = buf_ + size;
  return write_cursor_;
}
