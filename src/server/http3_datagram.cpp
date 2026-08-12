/**
 * @file http3_datagram.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – HTTP/3 Datagram Support
// See include/server/http3_datagram.h for design documentation.

#ifdef THEMIS_ENABLE_HTTP3

#include "server/http3_datagram.h"
#include "utils/logger.h"

#include <cstring>
#include <stdexcept>

namespace themis {
namespace server {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

Http3DatagramDispatcher::Http3DatagramDispatcher(const Http3DatagramConfig& config)
    : config_(config)
{}

// ─────────────────────────────────────────────────────────────────────────────
// Context Management
// ─────────────────────────────────────────────────────────────────────────────

bool Http3DatagramDispatcher::registerContext(uint64_t       context_id,
                                              DatagramHandler handler) {
    if (!config_.enable) {
        THEMIS_WARN("[Http3Datagram] registerContext({}) rejected: datagrams disabled",
                    context_id);
        return false;
    }

    std::lock_guard<std::mutex> lk(contexts_mutex_);
    contexts_[context_id] = Http3DatagramContext{context_id, std::move(handler), true};
    THEMIS_INFO("[Http3Datagram] registered context_id={}", context_id);
    return true;
}

bool Http3DatagramDispatcher::unregisterContext(uint64_t context_id) {
    std::lock_guard<std::mutex> lk(contexts_mutex_);
    auto it = contexts_.find(context_id);
    if (it == contexts_.end()) {
        return false;
    }
    contexts_.erase(it);
    THEMIS_INFO("[Http3Datagram] unregistered context_id={}", context_id);
    return true;
}

bool Http3DatagramDispatcher::hasContext(uint64_t context_id) const {
    std::lock_guard<std::mutex> lk(contexts_mutex_);
    return contexts_.count(context_id) > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Receive Path
// ─────────────────────────────────────────────────────────────────────────────

void Http3DatagramDispatcher::dispatch(const uint8_t* data, size_t len) {
    {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++stats_.datagrams_received;
    }

    if (!config_.enable || !data || len == 0) {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++stats_.datagrams_dropped;
        return;
    }

    // Decode Quarter Stream ID (RFC 9297 §2).
    uint64_t context_id = 0;
    size_t consumed = decodeVarint(data, len, context_id);
    if (consumed == 0 || consumed > len) {
        THEMIS_WARN("[Http3Datagram] malformed datagram: varint decode failed "
                    "(len={})", len);
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++stats_.datagrams_dropped;
        return;
    }

    const uint8_t* payload    = data + consumed;
    const size_t   payload_len = len - consumed;

    // Look up and invoke handler.
    DatagramHandler handler;
    {
        std::lock_guard<std::mutex> lk(contexts_mutex_);
        auto it = contexts_.find(context_id);
        if (it == contexts_.end() || !it->second.active) {
            std::lock_guard<std::mutex> slk(stats_mutex_);
            ++stats_.datagrams_dropped;
            THEMIS_WARN("[Http3Datagram] no handler for context_id={}", context_id);
            return;
        }
        handler = it->second.handler;
    }

    handler(context_id, payload, payload_len);

    std::lock_guard<std::mutex> slk(stats_mutex_);
    ++stats_.datagrams_dispatched;
}

// ─────────────────────────────────────────────────────────────────────────────
// Send Path
// ─────────────────────────────────────────────────────────────────────────────

/* static */
std::vector<uint8_t> Http3DatagramDispatcher::encode(uint64_t       context_id,
                                                      const uint8_t* payload,
                                                      size_t         paylen) {
    // Reserve 8 bytes for the varint prefix + payload.
    std::vector<uint8_t> frame;
    frame.resize(8 + paylen);

    uint8_t varint_buf[8];
    size_t varint_len = encodeVarint(context_id, varint_buf);
    if (varint_len == 0) {
        // context_id out of range for QUIC varint (>= 2^62)
        return {};
    }

    std::memcpy(frame.data(), varint_buf, varint_len);
    if (paylen > 0 && payload) {
        std::memcpy(frame.data() + varint_len, payload, paylen);
    }
    frame.resize(varint_len + paylen);
    return frame;
}

void Http3DatagramDispatcher::recordSent() {
    std::lock_guard<std::mutex> slk(stats_mutex_);
    ++stats_.datagrams_sent;
}

