/// @file security_interface.h
/// @brief Abstract interfaces for security operations
/// 
/// This file defines contracts for security-related operations:
/// - Field-level encryption
/// - Key management and key providers
/// - Encryption/decryption services
/// 
/// Design Goals:
/// - Break circular dependencies between Security ↔ Storage
/// - Enable isolated unit testing with mock implementations
/// - Support multiple encryption backends (OpenSSL, HSM, KMS)
/// - Allow storage engines to use encryption without knowing implementation
/// 
/// @note This is a Phase 1 interface definition. Implementations will be
///       refactored in subsequent phases to use this interface.

#pragma once

#include "themis/base/export.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {

/// @brief Encryption algorithm identifier
enum class EncryptionAlgorithm {
    AES_256_GCM,    ///< AES-256 in GCM mode (authenticated encryption)
    CHACHA20_POLY1305,  ///< ChaCha20-Poly1305 (authenticated encryption)
    AES_256_CBC,    ///< AES-256 in CBC mode (requires separate MAC)
    NONE            ///< No encryption (plaintext)
};

/// @brief Encrypted data container
/// 
/// Contains all information needed to decrypt data:
/// - Key identifier (logical name)
/// - Key version (for rotation support)
/// - Initialization vector
/// - Ciphertext
/// - Authentication tag (for AEAD modes)
/// 
/// @note Serialization/deserialization methods will be provided in
///       concrete implementations in future phases.
struct EncryptedData {
    std::string key_id;              ///< Logical key identifier (e.g., "user_pii")
    uint32_t key_version;            ///< Key version for rotation
    std::vector<uint8_t> iv;         ///< Initialization vector
    std::vector<uint8_t> ciphertext; ///< Encrypted payload
    std::vector<uint8_t> tag;        ///< Authentication tag (AEAD modes)
    EncryptionAlgorithm algorithm;   ///< Algorithm used

    EncryptedData() 
        : key_version(0)
        , algorithm(EncryptionAlgorithm::AES_256_GCM) {}
};

/// @brief Abstract interface for encryption key providers
/// 
/// Key providers abstract the source of encryption keys.
/// Implementations can use:
/// - In-memory key storage
/// - HSM (Hardware Security Module)
/// - Cloud KMS (AWS KMS, Azure Key Vault, GCP KMS)
/// - HashiCorp Vault
/// 
/// This enables storage engines to encrypt data without knowing
/// where keys come from or how they are managed.
class THEMIS_BASE_API IKeyProvider {
public:
    virtual ~IKeyProvider() = default;

    /// @brief Get an encryption key by ID and version
    /// 
    /// @param key_id Logical key identifier
    /// @param version Key version (0 = latest)
    /// @return Encryption key bytes, or std::nullopt if not found
    virtual std::optional<std::vector<uint8_t>> getKey(
        std::string_view key_id,
        uint32_t version = 0) const = 0;

    /// @brief Get the latest version number for a key
    /// 
    /// @param key_id Logical key identifier
    /// @return Latest version number, or 0 if key doesn't exist
    virtual uint32_t getLatestKeyVersion(std::string_view key_id) const = 0;

    /// @brief Rotate a key (create new version)
    /// 
    /// Creates a new version of the key. Old versions remain available
    /// for decrypting existing data.
    /// 
    /// @param key_id Logical key identifier
    /// @return New version number, or 0 on failure
    virtual uint32_t rotateKey(std::string_view key_id) = 0;

    /// @brief Check if a key exists
    /// 
    /// @param key_id Logical key identifier
    /// @return true if key exists, false otherwise
    virtual bool hasKey(std::string_view key_id) const = 0;

    /// @brief List all available key IDs
    /// 
    /// @return Vector of key identifiers
    virtual std::vector<std::string> listKeys() const = 0;
};

