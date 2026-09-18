/**
 * @file totp_secret_encryption.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/secure_memory.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <chrono>

namespace themis {
namespace auth {

class TOTPSecretEncryption {
public:
    struct Config {
        // Master encryption key (32 bytes for AES-256).
        // Stored in locked, cleanse-on-free memory to prevent key material
        // appearing in core dumps or freed heap pages.
        // In production, this should come from KMS/HSM.
        SecureBuffer<uint8_t> master_key;
        
        // Key derivation iterations (PBKDF2)
        // Higher = more secure but slower (100k recommended)
        int pbkdf2_iterations = 100000;
        
        // Salt size in bytes
        size_t salt_size = 16;
        
        // IV size in bytes (GCM standard is 12)
        size_t iv_size = 12;
        
        // Tag size in bytes (GCM standard is 16)
        size_t tag_size = 16;
        
        // Current key version (for rotation)
        int key_version = 1;
    };
    
    struct EncryptedSecret {
        int version = 0;                    // Key version used for encryption
        std::vector<uint8_t> salt;      // Unique salt for key derivation
        std::vector<uint8_t> iv;        // Initialization vector
        std::vector<uint8_t> ciphertext;// Encrypted secret
        std::vector<uint8_t> tag;       // Authentication tag
        
        /**
         * @brief Serialize to string for storage
         * @return Return value.
         */
        std::string serialize() const;
        
        // Deserialize from string
        /**
         * @brief Deserialize.
         * @param[in] data Input parameter.
         * @return Return value.
         */
        static EncryptedSecret deserialize(const std::string& data);
    };
    
    /**
     * @brief TOTPSecret Encryption.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit TOTPSecretEncryption(const Config& config);
    ~TOTPSecretEncryption();
    
    // Disable copy, allow move
    TOTPSecretEncryption(const TOTPSecretEncryption&) = delete;
    TOTPSecretEncryption& operator=(const TOTPSecretEncryption&) = delete;
    TOTPSecretEncryption(TOTPSecretEncryption&&) noexcept;
    TOTPSecretEncryption& operator=(TOTPSecretEncryption&&) noexcept;
    
    /**
     * @brief Encrypt.
     * @param[in] plaintext_secret Input parameter.
     * @return Return value.
     */
    EncryptedSecret encrypt(const std::string& plaintext_secret);
    
    /**
     * @brief Decrypt.
     * @param[in] encrypted Input parameter.
     * @return Return value.
     */
    std::string decrypt(const EncryptedSecret& encrypted);
    
    /**
     * @brief Encrypt And Serialize.
     * @param[in] plaintext_secret Input parameter.
     * @return Return value.
     */
    std::string encryptAndSerialize(const std::string& plaintext_secret);
    
    /**
     * @brief Deserialize And Decrypt.
     * @param[in] serialized Input parameter.
     * @return Return value.
     */
    std::string deserializeAndDecrypt(const std::string& serialized);
    
    /**
     * @brief Rotate Key.
     * @param[in] new_master_key Input parameter.
     * @param[in] new_version Input parameter.
     */
    void rotateKey(const SecureBuffer<uint8_t>& new_master_key, int new_version);
    
    /**
     * @brief Needs Reencryption.
     * @param[in] encrypted Input parameter.
     * @return True when the operation succeeds.
     */
    bool needsReencryption(const EncryptedSecret& encrypted) const;
    
    /**
     * @brief Reencrypt.
     * @param[in] old_encrypted Input parameter.
     * @return Return value.
     */
    EncryptedSecret reencrypt(const EncryptedSecret& old_encrypted);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Derive encryption key from master key and salt.
     * @param[in] salt Input parameter.
     * @return Return value.
     * @details Returns a SecureBuffer so the derived key is zeroed when it goes out of scope.
     */
    SecureBuffer<uint8_t> deriveKey(const std::vector<uint8_t>& salt);
    
    // Generate random bytes
    /**
     * @brief Generate Random Bytes.
     * @param[in] size Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> generateRandomBytes(size_t size);
};

class TOTPSecretRotationManager {
public:
    struct RotationConfig {
        // Grace period in seconds (default: 30 days)
        int grace_period_seconds = 30 * 24 * 60 * 60;
        
        // Auto-cleanup after grace period
        bool auto_cleanup = true;
    };
    
    struct SecretVersion {
        std::string secret;             // Encrypted secret
        int version;                    // Version number
        std::chrono::system_clock::time_point created_at;
        bool is_active;                 // Is this the current active secret?
    };
    
    TOTPSecretRotationManager();
    /**
     * @brief TOTPSecret Rotation Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit TOTPSecretRotationManager(const RotationConfig& config);
    
    /**
     * @brief Rotate Secret.
     * @param[in] user_id Identifier of the user.
     * @param[in] old_secret Input parameter.
     * @param[in] new_secret Input parameter.
     * @return Return value.
     */
    SecretVersion rotateSecret(
        const std::string& user_id,
        const std::string& old_secret,
        const std::string& new_secret
    );
    
    /**
     * @brief Get Active Secrets.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    std::vector<SecretVersion> getActiveSecrets(const std::string& user_id);
    
    /**
     * @brief Is Secret Valid.
     * @param[in] secret_version Input parameter.
     * @return True when the operation succeeds.
     */
    bool isSecretValid(const SecretVersion& secret_version) const;
    
    /**
     * @brief Cleanup Expired Secrets.
     * @return Return value.
     */
    size_t cleanupExpiredSecrets();

private:
    RotationConfig config_;
    
    // In-memory storage (replace with DB in production)
    std::map<std::string, std::vector<SecretVersion>> user_secrets_;
};

} // namespace auth
} // namespace themis
