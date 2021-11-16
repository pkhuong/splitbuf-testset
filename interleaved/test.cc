#include <sys/time.h>
#include <cstdint>
#include <iostream>

#include "base_meta_writer.h"
#include "data_writer.h"

namespace {
void data() {
  DataWriter data(100);

  std::cout << "1: " << data.string("") << "\n";
  std::cout << "2: " << data.varint(8) << "\n";
  std::cout << "3: " << data.varint(2066379) << "\n";
  std::cout << "4: " << data.string("3K+6)#") << "\n";
  std::cout << "9: "
            << data.string(
                   "10)2uiSuoXL1^)v}icF@>P(j<t#~tz\\lg??S&(<hr7EVs\'l{\'5`Gohc_"
                   "(=t eS s{_I?iCwaG]L\'*Pu5(&w_:4{~Z")
            << "\n";
  std::cout << "12: " << data.fixed(true) << "\n";
  std::cout << "13: " << data.fixed(false) << "\n";
  std::cout << "14: " << data.fixed(true) << "\n";

  std::cout << "15.1: " << data.varint(25) << "\n";
  std::cout << "15.2: " << data.varint(36) << "\n";
  std::cout << "15.15: "
            << data.string(
                   "\"?6PY4]L2c<}~2;\\TVF_w^[@YfbIc*v/"
                   "N+Z-oYuaWZr4C;5ib|*s@RCBbuvrQ3g(k,N")
            << "\n";
  std::cout << "15.21: " << data.fixed<uint64_t>(2813090458170031956) << "\n";
  std::cout << "15.22: " << data.varint(38) << "\n";
  std::cout << "15.23: " << data.fixed(true) << "\n";

  std::cout << "17: " << data.fixed(false) << "\n";
  std::cout << "18: " << data.string("{=Qwfe~#n{") << "\n";
  std::cout << "67: " << data.varint(1591432) << "\n";
  std::cout << "100: " << data.varint(31) << "\n";
}

struct Submessage {
  int32_t field_1{25};
  int32_t field_2{36};
  std::string field_15{
      "\"?6PY4]L2c<}~2;\\TVF_w^[@YfbIc*v/"
      "N+Z-oYuaWZr4C;5ib|*s@RCBbuvrQ3g(k,N"};
  uint64_t field_21{2813090458170031956};
  int32_t field_22{38};
  bool field_23{true};
};

struct Message {
  int32_t field_2{8};
  int32_t field_3{2066379};
  std::string field_4{"3K+6)#"};
  std::string field_9{
      "10)2uiSuoXL1^)v}icF@>P(j<t#~tz\\lg??S&(<hr7EVs\'l{\'5`Gohc_"
      "(=t eS s{_I?iCwaG]L\'*Pu5(&w_:4{~Z"};
  bool field_12{true};
  bool field_13{false};
  bool field_14{true};
  Submessage field_15;
  bool field_17{false};
  std::string field_18{"{=Qwfe~#n{"};
  int32_t field_67{1591432};
  int32_t field_100{31};
};

__attribute__((noinline)) void overhead(const Message &message,
                                        WriteBuffer *meta_buf,
                                        WriteBuffer *data_buf) {
  BaseMetaWriter meta(std::move(*meta_buf));
  DataWriter data(std::move(*data_buf));

  asm volatile("" ::"r"(&message) : "memory");

  asm volatile("" ::"r"(&data), "r"(&meta) : "memory");

  *meta_buf = std::move(meta.buf);
  *data_buf = std::move(data.buf);
  return;
}

__attribute__((noinline)) void test_meta(const Message &message,
                                         WriteBuffer *meta_buf,
                                         WriteBuffer *data_buf) {
  BaseMetaWriter meta(std::move(*meta_buf));
  DataWriter data(std::move(*data_buf));

  asm volatile("" ::"r"(&message) : "memory");
  {
    uint8_t width = data.varint(message.field_2);

    // 1, 2
    meta.one_field(1, width);
  }

  {
    uint8_t width = data.varint(message.field_3);
    size_t len = data.string(message.field_4);

    // 3, 4
    meta.field_n(width, len);
  }

  // 5-8
  meta.skip(4);

  {
    size_t len = data.string(message.field_9);

    // 9
    meta.field_n(0, len);
  }

  // 12
  meta.one_field(2, data.fixed(message.field_12));
  {
    uint8_t w1 = data.fixed(message.field_13);
    uint8_t w2 = data.fixed(message.field_14);

    // 13, 14
    meta.two_fields(w1, w2);
  }

  size_t submessage_begin = data.buf.written();
  const Submessage &sub_15 = message.field_15;

  {
    uint8_t width = data.varint(sub_15.field_1);

    meta.open_field(0, width);
  }

  {
    uint8_t width = data.varint(sub_15.field_2);

    meta.one_field(0, width);
  }

  // 15.3-15.14
  meta.skip(12);

  {
    size_t len = data.string(sub_15.field_15);

    meta.field_n(0, len);
  }

  // 15.16-15.20
  meta.skip(5);

  {
    uint8_t w1 = data.fixed(sub_15.field_21);
    uint8_t w2 = data.varint(sub_15.field_22);

    meta.two_fields(w1, w2);
  }

  {
    uint8_t w = data.fixed(sub_15.field_23);

    meta.field_close(w, data.buf.written() - submessage_begin);
  }

  {
    uint8_t w = data.fixed(message.field_17);

    // 16, 17
    meta.one_field(1, w);
  }

  {
    size_t len = data.string(message.field_18);

    // 18
    meta.field_n(0, len);
  }

  // 19..66
  meta.skip(48);

  {
    uint8_t w = data.varint(message.field_67);

    // 67
    meta.one_field(0, w);
  }

  meta.skip(32);
  {
    uint8_t w = data.varint(message.field_100);

    meta.field_close(w, data.buf.written());
  }

  asm volatile("" ::"r"(&data), "r"(&meta) : "memory");

  *meta_buf = std::move(meta.buf);
  *data_buf = std::move(data.buf);
  return;
}

double now() {
  timeval tv;

  gettimeofday(&tv, nullptr);
  return tv.tv_sec + 1e-6 * tv.tv_usec;
}

void decode_meta(const uint8_t *bytes, size_t count) {
  std::cout << "Meta stream";
  for (size_t i = 0; i < count; i++) {
    uint8_t byte = bytes[i];

    // Literal byte
    if (byte > 127) {
      std::cout << " " << (size_t)(byte - 128);
      continue;
    }

    Opcode op = (Opcode)(byte % 8);
    size_t imm1 = byte >> 5;
    size_t imm2 = (byte >> 3) % 4;

    std::cout << "\n\t" << OpcodeName(op) << "[" << imm1 << ", " << imm2 << "]";
  }

  std::cout << "\n";
  return;
}
}  // namespace

int main(int, char **) {
  DataWriter::SelfTest();

  data();

  size_t niter = 10000000;
  Message message;
  WriteBuffer meta(128);
  WriteBuffer data(128);

  test_meta(message, &meta, &data);
  decode_meta((const uint8_t *)meta.data(), meta.written());
  std::cout << "Data: " << data.written() << "; meta: " << meta.written()
            << "\n";

  {
    double begin = now();
    for (size_t i = 0; i < niter; i++) {
      meta.reset();
      data.reset();
      asm volatile("" : "+m"(message));
      overhead(message, &meta, &data);
    }

    double end = now();

    std::cout << "Overhead: " << 1e9 * (end - begin) / niter << " ns/iter\n";
  }

  {
    double begin = now();
    for (size_t i = 0; i < niter; i++) {
      meta.reset();
      data.reset();
      asm volatile("" : "+m"(message));
      test_meta(message, &meta, &data);
    }

    double end = now();

    std::cout << "Write: " << 1e9 * (end - begin) / niter << " ns/iter\n";
  }

  return 0;
}
