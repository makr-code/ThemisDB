#include "cache/adaptive_query_cache.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/zstd_codec.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <thread>
#include <openssl/sha.h>
#include <regex>

namespace themis {

// Constants
constexpr size_t QUERY_CACHE_PREFIX_LEN = 12;  // Length of "query_cache:"
constexpr const char* QUERY_CACHE_PREFIX = "query_cache:";

AdaptiveQueryCache::AdaptiveQueryCache(const Config& config)
    : config_(config) {
    
    THEMIS_INFO("AdaptiveQueryCache initialized: L1={} entries, L2={} entries, L3=RocksDB",
                config_.l1_max_entries, config_.l2_max_entries);
    
    // Initialize circuit breaker for L3 (Phase 1: Fault Isolation)
    if (config_.enable_circuit_breaker) {
        cache::CircuitBreaker::Config cb_config;
        cb_config.failure_threshold = config_.cb_failure_threshold;
        cb_config.timeout_ms = config_.cb_timeout_ms;
        l3_circuit_breaker_ = std::make_unique<cache::CircuitBreaker>(cb_config);
        THEMIS_INFO("Circuit breaker enabled for L3 cache (threshold={}, timeout={}ms)",
                    cb_config.failure_threshold, cb_config.timeout_ms);
    }
    
    // Initialize L3 (RocksDB) cache with retry logic
    int retry_count = 0;
    int max_retries = 3;
    int retry_delay_ms = 1000;  // Start with 1 second
    
    while (retry_count < max_retries) {
        try {
            RocksDBWrapper::Config db_config;
            db_config.db_path = config_.l3_db_path;
            db_config.create_if_missing = true;
            db_config.memtable_size_mb = 64;      // 64MB write buffer
            db_config.block_cache_size_mb = 256;  // small cache for query cache
            db_config.max_background_jobs = 2;
            
            l3_db_ = std::make_unique<RocksDBWrapper>(db_config);
            THEMIS_INFO("L3 cache (RocksDB) initialized at: {}", config_.l3_db_path);
            break;  // Success
        } catch (const std::exception& e) {
            retry_count++;
            if (retry_count < max_retries) {
                THEMIS_WARN("Failed to initialize L3 cache (attempt {}/{}): {}. Retrying in {}ms...",
                           retry_count, max_retries, e.what(), retry_delay_ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
                retry_delay_ms *= 2;  // Exponential backoff
            } else {
                THEMIS_WARN("Failed to initialize L3 cache after {} attempts: {}. L3 cache disabled.",
                           max_retries, e.what());
                l3_db_.reset();
                if (l3_circuit_breaker_) {
                    l3_circuit_breaker_->recordFailure();
                    enhanced_metrics_.l3_circuit_breaker_trips++;
                }
            }
        }
    }
}

AdaptiveQueryCache::~AdaptiveQueryCache() {
    clear();
}

std::string AdaptiveQueryCache::generateFingerprint(
    const std::string& query,
    const nlohmann::json& params
) const {
    // Concatenate query + params for hashing
    std::string input = query;
    if (!params.empty()) {
        input += "::" + params.dump();
    }
    
    // Compute SHA256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), 
           input.size(), hash);
    
    // Convert to hex string
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

std::optional<AdaptiveQueryCache::CacheEntry> AdaptiveQueryCache::get(
    const std::string& fingerprint
) {
    int64_t now_ms = getCurrentTimeMs();
    
    // Try L1 (HOT) - fastest path
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        auto it = l1_cache_.find(fingerprint);
        if (it != l1_cache_.end()) {
            auto& entry = it->second;
            
            // Check expiration
            if (isExpired(entry.created_at_ms, entry.ttl_seconds)) {
                l1_cache_.erase(it);
                stats_.evictions++;
                enhanced_metrics_.evictions++;
            } else {
                // Cache hit!
                entry.last_accessed_ms = now_ms;
                entry.access_count++;
                stats_.l1_hits++;
                enhanced_metrics_.l1_hits++;
                
                // Return entry
                CacheEntry result;
                result.query_fingerprint = fingerprint;
                result.result = entry.result;
                result.level = CacheLevel::HOT;
                result.created_at_ms = entry.created_at_ms;
                result.last_accessed_ms = entry.last_accessed_ms;
                result.access_count = entry.access_count;
                result.ttl_seconds = entry.ttl_seconds;
                
                THEMIS_DEBUG("L1 cache hit: fingerprint={}", fingerprint.substr(0, 16));
                return result;
            }
        }
    }
    
    // Try L2 (WARM) - compressed
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        auto it = l2_cache_.find(fingerprint);
        if (it != l2_cache_.end()) {
            auto& entry = it->second;
            
            // Check expiration
            if (isExpired(entry.created_at_ms, entry.ttl_seconds)) {
                l2_cache_.erase(it);
                stats_.evictions++;
                enhanced_metrics_.evictions++;
            } else {
                // Decompress result
                auto decompressed = utils::zstd_decompress(entry.compressed_result);
                if (!decompressed.empty()) {
                    std::string json_str(decompressed.begin(), decompressed.end());
                    nlohmann::json result = nlohmann::json::parse(json_str);
                    
                    // Update stats
                    entry.last_accessed_ms = now_ms;
                    entry.access_count++;
                    stats_.l2_hits++;
                    enhanced_metrics_.l2_hits++;
                    
                    // Promote to L1 if accessed frequently
                    if (entry.access_count >= 3 && decompressed.size() < config_.l1_max_entry_size) {
                        L1Entry l1_entry;
                        l1_entry.result = result;
                        l1_entry.created_at_ms = entry.created_at_ms;
                        l1_entry.last_accessed_ms = now_ms;
                        l1_entry.access_count = entry.access_count;
                        l1_entry.ttl_seconds = entry.ttl_seconds;
                        
                        std::lock_guard<std::mutex> l1_lock(l1_mutex_);
                        if (l1_cache_.size() >= config_.l1_max_entries) {
                            evictLRU(CacheLevel::HOT);
                        }
                        l1_cache_[fingerprint] = std::move(l1_entry);
                        l2_cache_.erase(it);
                        stats_.promotions++;
                        enhanced_metrics_.promotions++;
                        
                        THEMIS_DEBUG("Promoted L2->L1: fingerprint={}", fingerprint.substr(0, 16));
                    }
                    
                    // Return entry
                    CacheEntry cache_entry;
                    cache_entry.query_fingerprint = fingerprint;
                    cache_entry.result = result;
                    cache_entry.level = CacheLevel::WARM;
                    cache_entry.created_at_ms = entry.created_at_ms;
                    cache_entry.last_accessed_ms = entry.last_accessed_ms;
                    cache_entry.access_count = entry.access_count;
                    cache_entry.ttl_seconds = entry.ttl_seconds;
                    
                    THEMIS_DEBUG("L2 cache hit: fingerprint={}", fingerprint.substr(0, 16));
                    return cache_entry;
                } else {
                    // Decompression failed
                    THEMIS_WARN("Failed to decompress L2 cache entry");
                    enhanced_metrics_.decompression_failures++;
                    l2_cache_.erase(it);
                }
            }
        }
    }
    
