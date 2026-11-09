#include "websocket_binary_frame_parser.h"

#include <array>

namespace themis::network::detail {
namespace {

constexpr std::array<uint32_t, 256> makeCrc32Table() {
    std::array<uint32_t, 256> table{};
    for (uint32_t index = 0; index < table.size(); ++index) {
        uint32_t value = index;
        for (int bit = 0; bit < 8; ++bit) {
            value = (value & 1u) ? (value >> 1) ^ 0xEDB88320u : value >> 1;
        }
        table[index] = value;
    }
    return table;
}

constexpr auto kCrc32Table = makeCrc32Table();

uint32_t crc32Update(uint32_t crc, const uint8_t* data, std::size_t size) {
    crc = ~crc;
    for (std::size_t i = 0; i < size; ++i) {
        crc = kCrc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

WebSocketBinaryFrameParseResult invalid(uint32_t code, const char* message) {
    return {false, {}, code, message};
}

}  // namespace

WebSocketBinaryFrameParseResult parseWebSocketBinaryFrame(
    const std::vector<uint8_t>& data) {
    if (data.size() < kWebSocketWireHeaderSize) {
        return invalid(0x0008u, "Binary frame too short (minimum 12-byte header required)");
    }
    for (std::size_t i = 0; i < 4; ++i) {
        if (data[i] != kWebSocketWireMagic[i]) {
            return invalid(0x0009u, "Invalid magic bytes: expected TMDB");
        }
    }

    const uint16_t flags =
        (static_cast<uint16_t>(data[6]) << 8) | data[7];
    const uint32_t payload_size =
        (static_cast<uint32_t>(data[8]) << 24) |
        (static_cast<uint32_t>(data[9]) << 16) |
        (static_cast<uint32_t>(data[10]) << 8) |
        static_cast<uint32_t>(data[11]);
    if (payload_size > kWebSocketMaxPayloadBytes) {
        return invalid(0x0001u, "Payload size exceeds maximum allowed");
    }

    const bool has_checksum = !(flags & kWebSocketSkipChecksumFlag);
    const std::size_t expected_size =
        kWebSocketWireHeaderSize + payload_size + (has_checksum ? 4 : 0);
    if (data.size() < expected_size) {
        return invalid(0x000Au, "Binary frame incomplete");
    }
    if (has_checksum) {
        const std::size_t crc_offset = kWebSocketWireHeaderSize + payload_size;
        const uint32_t expected_crc =
            (static_cast<uint32_t>(data[crc_offset]) << 24) |
            (static_cast<uint32_t>(data[crc_offset + 1]) << 16) |
            (static_cast<uint32_t>(data[crc_offset + 2]) << 8) |
            static_cast<uint32_t>(data[crc_offset + 3]);
        uint32_t computed_crc = crc32Update(0, data.data(), kWebSocketWireHeaderSize);
        if (payload_size > 0) {
            computed_crc = crc32Update(
                computed_crc, data.data() + kWebSocketWireHeaderSize, payload_size);
        }
        if (computed_crc != expected_crc) {
            return invalid(0x000Fu, "Checksum mismatch");
        }
    }

    return {true, {data[5], payload_size}, 0, nullptr};
}

}  // namespace themis::network::detail
