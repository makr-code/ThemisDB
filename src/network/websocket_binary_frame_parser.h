#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace themis::network::detail {

inline constexpr std::size_t kWebSocketWireHeaderSize = 12;
inline constexpr uint8_t kWebSocketWireMagic[4] = {0x54, 0x4D, 0x44, 0x42};
inline constexpr uint16_t kWebSocketSkipChecksumFlag = 0x0004;
inline constexpr uint32_t kWebSocketMaxPayloadBytes = 64u * 1024u * 1024u;

struct WebSocketBinaryFrame {
    uint8_t opcode = 0;
    uint32_t payload_size = 0;
};

struct WebSocketBinaryFrameParseResult {
    bool valid = false;
    WebSocketBinaryFrame frame;
    uint32_t error_code = 0;
    const char* error_message = nullptr;
};

WebSocketBinaryFrameParseResult parseWebSocketBinaryFrame(
    const std::vector<uint8_t>& data);

}  // namespace themis::network::detail