    // Try L3 (COLD) - RocksDB
    if (l3_db_) {
        // Phase 1: Check circuit breaker before L3 operation
        if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
            THEMIS_DEBUG("L3 cache circuit breaker is open, skipping L3 lookup");
            enhanced_metrics_.l3_circuit_breaker_open = true;
            stats_.misses++;
            return std::nullopt;
        }
        
        std::lock_guard<std::mutex> lock(l3_mutex_);
        
        std::string key = QUERY_CACHE_PREFIX + fingerprint;
        std::optional<std::vector<uint8_t>> result;
        
        try {
            result = l3_db_->get(key);
        } catch (const std::exception& e) {
            THEMIS_WARN("L3 cache read exception: {}", e.what());
            enhanced_metrics_.l3_read_errors++;
            if (l3_circuit_breaker_) {
                l3_circuit_breaker_->recordFailure();
                if (l3_circuit_breaker_->isOpen()) {
                    enhanced_metrics_.l3_circuit_breaker_trips++;
                    enhanced_metrics_.l3_circuit_breaker_open = true;
                }
            }
            stats_.misses++;
            return std::nullopt;
        }
        
        if (result && !result->empty()) {
            try {
            std::string json_str(result->begin(), result->end());
            nlohmann::json entry_json = nlohmann::json::parse(json_str);
                
                int64_t created_at_ms = entry_json["created_at_ms"];
                int ttl_seconds = entry_json["ttl_seconds"];
                
                // Check expiration
                if (isExpired(created_at_ms, ttl_seconds)) {
                    l3_db_->del(key);
                    stats_.evictions++;
                    enhanced_metrics_.evictions++;
                } else {
                    // Update access stats
                    int64_t access_count = entry_json["access_count"].get<int64_t>() + 1;
                    entry_json["access_count"] = access_count;
                    entry_json["last_accessed_ms"] = now_ms;
                    l3_db_->put(key, entry_json.dump());
                    
                    stats_.l3_hits++;
                    enhanced_metrics_.l3_hits++;
                    
                    // Phase 1: Record success for circuit breaker
                    if (l3_circuit_breaker_) {
                        l3_circuit_breaker_->recordSuccess();
                        enhanced_metrics_.l3_circuit_breaker_open = false;
                    }
                    
                    // Return entry
                    CacheEntry cache_entry;
                    cache_entry.query_fingerprint = fingerprint;
                    cache_entry.result = entry_json["result"];
                    cache_entry.level = CacheLevel::COLD;
                    cache_entry.created_at_ms = created_at_ms;
                    cache_entry.last_accessed_ms = now_ms;
                    cache_entry.access_count = access_count;
                    cache_entry.ttl_seconds = ttl_seconds;
                    
                    THEMIS_DEBUG("L3 cache hit: fingerprint={}", fingerprint.substr(0, 16));
                    return cache_entry;
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to parse L3 cache entry: {}", e.what());
                l3_db_->del(key);
            }
        }
    }
    
