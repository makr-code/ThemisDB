/**
 * @file key_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifndef THEMIS_SECURITY_KEY_PROVIDER_H
#define THEMIS_SECURITY_KEY_PROVIDER_H

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include "themis/base/interfaces/security_interface.h"

namespace themis {

enum class KeyStatus : std::uint8_t {
    ACTIVE,      // Key is active and can be used for encryption/decryption
    ROTATING,    // Key rotation in progress (dual-write mode)
    DEPRECATED,  // Key can decrypt old data but not encrypt new data
    DELETED      // Key is deleted, no operations allowed
};

struct KeyMetadata {
    std::string key_id;      // Logical key identifier (e.g., "user_pii")
    uint32_t version = 0;        // Key version for rotation (1, 2, 3, ...)
    std::string algorithm;   // Encryption algorithm (e.g., "AES-256-GCM")
    int64_t created_at_ms = 0;   // Timestamp when key was created
    int64_t expires_at_ms = 0;   // Expiry timestamp (0 = never expires)
    KeyStatus status = KeyStatus::ACTIVE;        // Current status of the key
};

class KeyNotFoundException : public std::runtime_error {
public:
    /**
     * @brief Key Not Found Exception.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    explicit KeyNotFoundException(const std::string& key_id, uint32_t version)
        : std::runtime_error("Key not found: " + key_id + " v" + std::to_string(version))
        , key_id_(key_id)
        , version_(version)
    {}
    
    [[nodiscard]] const std::string& getKeyId() const { return key_id_; }
    [[nodiscard]] uint32_t getVersion() const { return version_; }
    
private:
    std::string key_id_;
    uint32_t version_;
};

class KeyOperationException : public std::runtime_error {
public:
    /**
     * @brief Key Operation Exception.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    explicit KeyOperationException(const std::string& message)
        : std::runtime_error(message)
        , http_code_(-1)
        , transient_(false)
    {}

    KeyOperationException(std::string message, int http_code, std::string vault_message, bool transient)
        : std::runtime_error(std::move(message))
        , http_code_(http_code)
        , vault_message_(std::move(vault_message))
        , transient_(transient)
    {}

    [[nodiscard]] int httpCode() const { return http_code_; }
    [[nodiscard]] const std::string& vaultMessage() const { return vault_message_; }
    [[nodiscard]] bool transient() const { return transient_; }
private:
    int http_code_;
    std::string vault_message_;
    bool transient_;
};
class KeyProvider : public virtual IKeyProvider {
public:
    KeyProvider() = default;
    KeyProvider(const KeyProvider&) = default;
    KeyProvider(KeyProvider&&) noexcept = default;
    KeyProvider& operator=(const KeyProvider&) = default;
    KeyProvider& operator=(KeyProvider&&) noexcept = default;
    ~KeyProvider() override = default;
    
    // IKeyProvider interface implementation (with defaults)
    std::vector<uint8_t> get_key(const std::string& key_id) override {
        auto key = getKey(key_id);
        return key;
    }
    
    std::vector<uint8_t> rotate_key(const std::string& key_id) override {
        [[maybe_unused]] const uint32_t rotated_version = rotateKey(key_id);
        auto key = getKey(key_id);
        return key;
    }
    
    /**
     * @brief Get Key.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     */
    virtual std::vector<uint8_t> getKey(const std::string& key_id) = 0;
    
    /**
     * @brief Get Key.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    virtual std::vector<uint8_t> getKey(const std::string& key_id, uint32_t version) = 0;
    
    /**
     * @brief Rotate Key.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     */
    virtual uint32_t rotateKey(const std::string& key_id) = 0;
    
    /**
     * @brief List Keys.
     * @return Return value.
     */
    virtual std::vector<KeyMetadata> listKeys() = 0;
    
    virtual KeyMetadata getKeyMetadata(const std::string& key_id, uint32_t version = 0) = 0;
    
    /**
     * @brief Get Current Version.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     * @details Calls: getKey().
     */
    virtual uint32_t getCurrentVersion(const std::string& key_id) {
        // Default probe: walk up from version 1 until getKey(v+1) throws.
        uint32_t ver = 0;
        try {
            // Verify at least version 1 exists (throws KeyNotFoundException if key absent).
            [[maybe_unused]] const auto probe = getKey(key_id, 1);
            ver = 1;
        } catch (...) {
            return 0;
        }
        // Walk higher until the version is not found.
        for (uint32_t v = 2; v <= 0xFFFFu; ++v) {
            try {
                [[maybe_unused]] const auto probe = getKey(key_id, v);
                ver = v;
            } catch (...) {
                break;
            }
            ver = v;
        }

        return ver;
    }

    /**
     * @brief Delete Key.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     */
    virtual void deleteKey(const std::string& key_id, uint32_t version) = 0;
    
    virtual bool hasKey(const std::string& key_id, uint32_t version = 0) = 0;
    
    virtual uint32_t createKeyFromBytes(
        const std::string& key_id,
        const std::vector<uint8_t>& key_bytes,
        const KeyMetadata& metadata = KeyMetadata()) = 0;
};

class KeyCache {
public:
    struct CacheEntry {
        std::vector<uint8_t> key;
        int64_t expires_at_ms;
        uint64_t access_count;
        int64_t last_access_ms;
    };
    
    explicit KeyCache(size_t max_size = 1000, int64_t ttl_ms = 3600000);
    
    /**
     * @brief Get.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     * @param[in,out] out_key Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool get(const std::string& key_id, uint32_t version, std::vector<uint8_t>& out_key);
    
    /**
     * @brief Put.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     * @param[in] key Input parameter.
     */
    void put(const std::string& key_id, uint32_t version, const std::vector<uint8_t>& key);
    
    void evict(const std::string& key_id, uint32_t version = 0);
    
    /**
     * @brief Clear.
     */
    void clear();
    
    /**
     * @brief Get Hit Rate.
     * @return Return value.
     */
    double getHitRate() const;
    
    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;

private:
    std::map<std::string, CacheEntry> cache_;  // "key_id:version" -> Entry
    size_t max_size_;
    int64_t ttl_ms_;
    
    mutable std::mutex mutex_;
    uint64_t total_requests_;
    uint64_t cache_hits_;
    
    /**
     * @brief Make Cache Key.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::string makeCacheKey(const std::string& key_id, uint32_t version) const;
    /**
     * @brief Evict Expired.
     */
    void evictExpired();
    /**
     * @brief Evict LRU.
     */
    void evictLRU();
    /**
     * @brief Get Current Time Ms.
     * @return Return value.
     */
    int64_t getCurrentTimeMs() const;
};

}  // namespace themis

#endif // THEMIS_SECURITY_KEY_PROVIDER_H
