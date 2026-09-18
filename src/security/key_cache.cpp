/**
 * @file key_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/key_provider.h"
#include <algorithm>
#include <chrono>

namespace themis {

KeyCache::KeyCache(size_t max_size, int64_t ttl_ms)
    : max_size_(max_size)
    , ttl_ms_(ttl_ms)
    , total_requests_(0)
    , cache_hits_(0)
{}

/**
 * @brief Get.
 * @param[in] key_id Identifier of the key.
 * @param[in] version Input parameter.
 * @param[in,out] out_key Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), makeCacheKey(), find(), end(), getCurrentTimeMs(), erase().
 */
bool KeyCache::get(const std::string& key_id, uint32_t version, std::vector<uint8_t>& out_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    total_requests_++;
    
    std::string cache_key = makeCacheKey(key_id, version);
    auto it = cache_.find(cache_key);
    
    if (it == cache_.end()) {
        return false;  // Cache miss
    }
    
    // Check expiry
    int64_t now = getCurrentTimeMs();
    if (now > it->second.expires_at_ms) {
        cache_.erase(it);
        return false;  // Expired
    }
    
    // Cache hit
    cache_hits_++;
    out_key = it->second.key;
    it->second.access_count++;
    it->second.last_access_ms = now;
    
    return true;
}

/**
 * @brief Put.
 * @param[in] key_id Identifier of the key.
 * @param[in] version Input parameter.
 * @param[in] key Input parameter.
 * @details Calls: lock(), evictExpired(), size(), evictLRU(), makeCacheKey(), getCurrentTimeMs(), std::move().
 */
void KeyCache::put(const std::string& key_id, uint32_t version, const std::vector<uint8_t>& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    evictExpired();
    
    // Evict LRU if cache is full
    if (cache_.size() >= max_size_) {
        evictLRU();
    }
    
    std::string cache_key = makeCacheKey(key_id, version);
    int64_t now = getCurrentTimeMs();
    
    CacheEntry entry;
    entry.key = key;
    entry.expires_at_ms = now + ttl_ms_;
    entry.access_count = 0;
    entry.last_access_ms = now;
    
    cache_[cache_key] = std::move(entry);
}

/**
 * @brief Evict.
 * @param[in] key_id Identifier of the key.
 * @param[in] version Input parameter.
 * @details Calls: lock(), begin(), end(), find(), erase(), makeCacheKey().
 */
void KeyCache::evict(const std::string& key_id, uint32_t version) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (version == 0) {
        // Evict all versions
        auto it = cache_.begin();
        while (it != cache_.end()) {
            if (it->first.find(key_id + ":") == 0) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        // Evict specific version
        std::string cache_key = makeCacheKey(key_id, version);
        cache_.erase(cache_key);
    }
}

/**
 * @brief Clear.
 * @details Calls: lock().
 */
void KeyCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    total_requests_ = 0;
    cache_hits_ = 0;
}

double KeyCache::getHitRate() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (total_requests_ == 0) {
        return 0.0;
    }
    
    return static_cast<double>(cache_hits_) / total_requests_;
}

size_t KeyCache::size() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.size();
}

std::string KeyCache::makeCacheKey(const std::string& key_id, uint32_t version) const {
    return key_id + ":" + std::to_string(version);
}

/**
 * @brief Evict Expired.
 * @details Calls: getCurrentTimeMs(), begin(), end(), erase().
 */
void KeyCache::evictExpired() {
    int64_t now = getCurrentTimeMs();
    
    auto it = cache_.begin();
    while (it != cache_.end()) {
        if (now > it->second.expires_at_ms) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * @brief Evict LRU.
 * @details Calls: empty(), begin(), end(), erase().
 */
void KeyCache::evictLRU() {
    if (cache_.empty()) {
        return;
    }
    
    // Find least recently used entry
    auto lru_it = cache_.begin();
    int64_t oldest_access = lru_it->second.last_access_ms;
    
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.last_access_ms < oldest_access) {
            oldest_access = it->second.last_access_ms;
            lru_it = it;
        }
    }
    
    cache_.erase(lru_it);
}

int64_t KeyCache::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

}  // namespace themis
