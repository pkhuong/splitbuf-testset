Serialisation/deserialisation benchmark data
============================================

It's not much, but it's better than nothing: these files are JSON
representations of the two in-repo benchmark datafiles in the protobuf
repo.  The protobuf schemas and the protobuf-encoded payloads were
extracted from Google's protobuf repository (see LICENSE.protobuf),
and the `.pbtxt` data converted to JSON by hand.

`message1.json` is the JSON equivalent of
`google_message1_proto2`'s protobuf bytes, in `protobuf/benchmarks`:

```
1: string
2: int32
3: int32
4: string
9: string
12: bool
13: bool
14: bool
15: {
  1: int32
  2: int32
  15: string
  21: fixed64
  22: int32
  23: bool
}
17: bool
18: string
67: int32
100: int32
```

`message2.json` corresponds to `google_message2`, again in
`protobuf/benchmarks`:

```
2: bytes
3: int64
4: int64
5: {
   5: int32
  11: float 
  12: string
  15: uint64
  31: {
    1: float
    3: float
    4: bool
    5: bool
    6: bool
    7: bool
    8: float
  }
  73: int32
}
21: int32
25: float
71: int32
129: int32
205: bool
206: bool
```

On my machine, C++ proto uses ~200 ns/message to parse message1 (into a
pre-allocated message), and ~120 ns/message to serialise message1.  It
needs ~116000 ns/message to parse message2, and ~85000 ns/message to
serialise it.

```
pkhuong@penguin:~/protobuf/benchmarks$ make cpp
./cpp-benchmark $(find $(cd . && pwd) -type f -name "dataset.*.pb" -not -path "$(cd . && pwd)/tmp/*")
Run on (2 X 1608.01 MHz CPU s)
2021-11-05 15:33:17
-----------------------------------------------------------------------------
Benchmark                                      Time           CPU Iterations
-----------------------------------------------------------------------------
google_message1_proto2_parse_new             357 ns        357 ns    2012371   609.041MB/s
google_message1_proto2_parse_reuse           195 ns        195 ns    3615240   1116.17MB/s
google_message1_proto2_parse_newarena        433 ns        433 ns    1637460   502.076MB/s
google_message1_proto2_serialize             120 ns        120 ns    6549858   1.77582GB/s
google_message1_proto3_parse_new             600 ns        599 ns    1117058   362.775MB/s
google_message1_proto3_parse_reuse           429 ns        429 ns    1607172    506.84MB/s
google_message1_proto3_parse_newarena        648 ns        647 ns    1074650   336.048MB/s
google_message1_proto3_serialize             216 ns        216 ns    3125955   976.913MB/s
google_message2_parse_new                 363927 ns     363652 ns       1892   221.784MB/s
google_message2_parse_reuse               116172 ns     116063 ns       5769   694.898MB/s
google_message2_parse_newarena            225815 ns     225683 ns       3000    357.37MB/s
google_message2_serialize                  84675 ns      84620 ns       8146   953.111MB/s
```
