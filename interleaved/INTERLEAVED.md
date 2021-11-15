Naïve interleaved metadata
==========================

The naive interleaved format encodes a metadata stream with any
literal size (for field skips, variable-length fields, and message
delimiter/close) immediately after the opcode byte.

Each opcode byte populates 7 bits: the low 3 bits encode the opcode,
the next two bits the second immediate, and the other two the first
immediate.  The top 8th bit is always 0.

Literal bytes encode little-endian 7-bit values, and the top 8th bit
is always set to 1.

The `SkipN` (variable length skip) opcode has an optional count
encoded in literal radix-128 bytes.  The number of literal bytes is
encoded in the second immediate: 0 -> 0 bytes, 1 -> 1 byte, 2 -> 2
bytes, 3 -> 4 bytes.

The `FieldClose` (one field and end of message run) opcode has a
mandatory run size in the literal bytes, with the number of radix-128
bytes encoded in the second immediate: 0 -> 1 byte, 1 -> 2 bytes, 2 ->
4 bytes, 3 -> 8 bytes.

The `FieldSeparate` (one field, end of a submessage, and beginning of
the next) opcode has a mandatory submessage size in the literal bytes,
wit the same encoding as `FieldClose`.

Finally, the `FieldN` (one field, and an arbitrary length field)
opcode has another mandatory field size in the literal bytes, with
the same encoding as `FieldClose` and `FieldSeparate`.

This encoding is compact, but doesn't expose a lot of decoding
parallelism.  Its one saving grace is the top bit tag on literals,
which makes it easy to skip ahead to the next opcode from an arbitrary
offset, and to counting matching open/close opcodes without having to
decode literals.  PDEP/PEXT for radix-128<->radix-256 bytes isn't too
much of an issue (it's not on the critical path when decoding a series
of opcodes), but it's more of a problem on AMD.

(Reportedly Zen 3 has better PDEP/PEXT performance than previous Zen designs.
Sadly ARM may not have a good solution for PDEP/PEXT. This being said, it does
not appear that PDEP/PEXT is so important for performance? Empirically, the
initial simdjson engine required PDEP/PEXT but the dependency was removed after
careful benchmarks showed that it provided little to no benefit.)

```
$ g++-8 -fno-exceptions -W -Wall -std=c++2a test.cc opcode.cc data_writer.cc write_buffer.cc base_meta_writer.cc -O2 -DNDEBUG -march=native -mtune=native && ./a.out
1: 0
2: 1
3: 4
4: 6
9: 89
12: 1
13: 1
14: 1
15.1: 1
15.2: 1
15.15: 67
15.21: 8
15.22: 1
15.23: 1
17: 1
18: 10
67: 4
100: 1
Meta stream
        OneField[1, 0]
        FieldN[3, 0] 6
        SkipN[3, 1] 1
        FieldN[0, 1] 89
        OneField[2, 0]
        TwoFields[0, 0]
        OpenField[1, 0]
        OneField[0, 0]
        SkipN[3, 1] 9
        FieldN[0, 1] 67
        SkipN[3, 1] 2
        TwoFields[3, 0]
        FieldClose[1, 1] 79
        OneField[1, 0]
        FieldN[0, 0] 10
        SkipN[3, 1] 45
        OneField[0, 2]
        SkipN[3, 1] 29
        FieldClose[1, 1] 70 1
Data: 198; meta: 31
144.362 ns/iter
```