// ─────────────────────────────────────────────────────────────────────────────
// QUIC Variable-Length Integer Codec (RFC 9000 §16)
// ─────────────────────────────────────────────────────────────────────────────
//
// Encoding:
//   2-bit prefix | value
//   00xxxxxx                               1-byte  (0   – 63)
//   01xxxxxx xxxxxxxx                      2-byte  (64  – 16383)
//   10xxxxxx xxxxxxxx xxxxxxxx xxxxxxxx    4-byte  (16384 – 1073741823)
//   11xxxxxx … (8 bytes)                   8-byte  (1073741824 – 2^62-1)

/* static */
size_t Http3DatagramDispatcher::encodeVarint(uint64_t value, uint8_t* buf) {
    // RFC 9000 §16: max 2^62 - 1
    if (value <= 63ULL) {                           // 6-bit range
        buf[0] = static_cast<uint8_t>(value);       // prefix 00
        return 1;
    }
    if (value <= 16383ULL) {                        // 14-bit range
        buf[0] = static_cast<uint8_t>(0x40 | (value >> 8));
        buf[1] = static_cast<uint8_t>(value & 0xFF);
        return 2;
    }
    if (value <= 1073741823ULL) {                   // 30-bit range
        buf[0] = static_cast<uint8_t>(0x80 | (value >> 24));
        buf[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        buf[2] = static_cast<uint8_t>((value >>  8) & 0xFF);
        buf[3] = static_cast<uint8_t>(value & 0xFF);
        return 4;
    }
    if (value <= 4611686018427387903ULL) {          // 62-bit range
        buf[0] = static_cast<uint8_t>(0xC0 | (value >> 56));
        buf[1] = static_cast<uint8_t>((value >> 48) & 0xFF);
        buf[2] = static_cast<uint8_t>((value >> 40) & 0xFF);
        buf[3] = static_cast<uint8_t>((value >> 32) & 0xFF);
        buf[4] = static_cast<uint8_t>((value >> 24) & 0xFF);
        buf[5] = static_cast<uint8_t>((value >> 16) & 0xFF);
        buf[6] = static_cast<uint8_t>((value >>  8) & 0xFF);
        buf[7] = static_cast<uint8_t>(value & 0xFF);
        return 8;
    }
    // Value exceeds 2^62 - 1: not representable.
    return 0;
}

/* static */
size_t Http3DatagramDispatcher::decodeVarint(const uint8_t* data, size_t len,
                                              uint64_t& value_out) {
    if (len == 0) {
        return 0;
    }

    const uint8_t prefix = data[0] >> 6;  // Top 2 bits indicate encoding width.

    switch (prefix) {
    case 0: {  // 1-byte
        value_out = static_cast<uint64_t>(data[0] & 0x3F);
        return 1;
    }
    case 1: {  // 2-byte
        if (len < 2) { return 0; }
        value_out = (static_cast<uint64_t>(data[0] & 0x3F) << 8) |
                     static_cast<uint64_t>(data[1]);
        return 2;
    }
    case 2: {  // 4-byte
        if (len < 4) { return 0; }
        value_out = (static_cast<uint64_t>(data[0] & 0x3F) << 24) |
                    (static_cast<uint64_t>(data[1])          << 16) |
                    (static_cast<uint64_t>(data[2])          <<  8) |
                     static_cast<uint64_t>(data[3]);
        return 4;
    }
    case 3: {  // 8-byte
        if (len < 8) { return 0; }
        value_out = (static_cast<uint64_t>(data[0] & 0x3F) << 56) |
                    (static_cast<uint64_t>(data[1])          << 48) |
                    (static_cast<uint64_t>(data[2])          << 40) |
                    (static_cast<uint64_t>(data[3])          << 32) |
                    (static_cast<uint64_t>(data[4])          << 24) |
                    (static_cast<uint64_t>(data[5])          << 16) |
                    (static_cast<uint64_t>(data[6])          <<  8) |
                     static_cast<uint64_t>(data[7]);
        return 8;
    }
    default:
        return 0;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

Http3DatagramDispatcher::Stats Http3DatagramDispatcher::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

}  // namespace server
}  // namespace themis

#endif  // THEMIS_ENABLE_HTTP3
