/**
 * @file network_audit_log.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "network/network_audit_log.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Minimal portable SHA-256 implementation (no external dependencies)
// Follows FIPS 180-4; used only for token hashing on the audit path.
// ---------------------------------------------------------------------------
namespace {

static constexpr std::array<uint32_t, 64> kK = {{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
}};

inline uint32_t rotr32(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32u - n));
}

// Returns 32-byte raw SHA-256 digest of `data` (length `len`).
std::array<uint8_t, 32> sha256(const uint8_t* data, size_t len) {
    // Initial hash values
    uint32_t h[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };

    // Pre-processing: padding
    const uint64_t bit_len = static_cast<uint64_t>(len) * 8u;
    size_t padded_len = len + 1;
    while (padded_len % 64 != 56) {
      ++padded_len;
    }
    padded_len += 8;

    std::vector<uint8_t> msg(padded_len, 0);
    std::copy(data, data + len, msg.begin());
    msg[len] = 0x80u;
    // Big-endian bit length at the end
    for (int i = 0; i < 8; ++i) {
        msg[padded_len - 8 + i] = static_cast<uint8_t>(bit_len >> (56u - 8u * i));
    }

    // Process each 512-bit (64-byte) block
    for (size_t off = 0; off < padded_len; off += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(msg[off + 4*i    ]) << 24u)
                 | (static_cast<uint32_t>(msg[off + 4*i + 1]) << 16u)
                 | (static_cast<uint32_t>(msg[off + 4*i + 2]) <<  8u)
                 |  static_cast<uint32_t>(msg[off + 4*i + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >> 3u);
            const uint32_t s1 = rotr32(w[i- 2], 17) ^ rotr32(w[i- 2], 19) ^ (w[i- 2] >> 10u);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }

        uint32_t a=h[0], b=h[1], c=h[2], d=h[3],
                 e=h[4], f=h[5], g=h[6], hh=h[7];

        for (int i = 0; i < 64; ++i) {
            const uint32_t S1  = rotr32(e,6) ^ rotr32(e,11) ^ rotr32(e,25);
            const uint32_t ch  = (e & f) ^ (~e & g);
            const uint32_t tmp1 = hh + S1 + ch + kK[i] + w[i];
            const uint32_t S0  = rotr32(a,2) ^ rotr32(a,13) ^ rotr32(a,22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t tmp2 = S0 + maj;
            hh = g; g = f; f = e; e = d + tmp1;
            d  = c; c = b; b = a; a = tmp1 + tmp2;
        }

        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;
        h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }

    std::array<uint8_t, 32> digest{};
    for (int i = 0; i < 8; ++i) {
        digest[4*i    ] = static_cast<uint8_t>(h[i] >> 24u);
        digest[4*i + 1] = static_cast<uint8_t>(h[i] >> 16u);
        digest[4*i + 2] = static_cast<uint8_t>(h[i] >>  8u);
        digest[4*i + 3] = static_cast<uint8_t>(h[i]);
    }
    return digest;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// NetworkAuditLog
// ---------------------------------------------------------------------------

namespace themis {
namespace network {

NetworkAuditLog::NetworkAuditLog(const Config& config)
    : config_(config)
{
    if (config_.max_entries == 0) {
        throw std::invalid_argument(
            "NetworkAuditLog: max_entries must be >= 1");
    }
}

// ---------------------------------------------------------------------------
// Callback
// ---------------------------------------------------------------------------

void NetworkAuditLog::setEventCallback(
    std::function<void(const AuditEvent&)> cb)
{
    std::lock_guard<std::mutex> lk(mutex_);
    callback_ = std::move([[maybe_unused]] cb);
}

// ---------------------------------------------------------------------------
// Recording
// ---------------------------------------------------------------------------

void NetworkAuditLog::record([[maybe_unused]] AuditEvent event) {
    // Fill in timestamp if caller left it default-constructed (epoch).
    if ([[maybe_unused]] event.timestamp == std::chrono::system_clock::time_point{}) {
        event.timestamp = std::chrono::system_clock::now();
    }

    std::function<void(const AuditEvent&)> cb_copy;
    {
        std::lock_guard<std::mutex> lk(mutex_);

        if (buffer_.size() >= config_.max_entries) {
            buffer_.pop_front();
            ++total_evicted_;
        }
        buffer_.push_back([[maybe_unused]] event);
        ++total_recorded_;
        updateCounters([[maybe_unused]] event.type);

        if ([[maybe_unused]] config_.enable_callback) {
            cb_copy = callback_;
        }
    }

    // Invoke callback outside the lock to avoid priority inversion.
    if (cb_copy) {
        cb_copy([[maybe_unused]] event);
    }
}

void NetworkAuditLog::recordConnectionOpen(const std::string& remote_address,
                                           uint64_t connection_id)
{
    AuditEvent ev;
    ev.type           = AuditEventType::CONNECTION_OPEN;
    ev.timestamp      = std::chrono::system_clock::now();
    ev.remote_address = remote_address;
    ev.connection_id  = connection_id;
    record(std::move(ev));
}

void NetworkAuditLog::recordConnectionClose(const std::string& remote_address,
                                            uint64_t connection_id,
                                            const std::string& reason)
{
    AuditEvent ev;
    ev.type           = AuditEventType::CONNECTION_CLOSE;
    ev.timestamp      = std::chrono::system_clock::now();
    ev.remote_address = remote_address;
    ev.connection_id  = connection_id;
    ev.detail         = reason;
    record(std::move(ev));
}

void NetworkAuditLog::recordAuth(bool success,
                                 const std::string& remote_address,
                                 uint64_t connection_id,
                                 const std::string& principal,
                                 const std::string& token)
{
    AuditEvent ev;
    ev.type           = success ? AuditEventType::AUTH_SUCCESS
                                : AuditEventType::AUTH_FAILURE;
    ev.timestamp      = std::chrono::system_clock::now();
    ev.remote_address = remote_address;
    ev.connection_id  = connection_id;
    ev.principal      = principal;
    ev.token_hash     = truncatedSha256Hex(token);
    record(std::move(ev));
}

void NetworkAuditLog::recordRateLimited(const std::string& remote_address,
                                        uint64_t connection_id,
                                        const std::string& rule)
{
    AuditEvent ev;
    ev.type           = AuditEventType::RATE_LIMITED;
    ev.timestamp      = std::chrono::system_clock::now();
    ev.remote_address = remote_address;
    ev.connection_id  = connection_id;
    ev.detail         = rule;
    record(std::move(ev));
}

// ---------------------------------------------------------------------------
// Query
// ---------------------------------------------------------------------------

std::vector<AuditEvent> NetworkAuditLog::getRecentEvents([[maybe_unused]] size_t n) const {
    std::lock_guard<std::mutex> lk(mutex_);
    if (n == 0 || n >= buffer_.size()) {
        return std::vector<AuditEvent>([[maybe_unused]] buffer_.begin(), buffer_.end());
    }
    const size_t skip = buffer_.size() - n;
    return std::vector<AuditEvent>(buffer_.begin() + static_cast<std::ptrdiff_t>(skip),
                                   buffer_.end());
}

std::vector<AuditEvent> NetworkAuditLog::getEventsByType([[maybe_unused]] AuditEventType type) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<AuditEvent> result;
    for (const auto& ev : buffer_) {
        if (ev.type == type) {
          result.push_back(ev);
        }
    }
    return result;
}

NetworkAuditLog::Stats NetworkAuditLog::getStats() const {
    std::lock_guard<std::mutex> lk(mutex_);
    Stats s;
    s.total_recorded    = total_recorded_;
    s.total_evicted     = total_evicted_;
    s.connection_opens  = connection_opens_;
    s.connection_closes = connection_closes_;
    s.auth_successes    = auth_successes_;
    s.auth_failures     = auth_failures_;
    s.rate_limited      = rate_limited_;
    s.current_size      = buffer_.size();
    return s;
}

void NetworkAuditLog::clear() {
    std::lock_guard<std::mutex> lk(mutex_);
    buffer_.clear();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string NetworkAuditLog::truncatedSha256Hex(const std::string& input) {
    if (input.empty()) return {};

    const auto digest = sha256(
        reinterpret_cast<const uint8_t*>(input.data()), input.size());

    // Encode first 8 bytes as 16 lower-case hex characters.
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) {
        oss << std::setw(2) << static_cast<unsigned>(digest[i]);
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void NetworkAuditLog::updateCounters([[maybe_unused]] AuditEventType type) {
    switch (type) {
    case AuditEventType::CONNECTION_OPEN:  ++connection_opens_;  break;
    case AuditEventType::CONNECTION_CLOSE: ++connection_closes_; break;
    case AuditEventType::AUTH_SUCCESS:     ++auth_successes_;    break;
    case AuditEventType::AUTH_FAILURE:     ++auth_failures_;     break;
    case AuditEventType::RATE_LIMITED:     ++rate_limited_;      break;
    }
}

} // namespace network
} // namespace themis
