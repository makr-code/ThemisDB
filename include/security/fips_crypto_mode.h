/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fips_crypto_mode.h                                 ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:42:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     213                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • d9d75ee094  2026-03-01  feat(security): implement FIPS 140-2/3 validated cryptogr... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <stdexcept>

namespace themis {

/**
 * @brief Exception thrown when a FIPS policy violation is detected.
 *
 * Raised by FipsCryptoMode::validateAlgorithm() when an algorithm that is not
 * approved under FIPS 140-2/3 is requested while FIPS mode is active.
 */
class FipsPolicyViolation : public std::runtime_error {
public:
    explicit FipsPolicyViolation(const std::string& message)
        : std::runtime_error("FIPS policy violation: " + message)
    {}
};

/**
 * @brief FIPS 140-2 / 140-3 validated cryptography mode manager.
 *
 * Manages the activation and enforcement of FIPS 140-2/3 validated
 * cryptography via the OpenSSL FIPS provider (OpenSSL 3.x).
 *
 * ## Design
 * - Singleton: one instance governs the process-wide FIPS state.
 * - Thread-safe: `enable()` / `disable()` must be called before
 *   spawning worker threads; status queries are read-only after that.
 * - Graceful degradation: if the FIPS provider is not installed, `enable()`
 *   returns `false` and logs a warning rather than aborting the process.
 *
 * ## Activation requirements
 * FIPS mode requires a FIPS-validated OpenSSL build (the `fips.so` provider
 * module and the `fips.cnf` configuration file).  These are NOT bundled with
 * ThemisDB.  Install the appropriate OS package (e.g.,
 * `openssl-fips` on RHEL / `openssl` FIPS module on Ubuntu FIPS kernels)
 * before calling `enable()`.
 *
 * ## FIPS-approved algorithms (NIST SP 800-175B rev. 1 / FIPS 140-3)
 * Symmetric:  AES-128-CBC, AES-192-CBC, AES-256-CBC,
 *             AES-128-CTR, AES-192-CTR, AES-256-CTR,
 *             AES-128-GCM, AES-192-GCM, AES-256-GCM,
 *             AES-128-CCM, AES-256-CCM,
 *             AES-128-XTS, AES-256-XTS,
 *             AES-128-KW,  AES-256-KW   (key-wrap)
 * Hash:       SHA-256, SHA-384, SHA-512, SHA-224,
 *             SHA-512/224, SHA-512/256, SHA3-256, SHA3-384, SHA3-512
 * MAC:        HMAC-SHA-256, HMAC-SHA-384, HMAC-SHA-512, HMAC-SHA-224,
 *             CMAC-AES-128, CMAC-AES-256
 * Asymmetric: RSA-2048, RSA-3072, RSA-4096 (PKCS#1 v2.1 / OAEP / PSS),
 *             ECDSA-P256, ECDSA-P384, ECDSA-P521,
 *             ECDH-P256,  ECDH-P384,  ECDH-P521,
 *             DH-2048,    DH-3072,    DH-4096
 * KDF:        PBKDF2-SHA-256, HKDF-SHA-256, HKDF-SHA-384, HKDF-SHA-512,
 *             SP800-108-CTR, SP800-108-FEEDBACK, SP800-108-PIPELINE
 * DRBG:       CTR_DRBG(AES-256), HASH_DRBG(SHA-512), HMAC_DRBG(SHA-512)
 *
 * Non-approved (blocked when FIPS mode is active):
 *   MD5, SHA-1 (for new signatures), RC4, DES, 3DES,
 *   Blowfish, IDEA, CAST5, ChaCha20-Poly1305 (not FIPS-approved).
 *
 * @note SHA-1 is allowed only for verification of existing signatures; it is
 *       blocked for generating new digital signatures under FIPS 140-3.
 *
 * Usage:
 * @code
 * auto& fips = themis::FipsCryptoMode::instance();
 *
 * if (!fips.enable()) {
 *     // FIPS provider not available; run in non-FIPS mode
 * }
 *
 * // Validate before using an algorithm
 * try {
 *     fips.validateAlgorithm("AES-256-GCM");   // OK
 *     fips.validateAlgorithm("MD5");            // throws FipsPolicyViolation
 * } catch (const FipsPolicyViolation& ex) {
 *     // reject operation
 * }
 * @endcode
 */
class FipsCryptoMode {
public:
    /**
     * @brief Return the process-wide singleton instance.
     */
    static FipsCryptoMode& instance();

    /**
     * @brief Attempt to activate FIPS 140-2/3 validated cryptography mode.
     *
     * Loads the OpenSSL FIPS provider (`fips`) and enables the
     * `fips=yes` property constraint so that only FIPS-approved algorithm
     * implementations are selected by `EVP_*` APIs.
     *
     * @return true  if FIPS mode was successfully activated.
     * @return false if the FIPS provider is unavailable (not installed);
     *               non-FIPS operation continues normally.
     *
     * @throws std::runtime_error if the provider is available but
     *         activation fails for an unexpected reason.
     */
    bool enable();

    /**
     * @brief Deactivate FIPS mode and unload the FIPS provider.
     *
     * Removes the `fips=yes` property constraint and unloads the provider.
     * Safe to call even if FIPS mode was never activated.
     */
    void disable();

    /**
     * @brief Query whether FIPS mode is currently active.
     *
     * @return true if `enable()` succeeded and `disable()` has not been
     *         called since.
     */
    bool isEnabled() const;

    /**
     * @brief Check whether the OpenSSL FIPS provider is available on this system.
     *
     * Does not activate FIPS mode; only probes for provider availability.
     *
     * @return true if the FIPS provider can be loaded.
     */
    bool isAvailable() const;

    /**
     * @brief Validate that @p algorithm is approved under FIPS 140-2/3.
     *
     * The check is performed regardless of whether FIPS mode is currently
     * active; callers can use this method to pre-validate algorithm choices.
     *
     * @param algorithm Case-insensitive algorithm name
     *        (e.g. "AES-256-GCM", "SHA-256", "ECDSA-P256").
     *
     * @throws FipsPolicyViolation if the algorithm is not on the approved list.
     */
    void validateAlgorithm(const std::string& algorithm) const;

    /**
     * @brief Return the set of FIPS-approved algorithm names.
     *
     * Names are upper-cased and match the strings accepted by
     * `validateAlgorithm()`.
     */
    const std::unordered_set<std::string>& approvedAlgorithms() const;

    /**
     * @brief Run the built-in FIPS provider self-tests.
     *
     * Invokes `OSSL_PROVIDER_self_test()` on the loaded FIPS provider.
     *
     * @return true if all self-tests pass.
     * @return false if FIPS mode is not active or self-tests fail.
     */
    bool runSelfTests() const;

    /**
     * @brief Securely zeroize a memory region (FIPS 140-3 requirement).
     *
     * Calls `OPENSSL_cleanse()` to prevent compiler optimisation from
     * eliding the wipe.
     *
     * @param ptr  Pointer to the memory to zeroize.
     * @param len  Number of bytes to zeroize.
     */
    static void zeroize(void* ptr, std::size_t len) noexcept;

    // Non-copyable / non-movable singleton
    FipsCryptoMode(const FipsCryptoMode&)            = delete;
    FipsCryptoMode& operator=(const FipsCryptoMode&) = delete;
    FipsCryptoMode(FipsCryptoMode&&)                 = delete;
    FipsCryptoMode& operator=(FipsCryptoMode&&)      = delete;

private:
    FipsCryptoMode();
    ~FipsCryptoMode();

    struct Impl;
    Impl* impl_;
};

}  // namespace themis
