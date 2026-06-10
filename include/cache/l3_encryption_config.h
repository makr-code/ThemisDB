/**
 * @file l3_encryption_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace cache {

// ---------------------------------------------------------------------------
// L3EncryptionMode — supported AEAD encryption algorithms
// ---------------------------------------------------------------------------

/**
 * @brief Encryption algorithm applied to L3 cache values.
 *
 * All production modes are AEAD; only DISABLED skips authentication tags.
 * DISABLED must not be used in production — it exists solely for
 * development environments gated by a build flag.
 */
enum class L3EncryptionMode {
    DISABLED,           ///< No encryption. Development-only.
    AES_256_GCM,        ///< AES-256-GCM authenticated encryption (default).
    CHACHA20_POLY1305,  ///< ChaCha20-Poly1305 — preferred on ARM/mobile.
    XTS_AES_256,        ///< XTS-AES-256 for block-level (sector) encryption.
};

// ---------------------------------------------------------------------------
// L3EncryptionConfig — encryption parameters for the L3 cache tier
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for L3 cache encryption.
 *
 * `key_provider_id` identifies the KMS/HSM provider registered with the
 * secrets subsystem (e.g., "aws-kms-prod", "vault-transit").
 *
 * `encrypt_keys_in_cache` controls whether map keys (in addition to values)
 * are encrypted; enabling this prevents enumeration of cache keys by anyone
 * with raw access to the storage backend.
 *
 * `key_rotation_interval_hours` drives automatic key rotation via `rotateKeys()`.
 */
struct L3EncryptionConfig {
    L3EncryptionMode mode                   = L3EncryptionMode::AES_256_GCM;
    std::string      key_provider_id;        ///< KMS/HSM provider identifier.
    bool             encrypt_keys_in_cache   = false; ///< Also encrypt map keys.
    bool             require_auth_tag        = true;  ///< Require AEAD authentication tag.
    int              key_rotation_interval_hours = 24;

    /**
     * @brief Return `true` if an encryption mode other than DISABLED is selected.
     */
    bool isEncryptionEnabled() const {
        return mode != L3EncryptionMode::DISABLED;
    }
};

// ---------------------------------------------------------------------------
// IL3CacheEncryptionManager — encryption/decryption and key management interface
// ---------------------------------------------------------------------------

/**
 * @brief Interface for L3 cache encryption, decryption, and key rotation.
 *
 * The cache layer calls `encrypt()` before persisting a value and
 * `decrypt()` after retrieving it.  The `key` parameter allows per-entry
 * AAD (Additional Authenticated Data) binding so that a ciphertext cannot
 * be moved to a different cache entry without detection.
 *
 * ### Thread safety
 * `encrypt()` and `decrypt()` must be safe to call concurrently.
 * `configure()` and `rotateKeys()` may briefly acquire an exclusive lock.
 */
class IL3CacheEncryptionManager {
public:
    virtual ~IL3CacheEncryptionManager() = default;

    /**
     * @brief Apply an encryption configuration.
     *
     * @return `true` on success; `false` if the key_provider_id cannot be
     *         resolved or the configuration is otherwise invalid.
     */
    virtual bool configure(const L3EncryptionConfig& config) = 0;

    /**
     * @brief Encrypt @p value, binding it to @p key as AAD.
     *
     * @return Ciphertext (includes nonce and authentication tag).
     */
    virtual std::vector<uint8_t> encrypt(
        const std::string&         key,
        const std::vector<uint8_t>& value
    ) = 0;

    /**
     * @brief Decrypt @p ciphertext, verifying the AAD binding to @p key.
     *
     * @return Plaintext on success; empty vector if decryption or tag
     *         verification fails.
     */
    virtual std::vector<uint8_t> decrypt(
        const std::string&         key,
        const std::vector<uint8_t>& ciphertext
    ) = 0;

    /**
     * @brief Rotate the active data-encryption key via the KMS/HSM provider.
     *
     * Re-encrypts in-flight writes with the new key; existing ciphertext is
     * lazily re-encrypted on next read/write.
     *
     * @return `true` if rotation succeeded.
     */
    virtual bool rotateKeys() = 0;

    /// Return the currently active encryption mode.
    virtual L3EncryptionMode activeMode() const = 0;
};

} // namespace cache
} // namespace themis
