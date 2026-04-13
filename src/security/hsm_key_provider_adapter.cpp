/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hsm_key_provider_adapter.cpp                       ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:29:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     491                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • e52586aae2  2026-02-22  feat(security): implement HSM PKCS#11 direct DEK wrap/unw... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "security/hsm_key_provider_adapter.h"
#include <nlohmann/json.hpp>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <algorithm>

namespace themis {
namespace security {

HSMKeyProviderAdapter::HSMKeyProviderAdapter(
    std::shared_ptr<HSMProvider> hsm,
    const Config& config
) : hsm_(hsm), config_(config) {
    if (!hsm_) {
        throw std::invalid_argument("HSM provider cannot be null");
    }
    
    if (!hsm_->isReady()) {
        throw std::invalid_argument("HSM provider must be initialized before creating adapter");
    }
    
    spdlog::info("HSMKeyProviderAdapter initialized:");
    spdlog::info("  KEK Label: {}", config_.kek_label);
    spdlog::info("  Cache TTL: {}ms", config_.cache_ttl_ms);
    spdlog::info("  Max Cache Size: {}", config_.max_cache_size);
    spdlog::info("  Caching: {}", config_.enable_caching ? "enabled" : "disabled");
}

HSMKeyProviderAdapter::HSMKeyProviderAdapter(
    std::shared_ptr<HSMProvider> hsm
) : HSMKeyProviderAdapter(hsm, Config{}) {
}

std::vector<uint8_t> HSMKeyProviderAdapter::getKey(const std::string& key_id) {
    // Get latest version
    std::lock_guard<std::mutex> lock(store_mutex_);
    
    auto it = key_store_.find(key_id);
    if (it == key_store_.end() || it->second.empty()) {
        throw KeyNotFoundException(key_id, 0);
    }
    
    // Find latest ACTIVE version
    uint32_t latest_version = 0;
    for (const auto& [version, data] : it->second) {
        if (data.metadata.status == KeyStatus::ACTIVE && version > latest_version) {
            latest_version = version;
        }
    }
    
    if (latest_version == 0) {
        throw KeyOperationException("No active version found for key: " + key_id);
    }
    
    return getKey(key_id, latest_version);
}

std::vector<uint8_t> HSMKeyProviderAdapter::getKey(const std::string& key_id, uint32_t version) {
    // Check cache first
    std::string cache_key = makeCacheKey(key_id, version);
    std::vector<uint8_t> dek;
    
    if (config_.enable_caching && getCachedDEK(cache_key, dek)) {
        stats_.cache_hits++;
        return dek;
    }
    
    stats_.cache_misses++;
    
    // Retrieve encrypted DEK from store
    std::lock_guard<std::mutex> lock(store_mutex_);
    
    auto key_it = key_store_.find(key_id);
    if (key_it == key_store_.end()) {
        throw KeyNotFoundException(key_id, version);
    }
    
    auto version_it = key_it->second.find(version);
    if (version_it == key_it->second.end()) {
        throw KeyNotFoundException(key_id, version);
    }
    
    // Check if key is deleted
    if (version_it->second.metadata.status == KeyStatus::DELETED) {
        throw KeyOperationException("Key is deleted: " + key_id + " v" + std::to_string(version));
    }
    
    // Unwrap DEK using HSM
    dek = unwrapDEK(version_it->second.encrypted_dek);
    
    // Cache the decrypted DEK
    if (config_.enable_caching) {
        putCachedDEK(cache_key, dek);
    }
    
    return dek;
}

uint32_t HSMKeyProviderAdapter::rotateKey(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(store_mutex_);
    
    // Get current latest version
    uint32_t new_version = getLatestVersion(key_id) + 1;
    
    // Generate new DEK
    auto dek = generateRandomDEK();
    
    // Wrap DEK with HSM KEK
    auto encrypted_dek = wrapDEK(dek);
    
    // Mark old version as DEPRECATED
    if (key_store_.find(key_id) != key_store_.end()) {
        for (auto& [version, data] : key_store_[key_id]) {
            if (data.metadata.status == KeyStatus::ACTIVE) {
                data.metadata.status = KeyStatus::DEPRECATED;
            }
        }
    }
    
    // Store new version
    KeyVersionData new_data;
    new_data.encrypted_dek = encrypted_dek;
    new_data.metadata.key_id = key_id;
    new_data.metadata.version = new_version;
    new_data.metadata.algorithm = "AES-256-GCM";
    new_data.metadata.status = KeyStatus::ACTIVE;
    new_data.metadata.created_at_ms = getCurrentTimeMs();
    new_data.metadata.expires_at_ms = 0; // Never expires
    
    key_store_[key_id][new_version] = new_data;
    
    stats_.key_rotations++;
    
    spdlog::info("Rotated key {} to version {}", key_id, new_version);
    
    return new_version;
}

std::vector<KeyMetadata> HSMKeyProviderAdapter::listKeys() {
    std::lock_guard<std::mutex> lock(store_mutex_);
    
    std::vector<KeyMetadata> result;
    for (const auto& [key_id, versions] : key_store_) {
        for (const auto& [version, data] : versions) {
            result.push_back(data.metadata);
        }
    }
    
    return result;
}

KeyMetadata HSMKeyProviderAdapter::getKeyMetadata(const std::string& key_id, uint32_t version) {
    std::lock_guard<std::mutex> lock(store_mutex_);
    
    auto key_it = key_store_.find(key_id);
    if (key_it == key_store_.end()) {
        throw KeyNotFoundException(key_id, version);
    }
    
    if (version == 0) {
        // Get latest version
        uint32_t latest = 0;
        for (const auto& [v, data] : key_it->second) {
            if (v > latest) {
                latest = v;
            }
        }
        version = latest;
    }
    
    auto version_it = key_it->second.find(version);
    if (version_it == key_it->second.end()) {
        throw KeyNotFoundException(key_id, version);
    }
    
    return version_it->second.metadata;
}

void HSMKeyProviderAdapter::deleteKey(const std::string& key_id, uint32_t version) {
    std::lock_guard<std::mutex> lock(store_mutex_);
    
    auto key_it = key_store_.find(key_id);
    if (key_it == key_store_.end()) {
        throw KeyNotFoundException(key_id, version);
    }
    
    auto version_it = key_it->second.find(version);
    if (version_it == key_it->second.end()) {
        throw KeyNotFoundException(key_id, version);
    }
    
    // Check if key is ACTIVE
    if (version_it->second.metadata.status == KeyStatus::ACTIVE) {
        throw KeyOperationException("Cannot delete ACTIVE key: " + key_id + " v" + std::to_string(version));
    }
    
    // Mark as DELETED
    version_it->second.metadata.status = KeyStatus::DELETED;
    
    // Clear from cache
    std::string cache_key = makeCacheKey(key_id, version);
    std::lock_guard<std::mutex> cache_lock(cache_mutex_);
    dek_cache_.erase(cache_key);
    
    spdlog::info("Deleted key {} version {}", key_id, version);
}

bool HSMKeyProviderAdapter::hasKey(const std::string& key_id, uint32_t version) {
    std::lock_guard<std::mutex> lock(store_mutex_);
    
    auto key_it = key_store_.find(key_id);
    if (key_it == key_store_.end()) {
        return false;
    }
    
    if (version == 0) {
        return !key_it->second.empty();
    }
    
    return key_it->second.find(version) != key_it->second.end();
}

uint32_t HSMKeyProviderAdapter::createKeyFromBytes(
    const std::string& key_id,
    const std::vector<uint8_t>& key_bytes,
    const KeyMetadata& metadata
) {
    if (key_bytes.size() != 32) {
        throw std::invalid_argument("Key must be exactly 32 bytes for AES-256");
    }
    
    std::lock_guard<std::mutex> lock(store_mutex_);
    
    uint32_t version = metadata.version;
    if (version == 0) {
        version = getLatestVersion(key_id) + 1;
    }
    
    // Wrap DEK with HSM KEK
    auto encrypted_dek = wrapDEK(key_bytes);
    
    // Store encrypted DEK
    KeyVersionData new_data;
    new_data.encrypted_dek = encrypted_dek;
    new_data.metadata = metadata;
    new_data.metadata.key_id = key_id;
    new_data.metadata.version = version;
    
    if (new_data.metadata.algorithm.empty()) {
        new_data.metadata.algorithm = "AES-256-GCM";
    }
    if (new_data.metadata.created_at_ms == 0) {
        new_data.metadata.created_at_ms = getCurrentTimeMs();
    }
    if (new_data.metadata.status == KeyStatus::ACTIVE) {
        // Mark other active versions as deprecated
        for (auto& [v, data] : key_store_[key_id]) {
            if (data.metadata.status == KeyStatus::ACTIVE) {
                data.metadata.status = KeyStatus::DEPRECATED;
            }
        }
    }
    
    key_store_[key_id][version] = new_data;
    
    spdlog::info("Created key {} version {} from bytes", key_id, version);
    
    return version;
}

nlohmann::json HSMKeyProviderAdapter::getStats() const {
    nlohmann::json stats;
    stats["cache_hits"] = stats_.cache_hits.load();
    stats["cache_misses"] = stats_.cache_misses.load();
    stats["cache_hit_rate"] = 0.0;
    
    uint64_t total_requests = stats_.cache_hits.load() + stats_.cache_misses.load();
    if (total_requests > 0) {
        stats["cache_hit_rate"] = static_cast<double>(stats_.cache_hits.load()) / total_requests;
    }
    
    stats["hsm_encrypt_operations"] = stats_.hsm_encrypt_operations.load();
    stats["hsm_decrypt_operations"] = stats_.hsm_decrypt_operations.load();
    stats["hsm_errors"] = stats_.hsm_errors.load();
    stats["key_rotations"] = stats_.key_rotations.load();
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        stats["cache_size"] = dek_cache_.size();
    }
    