    // Cache miss
    stats_.misses++;
    enhanced_metrics_.misses++;
    THEMIS_DEBUG("Cache miss: fingerprint={}", fingerprint.substr(0, 16));
    return std::nullopt;
}

bool AdaptiveQueryCache::put(
    const std::string& fingerprint,
    const nlohmann::json& query_params,
    const nlohmann::json& result
) {
    int64_t now_ms = getCurrentTimeMs();
    std::string result_str = result.dump();
    size_t result_size = result_str.size();
    
    // Phase 1: Validate entry size
    if (config_.enable_size_limits && !isWithinSizeLimit(result_size)) {
        THEMIS_WARN("Rejected cache entry due to size limit: size={}, max={}",
                   result_size, config_.max_total_entry_size);
        enhanced_metrics_.size_limit_rejections++;
        return false;
    }
    
    // Select cache level based on size
    CacheLevel level = selectCacheLevel(result_size);
    
    // Phase 1: Validate size for selected level
    if (!validateEntrySize(result_size, level)) {
        THEMIS_WARN("Entry size {} exceeds limit for cache level", result_size);
        enhanced_metrics_.size_limit_rejections++;
        return false;
    }
    
    // Calculate adaptive TTL (if enabled)
    int ttl_seconds;
    if (level == CacheLevel::HOT) {
        ttl_seconds = config_.l1_ttl_seconds;
    } else if (level == CacheLevel::WARM) {
        ttl_seconds = config_.l2_ttl_seconds;
    } else {
        ttl_seconds = config_.l3_ttl_seconds;
    }
    
    // Store in appropriate level
    if (level == CacheLevel::HOT && result_size < config_.l1_max_entry_size) {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        
        // Evict LRU if full
        if (l1_cache_.size() >= config_.l1_max_entries) {
            evictLRU(CacheLevel::HOT);
        }
        
        L1Entry entry;
        entry.result = result;
        entry.created_at_ms = now_ms;
        entry.last_accessed_ms = now_ms;
        entry.access_count = 1;
        entry.ttl_seconds = ttl_seconds;
        
        l1_cache_[fingerprint] = std::move(entry);
        enhanced_metrics_.total_bytes_cached += result_size;
        THEMIS_DEBUG("Stored in L1: fingerprint={}, size={}", fingerprint.substr(0, 16), result_size);
        return true;
        
    } else if (level == CacheLevel::WARM && result_size < config_.l2_max_entry_size) {
        // Compress with Zstd
        auto compressed = utils::zstd_compress(result_str, config_.l2_compression_level);
        if (compressed.empty()) {
            THEMIS_WARN("Failed to compress result for L2 cache");
            enhanced_metrics_.compression_failures++;
            return false;
        }
        
        std::lock_guard<std::mutex> lock(l2_mutex_);
        
        // Evict LRU if full
        if (l2_cache_.size() >= config_.l2_max_entries) {
            evictLRU(CacheLevel::WARM);
        }
        
        L2Entry entry;
        entry.compressed_result = std::move(compressed);
        entry.created_at_ms = now_ms;
        entry.last_accessed_ms = now_ms;
        entry.access_count = 1;
        entry.ttl_seconds = ttl_seconds;
        
        size_t compressed_size = entry.compressed_result.size();
        l2_cache_[fingerprint] = std::move(entry);
        enhanced_metrics_.total_bytes_cached += result_size;
        enhanced_metrics_.total_bytes_compressed += compressed_size;
        THEMIS_DEBUG("Stored in L2: fingerprint={}, size={}, compressed={}",
                    fingerprint.substr(0, 16), result_size, compressed_size);
        return true;
        
    } else if (l3_db_) {
        // Phase 1: Check circuit breaker before L3 operation
        if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
            THEMIS_WARN("L3 cache circuit breaker is open, rejecting write");
            enhanced_metrics_.l3_circuit_breaker_open = true;
            return false;
        }
        
        // Store in L3 (RocksDB)
        std::lock_guard<std::mutex> lock(l3_mutex_);
        
        nlohmann::json entry_json;
        entry_json["result"] = result;
        entry_json["query_params"] = query_params;
        entry_json["created_at_ms"] = now_ms;
        entry_json["last_accessed_ms"] = now_ms;
        entry_json["access_count"] = 1;
        entry_json["ttl_seconds"] = ttl_seconds;
        
        std::string key = QUERY_CACHE_PREFIX + fingerprint;
        bool ok = false;
        
        try {
            ok = l3_db_->put(key, entry_json.dump());
            
            if (ok) {
                // Phase 1: Record success for circuit breaker
                if (l3_circuit_breaker_) {
                    l3_circuit_breaker_->recordSuccess();
                    enhanced_metrics_.l3_circuit_breaker_open = false;
                }
                enhanced_metrics_.total_bytes_cached += result_size;
                THEMIS_DEBUG("Stored in L3: fingerprint={}, size={}", fingerprint.substr(0, 16), result_size);
                return true;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("L3 cache write exception: {}", e.what());
            enhanced_metrics_.l3_write_errors++;
            if (l3_circuit_breaker_) {
                l3_circuit_breaker_->recordFailure();
                if (l3_circuit_breaker_->isOpen()) {
                    enhanced_metrics_.l3_circuit_breaker_trips++;
                    enhanced_metrics_.l3_circuit_breaker_open = true;
                }
            }
            return false;
        }
        
        // Failed to write
        enhanced_metrics_.l3_write_errors++;
        if (l3_circuit_breaker_) {
            l3_circuit_breaker_->recordFailure();
            if (l3_circuit_breaker_->isOpen()) {
                enhanced_metrics_.l3_circuit_breaker_trips++;
                enhanced_metrics_.l3_circuit_breaker_open = true;
            }
        }
        THEMIS_WARN("Failed to store in L3 cache");
        return false;
    }
    
    return false;
}

