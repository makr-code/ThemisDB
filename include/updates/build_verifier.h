/**
 * @file build_verifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.5
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>

namespace themis {
namespace updates {

/**
 * @brief Result of a build-signature verification.
 */
struct BuildVerificationResult {
    /// True when the Ed25519 signature is valid.
    bool verified = false;

    /// Build channel string from the embedded constant ("official" / "community").
    std::string channel;

    /// Short Git SHA from the embedded constant (e.g. "a1b2c3d").
    std::string build_id;

    /// Human-readable reason when `verified == false`.
    std::string failure_reason;
};

/**
 * @brief Verify the compile-time Ed25519 build signature.
 *
 * The result is computed once and cached for the lifetime of the process.
 * Calling this function from multiple threads simultaneously is safe.
 *
 * @return `BuildVerificationResult` with `verified=true` iff the binary
 *         is a genuine official ThemisDB release.
 */
[[nodiscard]] BuildVerificationResult verifyBuildSignature();

/**
 * @brief Return the cached build-channel string without performing
 *        cryptographic verification.
 *
 * Returns `THEMIS_BUILD_CHANNEL` directly.  Does NOT imply the signature
 * is valid – use `verifyBuildSignature().verified` for that.
 */
[[nodiscard]] const char* buildChannel() noexcept;

/**
 * @brief Return the cached build-id (short Git SHA) string.
 */
[[nodiscard]] const char* buildId() noexcept;

} // namespace updates
} // namespace themis
