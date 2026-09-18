/**
 * @file l3_encryption_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

enum class L3EncryptionMode {
    DISABLED,           ///< No encryption. Development-only.
    AES_256_GCM,        ///< AES-256-GCM authenticated encryption (default).
    CHACHA20_POLY1305,  ///< ChaCha20-Poly1305 — preferred on ARM/mobile.
    XTS_AES_256,        ///< XTS-AES-256 for block-level (sector) encryption.
};

// ---------------------------------------------------------------------------
// L3EncryptionConfig — encryption parameters for the L3 cache tier
// ---------------------------------------------------------------------------

struct L3EncryptionConfig {
    L3EncryptionMode mode                   = L3EncryptionMode::AES_256_GCM;
    std::string      key_provider_id;        ///< KMS/HSM provider identifier.
    bool             encrypt_keys_in_cache   = false; ///< Also encrypt map keys.
    bool             require_auth_tag        = true;  ///< Require AEAD authentication tag.
    int              key_rotation_interval_hours = 24;

    bool isEncryptionEnabled() const {
        return mode != L3EncryptionMode::DISABLED;
    }
};

// ---------------------------------------------------------------------------
// IL3CacheEncryptionManager — encryption/decryption and key management interface
// ---------------------------------------------------------------------------

class IL3CacheEncryptionManager {
public:
    /**
     * @brief IL3 Cache Encryption Manager.
     * @return Return value.
     */
    virtual ~IL3CacheEncryptionManager() = default;

    /**
     * @brief Configure.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool configure(const L3EncryptionConfig& config) = 0;

    /**
     * @brief Encrypt.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    virtual std::vector<uint8_t> encrypt(
        const std::string&         key,
        const std::vector<uint8_t>& value
    ) = 0;

    /**
     * @brief Decrypt.
     * @param[in] key Input parameter.
     * @param[in] ciphertext Input parameter.
     * @return Return value.
     */
    virtual std::vector<uint8_t> decrypt(
        const std::string&         key,
        const std::vector<uint8_t>& ciphertext
    ) = 0;

    /**
     * @brief Rotate Keys.
     * @return True when the operation succeeds.
     */
    virtual bool rotateKeys() = 0;

    /**
     * @brief Active Mode.
     * @return Return value.
     */
    virtual L3EncryptionMode activeMode() const = 0;
};

} // namespace cache
} // namespace themis