size_t AdaptiveQueryCache::invalidate(const std::string& pattern) {
    size_t count = 0;
    std::regex re(pattern);
    
    // Invalidate L1
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
            if (std::regex_search(it->first, re)) {
                it = l1_cache_.erase(it);
                count++;
            } else {
                ++it;
            }
        }
    }
    
    // Invalidate L2
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
            if (std::regex_search(it->first, re)) {
                it = l2_cache_.erase(it);
                count++;
            } else {
                ++it;
            }
        }
    }
    
    // Phase 1: Invalidate L3 with proper iterator-based pattern matching
    if (l3_db_) {
        std::lock_guard<std::mutex> lock(l3_mutex_);
        
        // Phase 1: Check circuit breaker before L3 operation
        if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
            THEMIS_WARN("L3 cache circuit breaker is open, skipping L3 invalidation");
            enhanced_metrics_.l3_circuit_breaker_open = true;
        } else {
            try {
                // Use RocksDB iterator to scan all cache entries
                std::vector<std::string> keys_to_delete;
                l3_db_->scanPrefix(QUERY_CACHE_PREFIX, [&](std::string_view key, std::string_view) {
                    // Extract fingerprint from key (remove prefix)
                    std::string fingerprint(key.substr(QUERY_CACHE_PREFIX_LEN));
                    if (std::regex_search(fingerprint, re)) {
                        keys_to_delete.emplace_back(key);
                    }
                    return true;  // Continue iteration
                });
                
                // Delete matched keys
                for (const auto& key : keys_to_delete) {
                    l3_db_->del(key);
                    count++;
                }
                
                if (l3_circuit_breaker_) {
                    l3_circuit_breaker_->recordSuccess();
                    enhanced_metrics_.l3_circuit_breaker_open = false;
                }
                
                THEMIS_DEBUG("Invalidated {} L3 cache entries", keys_to_delete.size());
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to invalidate L3 cache entries: {}", e.what());
                enhanced_metrics_.l3_read_errors++;
                if (l3_circuit_breaker_) {
                    l3_circuit_breaker_->recordFailure();
                    if (l3_circuit_breaker_->isOpen()) {
                        enhanced_metrics_.l3_circuit_breaker_trips++;
                        enhanced_metrics_.l3_circuit_breaker_open = true;
                    }
                }
            }
        }
    }
    
    THEMIS_INFO("Invalidated {} cache entries matching pattern: {}", count, pattern);
    return count;
}

