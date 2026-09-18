/**
 * @file encryption.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/key_provider.h"
#include "themis/base/interfaces/security_interface.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <atomic>
#include <optional>
#include <unordered_set>

namespace themis {

class EncryptionException : public std::runtime_error {
public:
    /**
     * @brief Encryption Exception.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    explicit EncryptionException(const std::string& message)
        : std::runtime_error("Encryption failed: " + message)
    {}
};

class DecryptionException : public std::runtime_error {
public:
    /**
     * @brief Decryption Exception.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    explicit DecryptionException(const std::string& message)
        : std::runtime_error("Decryption failed: " + message)
    {}
};

struct EncryptedBlob {
    std::string key_id;
    uint32_t key_version = {};
    std::vector<uint8_t> iv;          // 12 bytes (AES-GCM standard)
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;         // 16 bytes (AES-GCM authentication tag)
    
    EncryptedBlob() : key_version(0) {}
    
    /**
     * @brief To Base64.
     * @return Return value.
     */
    std::string toBase64() const;
    
    /**
     * @brief From Base64.
     * @param[in] b64 Input parameter.
     * @return Return value.
     */
    static EncryptedBlob fromBase64(const std::string& b64);
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static EncryptedBlob fromJson(const nlohmann::json& j);
};


struct EncryptionConfig {
    std::unordered_map<std::string, std::string> field_key_mapping;
    
    std::unordered_set<std::string> encrypted_fields;
    
    std::string default_key_id = "default";
    
    bool empty() const {
        return field_key_mapping.empty() && encrypted_fields.empty();
    }
};

class FieldEncryption : public IFieldEncryption {
public:
    /**
     * @brief Field Encryption.
     * @param[in] key_provider Input parameter.
     * @return Return value.
     */
    explicit FieldEncryption(std::shared_ptr<KeyProvider> key_provider);
    
    ~FieldEncryption();
    
    /**
     * @brief Create Default.
     * @return Return value.
     */
    static std::shared_ptr<FieldEncryption> createDefault();
    
    /**
     * @brief Encrypt.
     * @param[in] plaintext Input parameter.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     */
    EncryptedBlob encrypt(const std::string& plaintext, const std::string& key_id);
    
    /**
     * @brief Encrypt.
     * @param[in] plaintext Input parameter.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     */
    EncryptedBlob encrypt(const std::vector<uint8_t>& plaintext, const std::string& key_id);
    
    /**
     * @brief Decrypt To String.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::string decryptToString(const EncryptedBlob& blob);
    
    /**
     * @brief Decrypt To Bytes.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> decryptToBytes(const EncryptedBlob& blob);
    
    /**
     * @brief Encrypt With Key.
     * @param[in] plaintext Input parameter.
     * @param[in] key_id Identifier of the key.
     * @param[in] key_version Input parameter.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    EncryptedBlob encryptWithKey(const std::string& plaintext,
                                  const std::string& key_id,
                                  uint32_t key_version,
                                  const std::vector<uint8_t>& key);
    
    /**
     * @brief Decrypt With Key.
     * @param[in] blob Input parameter.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::string decryptWithKey(const EncryptedBlob& blob,
                                const std::vector<uint8_t>& key);

    std::vector<EncryptedBlob> encryptEntityBatch(const std::vector<std::pair<std::string,std::string>>& items,
                                                  const std::string& key_id);
    
    /**
     * @brief Decrypt.
     * @param[in] blob Input parameter.
     * @return Return value.
     * @details Calls: decryptToString().
     */
    std::string decrypt(const EncryptedBlob& blob) { 
        return decryptToString(blob); 
    }
    
    std::shared_ptr<KeyProvider> getKeyProvider() const { 
        return key_provider_; 
    }
    
    // IFieldEncryption interface implementation
    
