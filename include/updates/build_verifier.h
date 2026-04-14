/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            build_verifier.h                                   ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-14                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file build_verifier.h
 * @brief Ed25519 build-signature verifier for build-channel authentication.
 *
 * Provides a single entry point `verifyBuildSignature()` that:
 *  1. Reconstructs the canonical manifest string from the compile-time
 *     constants embedded in `build_info.h`:
 *       `"<channel>|<version>|<build_id>|<timestamp>"`
 *  2. Decodes the Base64 signature `THEMIS_BUILD_SIG` and the Base64 public
 *     key `THEMIS_BUILD_PUBKEY`.
 *  3. Verifies the Ed25519 signature using OpenSSL's EVP API (EVP_DigestVerify
 *     with `EVP_PKEY_ED25519`).
 *  4. Caches the result so subsequent calls are O(1).
 *
 * ## Returns
 *  - `true`  – signature valid, binary is a genuine official release.
 *  - `false` – signature invalid, empty, or OpenSSL unavailable
 *              (community/self-compiled builds always return false).
 *
 * ## Thread safety
 *  The first call performs the verification under an internal `std::once_flag`.
 *  All subsequent calls read the cached result without locking.
 *
 * ## Dependencies
 *  OpenSSL 1.1.1+ or 3.x (`libssl` / `libcrypto`).
 *  When built without OpenSSL (`THEMIS_HAVE_OPENSSL` not defined) the
 *  function always returns `false` and logs a warning.
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