void AdaptiveQueryCache::clear() {
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        l1_cache_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        l2_cache_.clear();
    }
    
    if (l3_db_) {
        std::lock_guard<std::mutex> lock(l3_mutex_);
        // Clear L3 by deleting all keys with prefix
        // Note: Simplified implementation
        try {
            std::vector<std::string> keys;
            l3_db_->scanPrefix(QUERY_CACHE_PREFIX, [&keys](std::string_view key, std::string_view) {
                keys.emplace_back(key);
                return true;
            });
            for (const auto& key : keys) {
                l3_db_->del(key);
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to clear L3 cache: {}", e.what());
        }
    }
    
    THEMIS_INFO("Cache cleared");
}

uint64_t AdaptiveQueryCache::clearExpired() {
    uint64_t count = 0;
    int64_t now_ms = getCurrentTimeMs();
    
    // Clear expired L1 entries
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
            if (isExpired(it->second.created_at_ms, it->second.ttl_seconds)) {
                it = l1_cache_.erase(it);
                count++;
            } else {
                ++it;
            }
        }
    }
    
    // Clear expired L2 entries
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
            if (isExpired(it->second.created_at_ms, it->second.ttl_seconds)) {
                it = l2_cache_.erase(it);
                count++;
            } else {
                ++it;
            }
        }
    }
    
    stats_.evictions += count;
    
    if (count > 0) {
        THEMIS_INFO("Cleared {} expired cache entries", count);
    }
    
    return count;
}

AdaptiveQueryCache::CacheStats AdaptiveQueryCache::getStats() const {
    return stats_;
}

nlohmann::json AdaptiveQueryCache::getDetailedInfo() const {
    nlohmann::json info;
    
    info["stats"] = {
        {"l1_hits", stats_.l1_hits},
        {"l2_hits", stats_.l2_hits},
        {"l3_hits", stats_.l3_hits},
        {"misses", stats_.misses},
        {"hit_rate", stats_.getHitRate()},
        {"l1_hit_rate", stats_.getL1HitRate()},
        {"evictions", stats_.evictions},
        {"promotions", stats_.promotions},
        {"demotions", stats_.demotions}
    };
    
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        info["l1"] = {
            {"entries", l1_cache_.size()},
            {"max_entries", config_.l1_max_entries},
            {"utilization", static_cast<double>(l1_cache_.size()) / config_.l1_max_entries}
        };
    }
    
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        info["l2"] = {
            {"entries", l2_cache_.size()},
            {"max_entries", config_.l2_max_entries},
            {"utilization", static_cast<double>(l2_cache_.size()) / config_.l2_max_entries}
        };
    }
    
    info["l3"] = {
        {"enabled", l3_db_ != nullptr},
        {"path", config_.l3_db_path}
    };
    
    return info;
}

