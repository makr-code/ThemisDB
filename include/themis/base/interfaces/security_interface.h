/**
 * @file security_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace themis {

/**
 * @brief Interface for field-level encryption
 * 
 * Provides abstract interface for encrypting and decrypting individual fields
 * without depending on concrete encryption implementations.
 */
class IFieldEncryption {
public:
    virtual ~IFieldEncryption() = default;
    
    /**
     * @brief Encrypt a field value
     * 
     * @param field_name Name of the field being encrypted
     * @param plaintext The plaintext data to encrypt
     * @return Encrypted data as byte vector
     */
    [[nodiscard]] virtual std::vector<uint8_t> encrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& plaintext) = 0;
    
    /**
     * @brief Decrypt a field value
     * 
     * @param field_name Name of the field being decrypted
     * @param ciphertext The encrypted data to decrypt
     * @return Decrypted plaintext as byte vector
     */
    [[nodiscard]] virtual std::vector<uint8_t> decrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& ciphertext) = 0;
    
    /**
     * @brief Check if a field should be encrypted
     * 
     * @param field_name Name of the field to check
     * @return true if field should be encrypted, false otherwise
     */
    [[nodiscard]] virtual bool should_encrypt(const std::string& field_name) const = 0;
};

/// Shared pointer type for IFieldEncryption
using IFieldEncryptionPtr = std::shared_ptr<IFieldEncryption>;

/**
 * @brief Interface for cryptographic key management
 * 
 * Provides abstract interface for retrieving and rotating encryption keys
 * without depending on concrete key storage implementations (Vault, HSM, etc.).
 */
class IKeyProvider {
public:
    virtual ~IKeyProvider() = default;
    
    /**
     * @brief Get an encryption key by identifier
     * 
     * @param key_id Logical key identifier (e.g., "user_pii", "payment_info")
     * @return The encryption key as byte vector
     */
    [[nodiscard]] virtual std::vector<uint8_t> get_key(const std::string& key_id) = 0;
    
    /**
     * @brief Rotate a key to a new version
     * 
     * @param key_id Logical key identifier
     * @return The new encryption key as byte vector
     */
    [[nodiscard]] virtual std::vector<uint8_t> rotate_key(const std::string& key_id) = 0;
};

/// Shared pointer type for IKeyProvider
using IKeyProviderPtr = std::shared_ptr<IKeyProvider>;

/**
 * @brief Factory interface for field encryption
 */
class IFieldEncryptionFactory {
public:
    virtual ~IFieldEncryptionFactory() = default;
    
    /**
     * @brief Create a field encryption instance
     * 
     * @return Shared pointer to field encryption
     */
    [[nodiscard]] virtual IFieldEncryptionPtr create() = 0;
};

/**
 * @brief Factory interface for key provider
 */
class IKeyProviderFactory {
public:
    virtual ~IKeyProviderFactory() = default;
    
    /**
     * @brief Create a key provider instance
     * 
     * @return Shared pointer to key provider
     */
    [[nodiscard]] virtual IKeyProviderPtr create() = 0;
};

} // namespace themis
