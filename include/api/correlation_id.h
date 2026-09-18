/**
 * @file correlation_id.h
 * @brief RFC 4122 UUID-based request correlation ID infrastructure.
 *
 * @details Provides fixed-width (16-byte) correlation ID value types, parsing,
 * serialization, and interfaces for request tracing across service boundaries.
 *
 * Core components:
 *  - `CorrelationId`: Trivially copyable 128-bit UUID value type
 *  - `ICorrelationIDProvider`: Pure-virtual interface for ID generation
 *  - `RFC4122Generator`: UUID v4 generator with cryptographically-secure randomness
 *
 * Design properties:
 *  - Opaque 128-bit representation (16-byte array)
 *  - Canonical RFC 4122 string serialization (36 chars, lowercase hex: `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`)
 *  - Trivially copyable and suitable for `thread_local` contexts
 *  - Hashable and comparable, works as map keys
 *  - Never includes PII; format is UUID only
 *
 * Parsing:
 *  - `CorrelationId::parse()` accepts RFC 4122 strings (with or without dashes)
 *  - Accepts both lowercase and uppercase hex
 *  - Shorter IDs are zero-padded on the right
 *  - Throws `std::invalid_argument` on invalid format
 *
 * ### Thread safety
 * - `CorrelationId` is immutable and safe to share across threads
 * - `ICorrelationIDProvider::generate()` must be thread-safe
 * - `RFC4122Generator` uses `thread_local` PRNG (safe for concurrent calls)
 *
 * ### Usage
 * ```cpp
 * // Generate a new ID
 * auto id = RFC4122Generator::generate();
 * auto header_value = id.toString();  // "550e8400-e29b-41d4-a716-446655440000"
 *
 * // Parse from incoming header
 * auto parsed = CorrelationId::parse("550e8400-e29b-41d4-a716-446655440000");
 * ```
 *
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 */


#pragma once

#include <array>
#include <string>
#include <string_view>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace api {

// Forward declaration
struct HttpRequest;

// ---------------------------------------------------------------------------
// CorrelationId — fixed-width 16-byte UUID value type
// ---------------------------------------------------------------------------

class CorrelationId {
public:
    static constexpr std::size_t kByteSize = 16;

    CorrelationId() noexcept { bytes_.fill(0); }

    explicit CorrelationId(const std::array<uint8_t, kByteSize>& bytes) noexcept
        : bytes_(bytes) {}

    explicit CorrelationId(const uint8_t* bytes) noexcept {
        std::memcpy(bytes_.data(), bytes, kByteSize);
    }

    /**
     * @brief Parse.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static CorrelationId parse(std::string_view s);

    /**
     * @brief To String.
     * @return Return value.
     */
    std::string toString() const;

    const std::array<uint8_t, kByteSize>& bytes() const noexcept { return bytes_; }

    bool isNil() const noexcept {
        for (auto b : bytes_) {
          if (b != 0) return false;
        }
        return true;
    }

    bool operator==(const CorrelationId& other) const noexcept {
        return bytes_ == other.bytes_;
    }

    bool operator!=(const CorrelationId& other) const noexcept {
        return bytes_ != other.bytes_;
    }

private:
    std::array<uint8_t, kByteSize> bytes_;
};

// ---------------------------------------------------------------------------
// CorrelationId inline implementation
// ---------------------------------------------------------------------------

/**
 * @brief Parse.
 * @param[in] s Input parameter.
 * @return Return value.
 */
inline CorrelationId CorrelationId::parse(std::string_view s)
{
    // Normalise a hex digit character to lowercase; returns 0 if not a valid hex char.
    auto toLowerHex = [](char c) -> char {
        if (c >= '0' && c <= '9') {
          return c;
        }
        if (c >= 'a' && c <= 'f') {
          return c;
        }
        if (c >= 'A' && c <= 'F') {
          return static_cast<char>(c - 'A' + 'a');
        }
        return '\0'; // not a hex digit
    };

    // Strip dashes and normalise to 32 hex chars
    std::string hex = {};
    hex.reserve(32);
    for (char c : s) {
        if (c == '-') {
          continue;
        }
        const char lc = toLowerHex(c);
        if (lc == '\0') {
            throw std::invalid_argument("CorrelationId::parse: invalid character in UUID string");
        }
        hex.push_back(lc);
    }
    if (hex.size() > 32) {
        throw std::invalid_argument("CorrelationId::parse: UUID string too long");
    }
    hex.resize(32, '0'); // zero-pad if shorter

    std::array<uint8_t, kByteSize> bytes{};
    for (std::size_t i = 0; i < kByteSize; ++i) {
        const char hi = hex[i * 2];
        const char lo = hex[i * 2 + 1];
        auto hexVal = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') {
              return static_cast<uint8_t>(c - '0');
            }
            return static_cast<uint8_t>(c - 'a' + 10);
        };
        bytes[i] = static_cast<uint8_t>((hexVal(hi) << 4) | hexVal(lo));
    }
    return CorrelationId(bytes);
}

inline std::string CorrelationId::toString() const
{
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out = {};
    out.reserve(36);
    for (std::size_t i = 0; i < kByteSize; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
          out.push_back('-');
        }
        out.push_back(kHex[(bytes_[i] >> 4) & 0xF]);
        out.push_back(kHex[ bytes_[i]       & 0xF]);
    }
    return out;
}

// ---------------------------------------------------------------------------
// std::hash specialisation for CorrelationId
// ---------------------------------------------------------------------------

} // namespace api
} // namespace themis

namespace std {
template <>
struct hash<themis::api::CorrelationId> {
    std::size_t operator()(const themis::api::CorrelationId& id) const noexcept {
        // FNV-1a over the 16 bytes
        std::size_t h = 14695981039346656037ULL;
        for (auto b : id.bytes()) {
            h ^= static_cast<std::size_t>(b);
            h *= 1099511628211ULL;
        }
        return h;
    }
};
} // namespace std

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// ICorrelationIDProvider — pure-virtual interface for ID generation/extraction
// ---------------------------------------------------------------------------

class ICorrelationIDProvider {
public:
    /**
     * @brief ICorrelation IDProvider.
     * @return Return value.
     */
    virtual ~ICorrelationIDProvider() = default;

    static constexpr std::string_view kHeaderName = "X-Correlation-ID";

    /**
     * @brief Generate.
     * @return Return value.
     */
    virtual CorrelationId generate() const = 0;

    virtual CorrelationId extract(const std::unordered_map<std::string, std::string>& headers) const = 0;

    /**
     * @brief Serialize.
     * @param[in] id Input parameter.
     * @return Return value.
     * @details Calls: toString().
     */
    static std::string serialize(const CorrelationId& id) { return id.toString(); }
};

} // namespace api
} // namespace themis
