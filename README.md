# tink-cpp

tink data-flow node frame protocol — C++17 header-only library (no
dependencies). Universal and language-agnostic: any component that obeys the
frame protocol can join a tink pipeline.

```
帧 = [ len: u32 BE ][ payload: len 字节 ][ crc: u32 BE ]
len = payload 字节数
crc = CRC32-IEEE(payload)（多项式 0xEDB88320）
```

Mirrors `ext/tink.tie` (tie standard library) and the Rust / C / Python / JS
tink libraries; pure functions over byte buffers, IO (stdin/stdout) left to the
caller.

## API (`namespace tink`)

| function | description |
| --- | --- |
| `crc32(data: span) -> uint32_t` | CRC32-IEEE over a byte buffer. Check vector: `crc32("123456789", 9) == 0xCBF43926` |
| `frame_encode(payload, len) -> vector<uint8_t>` | encode a payload into a full frame `[len][payload][crc]` |
| `frame_next(bytes, pos) -> optional<Frame>` | parse one frame at `pos`, verify CRC; `Frame{payload, next}` on success, `nullopt` on out-of-bounds / mismatch |
| `frame_skip(bytes, pos) -> optional<size_t>` | skip one frame at `pos` without copying or verifying; `nullopt` on out-of-bounds |

Overloads taking `std::vector<uint8_t>` are provided for both `crc32` and
`frame_encode`. Frame payloads are copied into the returned `Frame`.

## Usage

```cpp
#include "tink.hpp"

auto frame = tink::frame_encode({1, 2, 3});
auto got = tink::frame_next(frame, 0); // std::optional<tink::Frame>
if (got) {
    // got->payload == {1, 2, 3}, got->next == frame.size()
}
```

## Build & test

```bash
clang++ -std=c++17 -Wall -Wextra test_tink.cpp -o test_tink && ./test_tink
```

## Cross-language

tink 帧协议各语言实现（API 语义与校验向量一致）：

| language | library |
| --- | --- |
| tie | `ext/tink.tie` |
| Rust | `tink-rust`（tink crate） |
| C | `tink-c`（`tink.h` + `tink.c`） |
| Python | `tink-python`（`tink.py`） |
| JavaScript | `tink-js`（`tink.js` + `tink.d.ts`） |
| C++ | this library（`tink-cpp`，`tink.hpp`） |

## License

本仓库使用 **Tie Public License v2.0 (TPL 2.0)**，完整文本见 [LICENSE](LICENSE)。
This repository is distributed under the **Tie Public License v2.0 (TPL 2.0)** — see [LICENSE](LICENSE) for the full text.