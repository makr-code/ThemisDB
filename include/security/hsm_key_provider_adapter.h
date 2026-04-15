/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hsm_key_provider_adapter.h                         ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     295                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/key_provider.h"
#include "security/hsm_provider.h"
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace security {

/**
 * @brief Adapter to use HSMProvider with KeyProvider interface
 * 
 * This adapter bridges HSMProvider (which provides signing/verification)
 * with KeyProvider interface (which provides encryption key management).
 * 
 * Implementation Strategy: Envelope Encryption
 * - Data Encryption Key (DEK): Random AES-256 key generated per adapter
 * - Key Encryption Key (KEK): RSA key stored in HSM
 * - DEK is encrypted by HSM KEK and stored with adapter metadata
 * - Actual data encryption uses DEK (fast AES-GCM)
 * - HSM only used for DEK wrap/unwrap (slow but secure)
 * 
 * Benefits:
 * - HSM provides hardware-backed security for KEK
 * - Fast data encryption (AES-GCM with cached DEK)
 * - Supports large data without HSM size limits
 * - KEK never leaves HSM hardware
 * 
 * Performance:
 * - DEK caching reduces HSM operations
 * - Cache TTL: 5 minutes (configurable)
 * - HSM operation: ~10-50ms per DEK wrap/unwrap
 * - Data encryption: ~0.5ms per 1KB (AES-GCM)
 * 
 * Thread Safety:
 * - All methods are thread-safe
 * - Uses mutex for cache access
 * - HSMProvider handles its own thread safety
 * 
 * Example Usage:
 * @code
 * // Create HSM provider
 * HSMConfig hsm_config;
 * hsm_config.library_path = "/usr/lib/softhsm/libsofthsm2.so";
 * hsm_config.slot_id = 0;
 * hsm_config.pin = "1234";
 * hsm_config.key_label = "lora-adapter-kek";
 * 
 * auto hsm = std::make_shared<HSMProvider>(hsm_config);
 * if (!hsm->initialize()) {
 *     throw std::runtime_error("HSM init failed");
 * }
 * 
 * // Create adapter
 * auto adapter = std::make_shared<HSMKeyProviderAdapter>(hsm, "lora-adapter-kek");
 * 
 * // Use with FieldEncryption
 * auto encryption = std::make_shared<FieldEncryption>(adapter);
 * @endcode
 */
class HSMKeyProviderAdapter : public KeyProvider {
public:
    /**
     * @brief Configuration for HSM key provider adapter
     */
    struct Config {
        std::string kek_label = "lora-adapter-kek";  // HSM KEK label
        int64_t cache_ttl_ms = 300000;               // 5 minutes
        size_t max_cache_size = 1000;                // Max cached DEKs
        bool enable_caching = true;                  // Enable DEK caching
    };
    
    /**
     * @brief Construct HSM key provider adapter
     * 
     * @param hsm Initialized HSM provider
     * @param config Adapter configuration
     * @throws std::invalid_argument if hsm is null or not initialized
     */
    explicit HSMKeyProviderAdapter(
        std::shared_ptr<HSMProvider> hsm,
        const Config& config
    );
    
    /// Constructor with default config
    explicit HSMKeyProviderAdapter(
        std::shared_ptr<HSMProvider> hsm
    );
    
    ~HSMKeyProviderAdapter() override = default;
    
    // KeyProvider interface implementation
    
    /**
     * @brief Get encryption key (DEK) for key_id
     * 
     * Process:
     * 1. Check cache for DEK
     * 2. If not cached: retrieve encrypted DEK from metadata store
     * 3. Use HSM to decrypt DEK (KEK unwraps DEK)
     * 4. Cache decrypted DEK with TTL
     * 5. Return DEK for data encryption
     * 
     * @param key_id Logical key identifier
     * @return DEK bytes (32 bytes for AES-256)
     * @throws KeyNotFoundException if key doesn't exist
     * @throws KeyOperationException if HSM operation fails
     */
    std::vector<uint8_t> getKey(const std::string& key_id) override;
    
    /**
     * @brief Get specific version of encryption key
     * 
     * @param key_id Logical key identifier
     * @param version Key version
     * @return DEK bytes (32 bytes for AES-256)
     * @throws KeyNotFoundException if key version doesn't exist
     */
    std::vector<uint8_t> getKey(const std::string& key_id, uint32_t version) override;
    
    /**
     * @brief Rotate key (create new version)
     * 
     * Process:
     * 1. Generate new random DEK
     * 2. Use HSM to encrypt DEK with KEK
     * 3. Store encrypted DEK as new version
     * 4. Mark old version as DEPRECATED
     * 5. Return new version number
     * 
     * @param key_id Key to rotate
     * @return New version number
     * @throws KeyOperationException if HSM operation fails
     */
    uint32_t rotateKey(const std::string& key_id) override;
    
    /**
     * @brief List all keys with metadata
     * 
     * @return Vector of key metadata
     */
    std::vector<KeyMetadata> listKeys() override;
    
    /**
     * @brief Get metadata for specific key
     * 
     * @param key_id Key identifier
     * @param version Key version (0 = latest)
     * @return Key metadata
     * @throws KeyNotFoundException if key doesn't exist
     */
    KeyMetadata getKeyMetadata(const std::string& key_id, uint32_t version = 0) override;
    
    /**
     * @brief Delete a key
     * 
     * @param key_id Key identifier
     * @param version Key version to delete
     * @throws KeyOperationException if key is ACTIVE
     */
    void deleteKey(const std::string& key_id, uint32_t version) override;
    
    /**
     * @brief Check if key exists
     * 
     * @param key_id Key identifier
     * @param version Key version (0 = any version)
     * @return true if key exists
     */
    bool hasKey(const std::string& key_id, uint32_t version = 0) override;
    
    /**
     * @brief Create key from provided bytes
     * 
     * Process:
     * 1. Validate key_bytes (must be 32 bytes)
     * 2. Use HSM to encrypt bytes with KEK
     * 3. Store encrypted DEK
     * 4. Return version number
     * 
     * @param key_id Key identifier
     * @param key_bytes Raw key material (32 bytes)
     * @param metadata Key metadata
     * @return Version number
     * @throws std::invalid_argument if key_bytes.size() != 32
     * @throws KeyOperationException if HSM operation fails
     */
    uint32_t createKeyFromBytes(
        const std::string& key_id,
        const std::vector<uint8_t>& key_bytes,
        const KeyMetadata& metadata = KeyMetadata()
    ) override;
    
    /**
     * @brief Get statistics about adapter operations
     * 
     * @return JSON with stats (cache hits, HSM operations, etc.)
     */
    nlohmann::json getStats() const;
    
    /**
     * @brief Clear DEK cache
     */
    void clearCache();
    
    /**
     * @brief Check if HSM is ready
     * 
     * @return true if HSM is initialized and ready
     */
    bool isHSMReady() const;

private:
    std::shared_ptr<HSMProvider> hsm_;
    Config config_;
    
    // DEK cache entry
    struct CachedDEK {
        std::vector<uint8_t> dek;           // Decrypted DEK
        int64_t expires_at_ms;              // Expiry timestamp
        uint64_t access_count;              // Access counter
        
        bool isExpired() const {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            return now >= expires_at_ms;
        }
    };
    
    // Cache: "key_id:version" -> CachedDEK
    std::map<std::string, CachedDEK> dek_cache_;
    mutable std::mutex cache_mutex_;
    
    // Statistics
    struct Stats {
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> cache_misses{0};
        std::atomic<uint64_t> hsm_encrypt_operations{0};
        std::atomic<uint64_t> hsm_decrypt_operations{0};
        std::atomic<uint64_t> hsm_errors{0};
        std::atomic<uint64_t> key_rotations{0};
    };
    mutable Stats stats_;
    
    // Metadata store (in-memory for now, can be replaced with persistent storage)
    struct KeyVersionData {
        std::vector<uint8_t> encrypted_dek;  // DEK encrypted by HSM KEK
        KeyMetadata metadata;
    };
    std::map<std::string, std::map<uint32_t, KeyVersionData>> key_store_;
    mutable std::mutex store_mutex_;
    
    // Helper methods
    std::vector<uint8_t> generateRandomDEK() const;
    std::vector<uint8_t> wrapDEK(const std::vector<uint8_t>& dek);
    std::vector<uint8_t> unwrapDEK(const std::vector<uint8_t>& encrypted_dek);
    std::string makeCacheKey(const std::string& key_id, uint32_t version) const;
    std::string makeStoreKey(const std::string& key_id, uint32_t version) const;
    bool getCachedDEK(const std::string& cache_key, std::vector<uint8_t>& out_dek);
    void putCachedDEK(const std::string& cache_key, const std::vector<uint8_t>& dek);
    void evictExpiredCache();
    uint32_t getLatestVersion(const std::string& key_id) const;
    int64_t getCurrentTimeMs() const;
};

} // namespace security
} // namespace themis