    {
        std::lock_guard<std::mutex> lock(store_mutex_);
        stats["total_keys"] = key_store_.size();
        size_t total_versions = 0;
        for (const auto& [key_id, versions] : key_store_) {
            total_versions += versions.size();
        }
        stats["total_key_versions"] = total_versions;
    }
    
    return stats;
}

void HSMKeyProviderAdapter::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    dek_cache_.clear();
    spdlog::info("Cleared DEK cache");
}

bool HSMKeyProviderAdapter::isHSMReady() const {
    return hsm_ && hsm_->isReady();
}

// Private helper methods

std::vector<uint8_t> HSMKeyProviderAdapter::generateRandomDEK() const {
    std::vector<uint8_t> dek(32); // 256 bits for AES-256
    
    if (RAND_bytes(dek.data(), dek.size()) != 1) {
        unsigned long err = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        throw std::runtime_error("Failed to generate random DEK: " + std::string(err_buf));
    }
    
    return dek;
}

std::vector<uint8_t> HSMKeyProviderAdapter::wrapDEK(const std::vector<uint8_t>& dek) {
    stats_.hsm_encrypt_operations++;
    
    try {
        // Use HSM to encrypt the DEK with the KEK stored in the HSM.
        // For real HSMs: uses PKCS#11 C_Encrypt (RSA-PKCS#1 v1.5) with the HSM public key.
        // For stub/fallback: uses AES-256-GCM with an in-memory stub KEK.
        auto encrypted = hsm_->encryptData(dek, config_.kek_label);
        
        if (encrypted.empty()) {
            stats_.hsm_errors++;
            throw KeyOperationException("HSM failed to wrap DEK: " + hsm_->getLastError());
        }
        
        return encrypted;
        
    } catch (const KeyOperationException&) {
        throw;
    } catch (const std::exception& e) {
        stats_.hsm_errors++;
        throw KeyOperationException("Failed to wrap DEK with HSM: " + std::string(e.what()));
    }
}

