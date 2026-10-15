// tink.hpp —— tink data-flow node frame protocol (universal, language-agnostic).
//
// Frame = [len u32 BE][payload][crc u32 BE]; crc = CRC32-IEEE (0xEDB88320).
// Mirrors ext/tink.tie (tie standard library) and the Rust / C / Python / JS
// tink libraries; pure functions over byte buffers, IO (stdin/stdout) left to
// the caller. Header-only, C++17, no dependencies.
//
//   auto frame = tink::frame_encode({1, 2, 3});
//   auto got = tink::frame_next(frame, 0);           // std::optional<Frame>
//   assert(got && got->payload == (std::vector<uint8_t>{1, 2, 3}));

#ifndef TINK_HPP
#define TINK_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace tink {

/// CRC32-IEEE over a byte range (bit-loop, no table; matches zlib.crc32).
/// Check vector: `crc32("123456789", 9) == 0xCBF43926`.
inline uint32_t crc32(const uint8_t *data, size_t len) {
    uint32_t crc = UINT32_C(0xFFFFFFFF);
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (UINT32_C(0xEDB88320) & (0u - (crc & 1u)));
    }
    return crc ^ UINT32_C(0xFFFFFFFF);
}

inline uint32_t crc32(const std::vector<uint8_t> &data) {
    return crc32(data.data(), data.size());
}

/// The result of parsing a frame: payload bytes plus the position right after
/// the frame (payload + 8). `payload` is a copy, memory owned by `Frame`.
struct Frame {
    std::vector<uint8_t> payload;
    size_t next;
};

/// Encode a payload into a full frame: `[len u32 BE][payload][crc u32 BE]`.
inline std::vector<uint8_t> frame_encode(const uint8_t *payload, size_t len) {
    std::vector<uint8_t> out;
    out.reserve(len + 8);
    uint32_t n = static_cast<uint32_t>(len);
    out.push_back(static_cast<uint8_t>(n >> 24));
    out.push_back(static_cast<uint8_t>(n >> 16));
    out.push_back(static_cast<uint8_t>(n >> 8));
    out.push_back(static_cast<uint8_t>(n));
    for (size_t i = 0; i < len; i++)
        out.push_back(payload[i]);
    uint32_t c = crc32(payload, len);
    out.push_back(static_cast<uint8_t>(c >> 24));
    out.push_back(static_cast<uint8_t>(c >> 16));
    out.push_back(static_cast<uint8_t>(c >> 8));
    out.push_back(static_cast<uint8_t>(c));
    return out;
}

inline std::vector<uint8_t> frame_encode(const std::vector<uint8_t> &payload) {
    return frame_encode(payload.data(), payload.size());
}

/// Parse one frame at `pos` (verifies CRC). Returns a `Frame` with a copy of
/// the payload and `next` = position after the frame; `std::nullopt` on
/// out-of-bounds or CRC mismatch.
inline std::optional<Frame> frame_next(const std::vector<uint8_t> &bytes,
                                       size_t pos) {
    if (bytes.size() < pos + 8)
        return std::nullopt;
    uint32_t n = (static_cast<uint32_t>(bytes[pos]) << 24) |
                 (static_cast<uint32_t>(bytes[pos + 1]) << 16) |
                 (static_cast<uint32_t>(bytes[pos + 2]) << 8) |
                 static_cast<uint32_t>(bytes[pos + 3]);
    size_t end = pos + 8 + static_cast<size_t>(n);
    if (bytes.size() < end)
        return std::nullopt;
    uint32_t want = (static_cast<uint32_t>(bytes[end - 4]) << 24) |
                    (static_cast<uint32_t>(bytes[end - 3]) << 16) |
                    (static_cast<uint32_t>(bytes[end - 2]) << 8) |
                    static_cast<uint32_t>(bytes[end - 1]);
    if (crc32(bytes.data() + pos + 4, n) != want)
        return std::nullopt;
    Frame out{{bytes.begin() + static_cast<ptrdiff_t>(pos + 4),
               bytes.begin() + static_cast<ptrdiff_t>(pos + 4 + n)},
              end};
    return out;
}

/// Skip one frame at `pos` without copying or verifying (zero-copy).
/// Returns `next` = position after the frame, or `std::nullopt` on
/// out-of-bounds.
inline std::optional<size_t> frame_skip(const std::vector<uint8_t> &bytes,
                                        size_t pos) {
    if (bytes.size() < pos + 8)
        return std::nullopt;
    uint32_t n = (static_cast<uint32_t>(bytes[pos]) << 24) |
                 (static_cast<uint32_t>(bytes[pos + 1]) << 16) |
                 (static_cast<uint32_t>(bytes[pos + 2]) << 8) |
                 static_cast<uint32_t>(bytes[pos + 3]);
    size_t end = pos + 8 + static_cast<size_t>(n);
    if (bytes.size() < end)
        return std::nullopt;
    return end;
}

} // namespace tink

#endif // TINK_HPP