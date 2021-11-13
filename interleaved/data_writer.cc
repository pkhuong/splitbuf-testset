#include "data_writer.h"

#include <assert.h>

void DataWriter::SelfTest() {
  {
    DataWriter self(10);
    uint64_t dst = 0;

    assert(1 == self.varint(0));
    assert(self.buf.written() == 1);
    memcpy(&dst, self.buf.data(), 1);
    assert(dst == 0);
  }

  for (size_t i = 0; i < 64; i++) {
    DataWriter self(10);
    const uint64_t value = 1ULL << i;
    uint64_t dst = 0;
    size_t size = self.varint(value);

    (void)size;
    assert(size == 1 || size == 2 || size == 4 || size == 8);
    memcpy(&dst, self.buf.data(), self.buf.written());
    assert(dst == value);
  }
}
