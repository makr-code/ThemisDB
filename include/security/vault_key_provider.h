/**
 * @file vault_key_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "key_provider.h"
#include "security/signing_provider.h"
#include <curl/curl.h>
#include <chrono>
#include <memory>
#include <functional>

namespace themis {

class VaultKeyProvider : public SigningProvider {
public:
    struct Config {
        std::string vault_addr;      // Production: "https://vault.example.com:8200"; loopback-only dev HTTP is allowed.
        std::string vault_token;     // Authentication token
        std::string kv_mount_path;   // KV secrets engine mount (default: "themis")
        std::string transit_mount;   // Transit mount path (default: "transit")
        std::string kv_version;      // "v1" or "v2" (default: "v2")
        int cache_ttl_seconds;       // Cache TTL (default: 3600)
        int cache_capacity;          // Max cached keys (default: 1000)
        int request_timeout_ms;      // HTTP timeout (default: 5000)
        bool verify_ssl;             // SSL verification (default: true)
        int transit_max_retries;      // Optional retry settings for transit calls
        int transit_backoff_ms;       // Backoff between retry attempts

        Config()
            : vault_addr(),
              vault_token(),
              kv_mount_path("themis"),
              transit_mount("transit"),
              kv_version("v2"),
              cache_ttl_seconds(3600),
              cache_capacity(1000),
              request_timeout_ms(5000),
              verify_ssl(true),
              transit_max_retries(3),
              transit_backoff_ms(200)
        {}
    };
    
    /**
     * @brief Vault Key Provider.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit VaultKeyProvider(const Config& config);
    
    VaultKeyProvider(
        const std::string& vault_addr,
        const std::string& vault_token,
        const std::string& kv_mount_path = "themis"
    );
    
    ~VaultKeyProvider() override;
    
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

    // SigningProvider interface: perform a sign operation via Vault Transit
    SigningResult sign(const std::string& key_id, const std::vector<uint8_t>& data) override;
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    struct CacheStats {
        double hit_rate = 0;
        size_t total_requests;
        size_t cache_hits;
        size_t cache_size;
    };
    /**
     * @brief Get Cache Stats.
     * @return Return value.
     */
    CacheStats getCacheStats() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
protected:
    /**
     * @brief Http Get.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    virtual std::string httpGet(const std::string& path);
    /**
     * @brief Http Post.
     * @param[in] path Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    virtual std::string httpPost(const std::string& path, const std::string& body);
    /**
     * @brief Http List.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    virtual std::string httpList(const std::string& path);
    
    // Vault API wrappers
    std::string readSecret(const std::string& key_id, uint32_t version = 0);
    /**
     * @brief Read Secret Metadata.
     * @param[in] key_id Identifier of the key.
     * @return Return value.
     */
    std::string readSecretMetadata(const std::string& key_id);
    /**
     * @brief Write Secret.
     * @param[in] key_id Identifier of the key.
     * @param[in] key_b64 Input parameter.
     * @param[in] version Input parameter.
     */
    void writeSecret(const std::string& key_id, const std::string& key_b64, uint32_t version);
    /**
     * @brief List Secrets.
     * @return Return value.
     */
    std::vector<std::string> listSecrets();
    
    /**
     * @brief Parse Key From Vault Response.
     * @param[in] json_response Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> parseKeyFromVaultResponse(const std::string& json_response);
    /**
     * @brief Parse Metadata From Vault Response.
     * @param[in] json_response Input parameter.
     * @return Return value.
     */
    KeyMetadata parseMetadataFromVaultResponse(const std::string& json_response);
    
    /**
     * @brief Make Cache Key.
     * @param[in] key_id Identifier of the key.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::string makeCacheKey(const std::string& key_id, uint32_t version) const;

public:
    // Testing: override HTTP behavior (url, method, body) -> response
    // Tests need to override HTTP behavior; expose this as public for test harnesses.
    void setTestRequestOverride(std::function<std::string(const std::string&, const std::string&, const std::string&)> fn);

protected:
};

} // namespace themis
