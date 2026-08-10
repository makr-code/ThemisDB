/**
 * @file hash_util.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.7
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

namespace themis {
namespace hash {

// ---------------------------------------------------------------------------
// FNV-1a constants
// ---------------------------------------------------------------------------

/// FNV-1a 32-bit offset basis.
inline constexpr uint32_t kFnv32OffsetBasis = 2166136261U;
/// FNV-1a 32-bit prime.
inline constexpr uint32_t kFnv32Prime       = 16777619U;

/// FNV-1a 64-bit offset basis.
inline constexpr uint64_t kFnv64OffsetBasis = 14695981039346656037ULL;
/// FNV-1a 64-bit prime.
inline constexpr uint64_t kFnv64Prime       = 1099511628211ULL;

// ---------------------------------------------------------------------------
// FNV-1a 32-bit
// ---------------------------------------------------------------------------

/**
 * @brief Compute FNV-1a 32-bit hash of @p len bytes starting at @p data.
 *
 * constexpr-safe (C++14+).
 */
[[nodiscard]] constexpr uint32_t fnv1a32(const char* data, std::size_t len) noexcept {
    uint32_t h = kFnv32OffsetBasis;
    for (std::size_t i = 0; i < len; ++i) {
        h ^= static_cast<uint32_t>(static_cast<unsigned char>(data[i]));
        h *= kFnv32Prime;
    }
    return h;
}

/** @overload Accepts `const uint8_t*`. */
[[nodiscard]] constexpr uint32_t fnv1a32(const uint8_t* data, std::size_t len) noexcept {
    uint32_t h = kFnv32OffsetBasis;
    for (std::size_t i = 0; i < len; ++i) {
        h ^= static_cast<uint32_t>(data[i]);
        h *= kFnv32Prime;
    }
    return h;
}

/** @overload Accepts `std::string_view`. */
[[nodiscard]] constexpr uint32_t fnv1a32(std::string_view s) noexcept {
    return fnv1a32(s.data(), s.size());
}

/** @overload Accepts `const std::string&`. */
[[nodiscard]] inline uint32_t fnv1a32(const std::string& s) noexcept {
    return fnv1a32(s.data(), s.size());
}

// ---------------------------------------------------------------------------
// FNV-1a 64-bit
// ---------------------------------------------------------------------------

/**
 * @brief Compute FNV-1a 64-bit hash of @p len bytes starting at @p data.
 *
 * constexpr-safe (C++14+).
 */
[[nodiscard]] constexpr uint64_t fnv1a64(const char* data, std::size_t len) noexcept {
    uint64_t h = kFnv64OffsetBasis;
    for (std::size_t i = 0; i < len; ++i) {
        h ^= static_cast<uint64_t>(static_cast<unsigned char>(data[i]));
        h *= kFnv64Prime;
    }
    return h;
}

/** @overload Accepts `const uint8_t*`. */
[[nodiscard]] constexpr uint64_t fnv1a64(const uint8_t* data, std::size_t len) noexcept {
    uint64_t h = kFnv64OffsetBasis;
    for (std::size_t i = 0; i < len; ++i) {
        h ^= static_cast<uint64_t>(data[i]);
        h *= kFnv64Prime;
    }
    return h;
}

/** @overload Accepts `std::string_view`. */
[[nodiscard]] constexpr uint64_t fnv1a64(std::string_view s) noexcept {
    return fnv1a64(s.data(), s.size());
}

/** @overload Accepts `const std::string&`. */
[[nodiscard]] inline uint64_t fnv1a64(const std::string& s) noexcept {
    return fnv1a64(s.data(), s.size());
}

// ---------------------------------------------------------------------------
// Convenience: hex string
// ---------------------------------------------------------------------------

/**
 * @brief Return the FNV-1a 64-bit hash of @p s as a 16-character lowercase
 *        hexadecimal string.  Useful for cache keys, ETags, and checksums.
 */
[[nodiscard]] inline std::string fnv1a64_hex(std::string_view s) {
    constexpr char kHex[] = "0123456789abcdef";
    const uint64_t h = fnv1a64(s);
    std::string out(16, '0');
    // Write most-significant nibble first
    for (int i = 0; i < 16; ++i) {
        out[static_cast<std::size_t>(i)] = kHex[(h >> (60 - i * 4)) & 0xFU];
    }
    return out;
}

/**
 * @brief Return the FNV-1a 64-bit hash of @p s as a 16-character lowercase
 *        hexadecimal string.
 */
[[nodiscard]] inline std::string fnv1a64_hex(const std::string& s) {
    return fnv1a64_hex(std::string_view{s});
}

// ---------------------------------------------------------------------------
// SHA-256 hex encoding helper
// ---------------------------------------------------------------------------

/**
 * @brief Encode @p n raw bytes as a lowercase hexadecimal string.
 *
 * Intended for encoding SHA-256 (32 bytes → 64 chars) and similar digests.
 * Uses a nibble-lookup table; avoids the overhead of std::ostringstream.
 *
 * @param data  Pointer to the raw bytes.
 * @param n     Number of bytes to encode.
 * @return      Lowercase hex string of length 2*n.
 */
[[nodiscard]] inline std::string bytes_to_hex(const unsigned char* data, std::size_t n) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(n * 2, '\0');
    for (std::size_t i = 0; i < n; ++i) {
        out[2 * i]     = kHex[data[i] >> 4];
        out[2 * i + 1] = kHex[data[i] & 0x0f];
    }
    return out;
}

} // namespace hash
} // namespace themis
