/**
 * @file mock_key_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/key_provider.h"
#include <map>
#include <mutex>
#include <random>

namespace themis {

class MockKeyProvider : public virtual KeyProvider {
public:
    MockKeyProvider();
    ~MockKeyProvider() override = default;
    
    /**
     * @brief Create Key.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     */
    void createKey(const std::string& key_id, uint32_t version);
    
    /**
     * @brief Create Key With Bytes.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     * @param[in] key_bytes Input parameter.
     */
    void createKeyWithBytes(const std::string& key_id, 
                           uint32_t version,
                           const std::vector<uint8_t>& key_bytes);
    
    // KeyProvider interface implementation
    std::vector<uint8_t> getKey(const std::string& key_id) override;
    
    std::vector<uint8_t> getKey(const std::string& key_id, uint32_t version) override;
    
    uint32_t rotateKey(const std::string& key_id) override;
    
    std::vector<KeyMetadata> listKeys() override;
    
    KeyMetadata getKeyMetadata(const std::string& key_id, uint32_t version = 0) override;
    
    void deleteKey(const std::string& key_id, uint32_t version) override;
    
    bool hasKey(const std::string& key_id, uint32_t version = 0) override;
    
    uint32_t createKeyFromBytes(
        const std::string& key_id,
        const std::vector<uint8_t>& key_bytes,
        const KeyMetadata& metadata = KeyMetadata()) override;
    
    /**
     * @brief Get Latest Version.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     */
    uint32_t getLatestVersion(const std::string& key_id) const;
    
    /**
     * @brief Clear.
     */
    void clear();

private:
    struct KeyEntry {
        std::vector<uint8_t> key;
        KeyMetadata metadata;
    };
    
    // key_id -> (version -> KeyEntry)
    std::map<std::string, std::map<uint32_t, KeyEntry>> keys_;
    
    mutable std::mutex mutex_;
    std::mt19937 rng_;
    
    /**
     * @brief Generate Random Key.
     * @return Return value.
     */
    std::vector<uint8_t> generateRandomKey();
    /**
     * @brief Make Key Path.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::string makeKeyPath(const std::string& key_id, uint32_t version) const;
    /**
     * @brief Get Current Time Ms.
     * @return Return value.
     */
    int64_t getCurrentTimeMs() const;
};

}  // namespace themis