/// @brief Abstract interface for field-level encryption
/// 
/// Provides high-level encryption operations for field data.
/// Handles:
/// - Algorithm selection
/// - IV generation
/// - Key versioning
/// - Authentication tag generation
/// 
/// Storage engines use this interface to encrypt/decrypt field values
/// without understanding cryptographic details.
class THEMIS_BASE_API IFieldEncryption {
public:
    virtual ~IFieldEncryption() = default;

    /// @brief Encrypt plaintext data
    /// 
    /// Automatically:
    /// - Generates random IV
    /// - Retrieves latest key version
    /// - Computes authentication tag (for AEAD)
    /// - Returns complete EncryptedData container
    /// 
    /// @param key_id Logical key identifier
    /// @param plaintext Data to encrypt
    /// @return Encrypted data container, or std::nullopt on failure
    virtual std::optional<EncryptedData> encrypt(
        std::string_view key_id,
        std::string_view plaintext) = 0;

    /// @brief Decrypt encrypted data
    /// 
    /// Automatically:
    /// - Retrieves correct key version
    /// - Verifies authentication tag (for AEAD)
    /// - Decrypts ciphertext
    /// 
    /// @param encrypted_data Encrypted data container
    /// @return Decrypted plaintext, or std::nullopt on failure
    virtual std::optional<std::string> decrypt(
        const EncryptedData& encrypted_data) = 0;

    /// @brief Re-encrypt data with latest key version
    /// 
    /// Used for key rotation: decrypt with old key, encrypt with new key.
    /// 
    /// @param encrypted_data Data encrypted with old key
    /// @return Data re-encrypted with latest key, or std::nullopt on failure
    virtual std::optional<EncryptedData> reEncrypt(
        const EncryptedData& encrypted_data) = 0;

    /// @brief Set the key provider
    /// 
    /// @param provider Key provider instance
    virtual void setKeyProvider(std::shared_ptr<IKeyProvider> provider) = 0;

    /// @brief Get current key provider
    /// 
    /// @return Current key provider, or nullptr if not set
    virtual std::shared_ptr<IKeyProvider> getKeyProvider() const = 0;

    /// @brief Get supported algorithms
    /// 
    /// @return Vector of supported algorithm identifiers
    virtual std::vector<EncryptionAlgorithm> getSupportedAlgorithms() const = 0;
};

/// @brief Configuration for field encryption
struct FieldEncryptionConfig {
    std::string default_key_id;          ///< Default key for new encryptions
    EncryptionAlgorithm algorithm;       ///< Encryption algorithm to use
    bool auto_rotate_on_read;            ///< Re-encrypt with latest key on read
    uint32_t rotation_period_days;       ///< Auto-rotation period (0 = disabled)

    FieldEncryptionConfig()
        : default_key_id("default")
        , algorithm(EncryptionAlgorithm::AES_256_GCM)
        , auto_rotate_on_read(false)
        , rotation_period_days(0) {}
};

/// @brief Factory interface for creating encryption services
/// 
/// Enables dependency injection of encryption implementations
class THEMIS_BASE_API IFieldEncryptionFactory {
public:
    virtual ~IFieldEncryptionFactory() = default;

    /// @brief Create a new field encryption instance
    /// 
    /// @param config Configuration
    /// @param key_provider Key provider to use
    /// @return Field encryption instance, or nullptr on failure
    virtual std::unique_ptr<IFieldEncryption> createFieldEncryption(
        const FieldEncryptionConfig& config,
        std::shared_ptr<IKeyProvider> key_provider) = 0;
};

/// @brief Abstract interface for key provider factories
/// 
/// Creates key providers based on configuration.
/// Enables pluggable key storage backends.
class THEMIS_BASE_API IKeyProviderFactory {
public:
    virtual ~IKeyProviderFactory() = default;

    /// @brief Create a new key provider
    /// 
    /// @param type Provider type ("memory", "hsm", "vault", "kms", etc.)
    /// @param config Provider-specific configuration (JSON string)
    /// @return Key provider instance, or nullptr on failure
    virtual std::shared_ptr<IKeyProvider> createKeyProvider(
        std::string_view type,
        const std::string& config) = 0;
};

} // namespace themis