    std::vector<uint8_t> encrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& plaintext) override;
    
    std::vector<uint8_t> decrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& ciphertext) override;
    
    bool should_encrypt(const std::string& field_name) const override;
    
    /**
     * @brief Set Encryption Config.
     * @param[in] config Input parameter.
     */
    void setEncryptionConfig(const EncryptionConfig& config);

    /**
     * @brief Decrypt And Re Encrypt.
     * @param[in] blob Input parameter.
     * @param[in] key_id Identifier of the key.
     * @param[in,out] updated_blob Input/output parameter.
     * @return Return value.
     */
    std::string decryptAndReEncrypt(const EncryptedBlob& blob,
                                     const std::string& key_id,
                                     std::optional<EncryptedBlob>& updated_blob);

    /**
     * @brief Needs Re Encryption.
     * @param[in] blob Input parameter.
     * @param[in] key_id Identifier of the key.
     * @return True when the operation succeeds.
     */
    bool needsReEncryption(const EncryptedBlob& blob, const std::string& key_id);

    struct Metrics {
        // Operation counters
        std::atomic<uint64_t> encrypt_operations_total{0};
        std::atomic<uint64_t> decrypt_operations_total{0};
        std::atomic<uint64_t> reencrypt_operations_total{0};
        std::atomic<uint64_t> reencrypt_skipped_total{0};  // Already latest version
        
        // Error counters
        std::atomic<uint64_t> encrypt_errors_total{0};
        std::atomic<uint64_t> decrypt_errors_total{0};
        std::atomic<uint64_t> reencrypt_errors_total{0};
        
        // Key version tracking (per key_id)
        // Note: In production, use thread-safe map or registry pattern
        std::atomic<uint64_t> key_rotation_events_total{0};
        
        // Performance metrics (duration buckets in microseconds)
        std::atomic<uint64_t> encrypt_duration_le_100us{0};
        std::atomic<uint64_t> encrypt_duration_le_500us{0};
        std::atomic<uint64_t> encrypt_duration_le_1ms{0};
        std::atomic<uint64_t> encrypt_duration_le_5ms{0};
        std::atomic<uint64_t> encrypt_duration_le_10ms{0};
        std::atomic<uint64_t> encrypt_duration_gt_10ms{0};
        
        std::atomic<uint64_t> decrypt_duration_le_100us{0};
        std::atomic<uint64_t> decrypt_duration_le_500us{0};
        std::atomic<uint64_t> decrypt_duration_le_1ms{0};
        std::atomic<uint64_t> decrypt_duration_le_5ms{0};
        std::atomic<uint64_t> decrypt_duration_le_10ms{0};
        std::atomic<uint64_t> decrypt_duration_gt_10ms{0};
        
        // Bytes processed
        std::atomic<uint64_t> encrypt_bytes_total{0};
        std::atomic<uint64_t> decrypt_bytes_total{0};
    };

    const Metrics& getMetrics() const { return metrics_; }

private:
    std::shared_ptr<KeyProvider> key_provider_;
    EncryptionConfig config_;
    mutable Metrics metrics_;  // Thread-safe atomic counters
    
    /**
     * @brief Generate IV.
     * @return Return value.
     */
    std::vector<uint8_t> generateIV() const;
    
    /**
     * @brief Get Key Id For Field.
     * @param[in] field_name Name of the field.
     * @return Return value.
     */
    std::string getKeyIdForField(const std::string& field_name) const;
    
    /**
     * @brief Encrypt Internal.
     * @param[in] plaintext Input parameter.
     * @param[in] key_id Identifier of the key.
     * @param[in] key_version Input parameter.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    EncryptedBlob encryptInternal(const std::vector<uint8_t>& plaintext,
                                   const std::string& key_id,
                                   uint32_t key_version,
                                   const std::vector<uint8_t>& key);
    
    /**
     * @brief Decrypt Internal.
     * @param[in] blob Input parameter.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> decryptInternal(const EncryptedBlob& blob,
                                          const std::vector<uint8_t>& key);
};

template<typename T>
class EncryptedField {
public:
    /**
     * @brief Set Field Encryption.
     * @param[in] encryption Input parameter.
     */
    static void setFieldEncryption(std::shared_ptr<FieldEncryption> encryption);
    
    EncryptedField();
    
    EncryptedField(const T& value, const std::string& key_id);
    
    /**
     * @brief Encrypted Field.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    explicit EncryptedField(const EncryptedBlob& blob);
    
    /**
     * @brief Encrypt.
     * @param[in] value Input parameter.
     * @param[in] key_id Identifier of the key.
     */
    void encrypt(const T& value, const std::string& key_id);
    
    T decrypt() const;
    
    /**
     * @brief Is Encrypted.
     * @return True when the operation succeeds.
     */
    bool isEncrypted() const;
    
    /**
     * @brief Has Value.
     * @return True when the operation succeeds.
     */
    bool hasValue() const;
    
    /**
     * @brief To Base64.
     * @return Return value.
     */
    std::string toBase64() const;
    
    /**
     * @brief From Base64.
     * @param[in] b64 Input parameter.
     * @return Return value.
     */
    static EncryptedField<T> fromBase64(const std::string& b64);
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static EncryptedField<T> fromJson(const nlohmann::json& j);
    
    const EncryptedBlob& getBlob() const { return blob_; }

private:
    EncryptedBlob blob_;
    static std::shared_ptr<FieldEncryption> field_encryption_;
    
    /**
     * @brief Serialize.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static std::string serialize(const T& value);
    /**
     * @brief Deserialize.
     * @param[in] str Input parameter.
     * @return Return value.
     */
    static T deserialize(const std::string& str);
};

// Template method declarations (implementation in encryption.cpp)
template<typename T>
std::shared_ptr<FieldEncryption> EncryptedField<T>::field_encryption_ = nullptr;

}  // namespace themis
