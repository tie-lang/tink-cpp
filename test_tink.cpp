// test_tink.cpp —— unit tests for tink.hpp (C++17). Run: clang++ -std=c++17 test_tink.cpp && ./a.out

#include <cassert>
#include <cstdio>
#include "tink.hpp"

using tink::crc32;
using tink::frame_encode;
using tink::frame_next;
using tink::frame_skip;

int main() {
    // crc32 check vector
    const char *s = "123456789";
    assert(crc32(reinterpret_cast<const uint8_t *>(s), 9) == 0xCBF43926u);

    // frame roundtrip
    std::vector<uint8_t> p{1, 2, 3};
    auto f = frame_encode(p);
    assert(f.size() == p.size() + 8);
    auto got = frame_next(f, 0);
    assert(got.has_value());
    assert(got->next == f.size());
    assert(got->payload == p);

    // empty frame roundtrip
    auto fe = frame_encode(std::vector<uint8_t>{});
    auto ge = frame_next(fe, 0);
    assert(ge.has_value());
    assert(ge->next == fe.size());
    assert(ge->payload.empty());

    // CRC tamper rejected
    auto ft = frame_encode(p);
    ft[5] += 1; // payload[1] tampered
    assert(!frame_next(ft, 0).has_value());

    // frame_skip matches length
    auto fs = frame_encode(p);
    assert(frame_skip(fs, 0).value_or(0) == fs.size());

    // out of bounds
    assert(!frame_next(f, f.size()).has_value());
    assert(!frame_skip(f, f.size()).has_value());

    std::puts("all tests passed");
    return 0;
}