// Private helper methods

int64_t AdaptiveQueryCache::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

bool AdaptiveQueryCache::isExpired(int64_t created_at_ms, int ttl_seconds) const {
    int64_t now_ms = getCurrentTimeMs();
    int64_t expiry_ms = created_at_ms + (ttl_seconds * 1000LL);
    return now_ms > expiry_ms;
}

int AdaptiveQueryCache::calculateAdaptiveTTL(int64_t access_count) const {
    if (!config_.enable_adaptive_ttl) {
        return config_.l1_ttl_seconds;
    }
    
    // More frequently accessed entries get longer TTL
    // Formula: TTL = base + (access_count * scale), clamped to [min, max]
    int base_ttl = config_.l1_ttl_seconds;
    int ttl = base_ttl + static_cast<int>(access_count * 60);  // +1 minute per access
    
    ttl = std::max(ttl, config_.min_ttl_seconds);
    ttl = std::min(ttl, config_.max_ttl_seconds);
    
    return ttl;
}

AdaptiveQueryCache::CacheLevel AdaptiveQueryCache::selectCacheLevel(size_t result_size) const {
    if (result_size < config_.l1_max_entry_size) {
        return CacheLevel::HOT;
    } else if (result_size < config_.l2_max_entry_size) {
        return CacheLevel::WARM;
    } else {
        return CacheLevel::COLD;
    }
}

void AdaptiveQueryCache::evictLRU(CacheLevel level) {
    if (level == CacheLevel::HOT) {
        // Find LRU entry in L1
        if (l1_cache_.empty()) return;
        
        auto lru_it = l1_cache_.begin();
        double min_score = calculateLRUScore(lru_it->second.last_accessed_ms, 
                                             lru_it->second.access_count);
        
        for (auto it = l1_cache_.begin(); it != l1_cache_.end(); ++it) {
            double score = calculateLRUScore(it->second.last_accessed_ms, 
                                            it->second.access_count);
            if (score < min_score) {
                min_score = score;
                lru_it = it;
            }
        }
        
        l1_cache_.erase(lru_it);
        stats_.evictions++;
        enhanced_metrics_.evictions++;
        
    } else if (level == CacheLevel::WARM) {
        // Find LRU entry in L2
        if (l2_cache_.empty()) return;
        
        auto lru_it = l2_cache_.begin();
        double min_score = calculateLRUScore(lru_it->second.last_accessed_ms,
                                             lru_it->second.access_count);
        
        for (auto it = l2_cache_.begin(); it != l2_cache_.end(); ++it) {
            double score = calculateLRUScore(it->second.last_accessed_ms,
                                            it->second.access_count);
            if (score < min_score) {
                min_score = score;
                lru_it = it;
            }
        }
        
        l2_cache_.erase(lru_it);
        stats_.evictions++;
        enhanced_metrics_.evictions++;
    }
}

double AdaptiveQueryCache::calculateLRUScore(int64_t last_accessed_ms, int64_t access_count) const {
    int64_t now_ms = getCurrentTimeMs();
    int64_t age_ms = now_ms - last_accessed_ms;
    
    // Score = frequency_weight * access_count - (1 - frequency_weight) * age
    // Higher score = more valuable entry
    double score = config_.frequency_weight * access_count
                   - (1.0 - config_.frequency_weight) * (age_ms / 1000.0);
    
    return score;
}

// Phase 1: Size validation helpers
bool AdaptiveQueryCache::isWithinSizeLimit(size_t size) const {
    return size <= config_.max_total_entry_size;
}

bool AdaptiveQueryCache::validateEntrySize(size_t size, CacheLevel level) const {
    switch (level) {
        case CacheLevel::HOT:
            return size <= config_.l1_max_entry_size;
        case CacheLevel::WARM:
            return size <= config_.l2_max_entry_size;
        case CacheLevel::COLD:
            return size <= config_.max_total_entry_size;
        default:
            return false;
    }
}

} // namespace themis