std::vector<uint8_t> HSMKeyProviderAdapter::unwrapDEK(const std::vector<uint8_t>& encrypted_dek) {
    stats_.hsm_decrypt_operations++;
    
    try {
        // Use HSM to decrypt the wrapped DEK using the KEK stored in the HSM.
        // For real HSMs: uses PKCS#11 C_Decrypt (RSA-PKCS#1 v1.5) with the HSM private key.
        // For stub/fallback: uses AES-256-GCM with the same in-memory stub KEK used for wrapping.
        auto dek = hsm_->decryptData(encrypted_dek, config_.kek_label);
        
        if (dek.empty()) {
            stats_.hsm_errors++;
            throw KeyOperationException("HSM failed to unwrap DEK: " + hsm_->getLastError());
        }
        
        return dek;
        
    } catch (const KeyOperationException&) {
        throw;
    } catch (const std::exception& e) {
        stats_.hsm_errors++;
        throw KeyOperationException("Failed to unwrap DEK with HSM: " + std::string(e.what()));
    }
}

std::string HSMKeyProviderAdapter::makeCacheKey(const std::string& key_id, uint32_t version) const {
    return key_id + ":" + std::to_string(version);
}

std::string HSMKeyProviderAdapter::makeStoreKey(const std::string& key_id, uint32_t version) const {
    return key_id + ":" + std::to_string(version);
}

bool HSMKeyProviderAdapter::getCachedDEK(const std::string& cache_key, std::vector<uint8_t>& out_dek) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Evict expired entries
    evictExpiredCache();
    
    auto it = dek_cache_.find(cache_key);
    if (it == dek_cache_.end()) {
        return false;
    }
    
    if (it->second.isExpired()) {
        dek_cache_.erase(it);
        return false;
    }
    
    out_dek = it->second.dek;
    it->second.access_count++;
    return true;
}

void HSMKeyProviderAdapter::putCachedDEK(const std::string& cache_key, const std::vector<uint8_t>& dek) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check cache size limit
    if (dek_cache_.size() >= config_.max_cache_size) {
        // Evict least recently used entry
        auto oldest = dek_cache_.begin();
        for (auto it = dek_cache_.begin(); it != dek_cache_.end(); ++it) {
            if (it->second.access_count < oldest->second.access_count) {
                oldest = it;
            }
        }
        dek_cache_.erase(oldest);
    }
    
    CachedDEK cached;
    cached.dek = dek;
    cached.expires_at_ms = getCurrentTimeMs() + config_.cache_ttl_ms;
    cached.access_count = 1;
    
    dek_cache_[cache_key] = cached;
}

void HSMKeyProviderAdapter::evictExpiredCache() {
    // Note: called with cache_mutex_ already held
    auto it = dek_cache_.begin();
    while (it != dek_cache_.end()) {
        if (it->second.isExpired()) {
            it = dek_cache_.erase(it);
        } else {
            ++it;
        }
    }
}

uint32_t HSMKeyProviderAdapter::getLatestVersion(const std::string& key_id) const {
    // Note: called with store_mutex_ already held
    auto it = key_store_.find(key_id);
    if (it == key_store_.end() || it->second.empty()) {
        return 0;
    }
    
    uint32_t latest = 0;
    for (const auto& [version, data] : it->second) {
        if (version > latest) {
            latest = version;
        }
    }
    
    return latest;
}

int64_t HSMKeyProviderAdapter::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace security
} // namespace themis
