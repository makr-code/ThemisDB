// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file hash_util.h
 * @brief Canonical constexpr FNV-1a hash primitives for ThemisDB.
 *
 * Single Source of Truth for FNV-1a hashing.  All modules should use these
 * functions instead of maintaining their own local copies.
 *
 * Supersedes duplicate implementations in (non-exhaustive list):
 *   - src/server/cdn_cache_middleware.cpp   (file-static fnv1a64)
 *   - src/index/index_compression.cpp       (file-static fnv1a64)
 *   - src/prompt_engineering/prompt_library_io.cpp (file-static fnv1a64)
 *   - src/query/runtime_reoptimizer.cpp     (file-static fnv1a_hex)
 *   - src/query/query_compiler.cpp          (file-static fnv1a64)
 *   - src/importers/mysql_importer.cpp      (file-static mysql_fnv1a64)
 *   - src/importers/mdm_audit_trail.cpp     (file-static fnv1a64)
 *   - src/core/concerns/redis_cache.cpp     (method fnv1a32)
 *   - src/utils/consistent_hash.cpp         (method fnv1a64)
 *   - include/auth/token_blacklist.h        (inline FNV-1a)
 *   - include/themis/gpu/graph_cache.h      (inline FNV-1a)
 *   - include/acceleration/cuda_backend.h   (inline FNV-1a hash structs)
 *
 * Usage:
 * @code
 *   #include "utils/hash_util.h"
 *
 *   // Compile-time hash of a string literal:
 *   constexpr uint64_t h = themis::hash::fnv1a64("hello");
 *
 *   // Runtime hash of arbitrary bytes:
 *   uint64_t h2 = themis::hash::fnv1a64(ptr, len);
 *
 *   // Hex string (16 lowercase hex chars, useful for cache keys):
 *   std::string key = themis::hash::fnv1a64_hex(query_text);
 * @endcode
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
    return fnv1a32(reinterpret_cast<const char*>(data), len);
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
    return fnv1a64(reinterpret_cast<const char*>(data), len);
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
    for (int i = 15; i >= 0; --i) {
        out[static_cast<std::size_t>(i)] = kHex[h >> (static_cast<unsigned>(i) * 4) & 0xFU];
    }
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

} // namespace hash
} // namespace themis
