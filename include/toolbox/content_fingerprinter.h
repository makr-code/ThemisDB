/**
 * @file content_fingerprinter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// ContentFingerprint
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Standardised content fingerprint produced by `ContentFingerprinter`.
 *
 * All fields are populated by `ContentFingerprinter::compute()`.
 */
struct ContentFingerprint {
    /// SHA-256 digest of the raw UTF-8 content as a 64-character lowercase
    /// hexadecimal string.  Empty only when the input is empty.
    std::string sha256_hex;

    /// Byte length of the original content (number of UTF-8 bytes / raw bytes).
    std::size_t byte_len = 0;

    /// Estimated token count using the default 4 chars-per-token heuristic.
    /// Consistent with `DocumentSplitter`'s token estimation.
    std::size_t token_estimate = 0;
    
    /// Latency of fingerprinting operation in microseconds (for observability).
    std::uint64_t latency_us = 0;

    /// @return `true` when @c sha256_hex is non-empty (i.e. input was
    ///         non-empty and fingerprinting succeeded).
    [[nodiscard]] bool valid() const noexcept { return !sha256_hex.empty(); }

    /// Equality is defined by the SHA-256 digest only.
    bool operator==(const ContentFingerprint& o) const noexcept {
        return sha256_hex == o.sha256_hex;
    }
    bool operator!=(const ContentFingerprint& o) const noexcept {
        return !(*this == o);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ContentFingerprinter
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Computes a `ContentFingerprint` from raw bytes or UTF-8 text.
 *
 * Internally uses OpenSSL `SHA256()` for the cryptographic digest.
 * Token estimation uses the same 4 chars-per-token heuristic as
 * `rag::DocumentSplitter`.
 *
 * Thread-safety: all methods are stateless; an instance may be shared across
 * threads.
 */
class ContentFingerprinter {
public:
    ContentFingerprinter()  = default;
    ~ContentFingerprinter() = default;

    ContentFingerprinter(const ContentFingerprinter&)            = default;
    ContentFingerprinter& operator=(const ContentFingerprinter&) = default;

    /**
     * @brief Compute a `ContentFingerprint` for the given UTF-8 text.
     *
     * @param text  UTF-8 text or raw byte content.
     * @return Populated fingerprint.  `sha256_hex` is empty only when @p text
     *         is empty.
     */
    ContentFingerprint compute(std::string_view text) const;

    /**
     * @brief Compute a `ContentFingerprint` for the given raw bytes.
     *
     * @param data Pointer to the first byte.
     * @param len  Number of bytes.
     * @return Populated fingerprint.
     */
    ContentFingerprint compute(const unsigned char* data, std::size_t len) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Compute a content fingerprint for @p text.
 *
 * Convenience free function.  Equivalent to
 * `ContentFingerprinter{}.compute(text)`.
 *
 * @param text UTF-8 text or raw bytes.
 * @return Populated `ContentFingerprint`.
 */

#pragma once
ContentFingerprint fingerprint(std::string_view text);

} // namespace toolbox
} // namespace themis
