/**
 * @file totp_secret_encryption.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief TOTP Secret Encryption - protects secrets at rest
 * 
 * Security Feature: Encrypts TOTP secrets using AES-256-GCM before storage.
 * Prevents secret disclosure if database is compromised.
 * 
 * Key Management:
 * - Uses master key for encryption (should be from KMS/HSM in production)
 * - Key derivation with PBKDF2 (100k iterations) or Argon2
 * - Unique salt per secret
 * - Authentication tag for integrity
 * 
 * Features:
 * - AES-256-GCM authenticated encryption
 * - Unique IV per encryption
 * - Base64 encoding for storage
 * - Secret rotation support
 * - Key versioning (for key rotation)
 * 
 * Format: version|salt|iv|ciphertext|tag (all base64 encoded)
 * 
 * P1 (High Priority) security hardening feature.
 */
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
    
    /**
     * @brief Encrypted secret container
     */
    struct EncryptedSecret {
        int version;                    // Key version used for encryption
        std::vector<uint8_t> salt;      // Unique salt for key derivation
        std::vector<uint8_t> iv;        // Initialization vector
        std::vector<uint8_t> ciphertext;// Encrypted secret
        std::vector<uint8_t> tag;       // Authentication tag
        
        // Serialize to string for storage
        std::string serialize() const;
        
        // Deserialize from string
        static EncryptedSecret deserialize(const std::string& data);
    };
    
    explicit TOTPSecretEncryption(const Config& config);
    ~TOTPSecretEncryption();
    
    // Disable copy, allow move
    TOTPSecretEncryption(const TOTPSecretEncryption&) = delete;
    TOTPSecretEncryption& operator=(const TOTPSecretEncryption&) = delete;
    TOTPSecretEncryption(TOTPSecretEncryption&&) noexcept;
    TOTPSecretEncryption& operator=(TOTPSecretEncryption&&) noexcept;
    
    /**
     * @brief Encrypt a TOTP secret
     * 
     * @param plaintext_secret The secret to encrypt (typically 20 bytes base32 encoded)
     * @return EncryptedSecret Encrypted secret with metadata
     * @throws std::runtime_error on encryption failure
     */
    EncryptedSecret encrypt(const std::string& plaintext_secret);
    
    /**
     * @brief Decrypt a TOTP secret
     * 
     * @param encrypted The encrypted secret
     * @return std::string Decrypted plaintext secret
     * @throws std::runtime_error on decryption/authentication failure
     */
    std::string decrypt(const EncryptedSecret& encrypted);
    
    /**
     * @brief Encrypt and serialize to string
     * 
     * Convenience method for direct storage.
     * 
     * @param plaintext_secret The secret to encrypt
     * @return std::string Serialized encrypted secret
     */
    std::string encryptAndSerialize(const std::string& plaintext_secret);
    
    /**
     * @brief Deserialize and decrypt from string
     * 
     * Convenience method for direct retrieval.
     * 
     * @param serialized Serialized encrypted secret
     * @return std::string Decrypted plaintext secret
     */
    std::string deserializeAndDecrypt(const std::string& serialized);
    
    /**
     * @brief Rotate encryption key
     * 
     * Updates master key and key version. Old secrets can still be decrypted
     * with old key if provided via rotation support.
     * 
     * @param new_master_key New master key (32 bytes), stored securely
     * @param new_version New key version number
     */
    void rotateKey(const SecureBuffer<uint8_t>& new_master_key, int new_version);
    
    /**
     * @brief Check if secret needs re-encryption (old key version)
     * 
     * @param encrypted The encrypted secret to check
     * @return true if secret should be re-encrypted with current key
     */
    bool needsReencryption(const EncryptedSecret& encrypted) const;
    
    /**
     * @brief Re-encrypt a secret with current key
     * 
     * Used during key rotation to migrate secrets to new key.
     * 
     * @param old_encrypted Secret encrypted with old key
     * @return EncryptedSecret Secret re-encrypted with current key
     */
    EncryptedSecret reencrypt(const EncryptedSecret& old_encrypted);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Derive encryption key from master key and salt.
    // Returns a SecureBuffer so the derived key is zeroed when it goes out of scope.
    SecureBuffer<uint8_t> deriveKey(const std::vector<uint8_t>& salt);
    
    // Generate random bytes
    std::vector<uint8_t> generateRandomBytes(size_t size);
};

/**
 * @brief TOTP Secret Rotation Manager
 * 
 * Manages the lifecycle of TOTP secrets including rotation and migration.
 * 
 * Rotation Strategy:
 * - Grace period: both old and new secrets work during migration
 * - Gradual rollout: users re-enroll over time
 * - Automatic cleanup: old secrets deleted after grace period
 */
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
    explicit TOTPSecretRotationManager(const RotationConfig& config);
    
    /**
     * @brief Rotate a user's TOTP secret
     * 
     * Creates new secret while keeping old one active during grace period.
     * 
     * @param user_id User identifier
     * @param old_secret Current secret
     * @param new_secret New secret to rotate to
     * @return SecretVersion New secret version info
     */
    SecretVersion rotateSecret(
        const std::string& user_id,
        const std::string& old_secret,
        const std::string& new_secret
    );
    
    /**
     * @brief Get all active secrets for a user (during grace period)
     * 
     * @param user_id User identifier
     * @return std::vector<SecretVersion> All active secrets
     */
    std::vector<SecretVersion> getActiveSecrets(const std::string& user_id);
    
    /**
     * @brief Check if a secret is still valid (within grace period)
     * 
     * @param secret_version Secret version to check
     * @return true if secret is still valid
     */
    bool isSecretValid(const SecretVersion& secret_version) const;
    
    /**
     * @brief Clean up expired secrets
     * 
     * Removes secrets that are past their grace period.
     * 
     * @return size_t Number of secrets cleaned up
     */
    size_t cleanupExpiredSecrets();

private:
    RotationConfig config_;
    
    // In-memory storage (replace with DB in production)
    std::map<std::string, std::vector<SecretVersion>> user_secrets_;
};

} // namespace auth
} // namespace themis
