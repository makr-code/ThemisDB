/**
 * @file query_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Query Result Caching System Implementation
#include "query/query_cache.h"
#include "utils/logger.h"
#include <openssl/sha.h>
#include <algorithm>

namespace themis {
namespace query {

QueryCache::QueryCache(const Config& config)
    : config_(config) {
    THEMIS_INFO("QueryCache initialized: max_entries={}, max_memory={}MB, policy={}",
                config_.max_entries,
                config_.max_memory_bytes / (1024 * 1024),
                config_.eviction_policy == EvictionPolicy::LRU ? "LRU" : "LFU");
}

QueryCache::~QueryCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
    lru_list_.clear();
    THEMIS_DEBUG("QueryCache destroyed: {} total requests, {:.2f}% hit rate",
                stats_.total_requests, stats_.hitRate() * 100.0);
}

std::string QueryCache::generateFingerprint(
    const std::string& query,
    const nlohmann::json& params
) const {
    // Optimized: Concatenate query + params for hashing using StringBuilder pattern
    // Pre-estimate size to reduce allocations (avoid repeated reallocations)
    std::string params_json = (!params.empty() && !params.is_null()) ? params.dump() : "";
    size_t total_size = query.size() + (params_json.empty() ? 0 : (2 + params_json.size()));
    
    std::string input = {};
    input.reserve(total_size);  // Reserve capacity once to eliminate reallocations
    input.append(query);
    if (!params_json.empty()) {
        input.append("::");
        input.append(params_json);
    }
    
    // Compute SHA256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()),
           input.size(), hash);
    
    // Convert to hex string
    std::string hex = {};
    hex.reserve(SHA256_DIGEST_LENGTH * 2);
    const char digits[] = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hex.push_back(digits[(hash[i] >> 4) & 0xf]);
        hex.push_back(digits[hash[i] & 0xf]);
    }

    return hex;
}

Result<void> QueryCache::put(
    const std::string& query,
    const nlohmann::json& params,
    const nlohmann::json& result,
    const std::vector<std::string>& dependencies,
    std::optional<std::chrono::seconds> ttl
) {
    // Validate input
    if (query.empty()) {
        return Err<void>(errors::ErrorCode::ERR_QUERY_INVALID, "Query string cannot be empty");
    }
    
    // Generate fingerprint
    std::string fingerprint = generateFingerprint(query, params);
    
    // Create cache entry
    CacheEntry entry;
    entry.query_fingerprint = fingerprint;
    entry.original_query = query;
    entry.query_params = params;
    entry.result = result;
    entry.created_at = std::chrono::system_clock::now();
    entry.last_accessed = entry.created_at;
    entry.access_count = 1;
    entry.ttl = ttl.value_or(config_.default_ttl);
    entry.dependencies = dependencies;
    
    // Estimate size
    entry.result_size_bytes = estimateEntrySize(entry);
    
    // Check if entry is too large
    if (entry.result_size_bytes > config_.max_entry_size) {
        THEMIS_WARN("Query result too large to cache: {} bytes (max: {} bytes)",
                   entry.result_size_bytes, config_.max_entry_size);
        return Err<void>(errors::ErrorCode::ERR_CACHE_ENTRY_TOO_LARGE,
                        "Result size " + std::to_string(entry.result_size_bytes) + 
                        " exceeds max entry size " + std::to_string(config_.max_entry_size));
    }
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check if we need to evict entries
    while (shouldEvict() && !cache_.empty()) {
        evictOne();
    }
    
    // Additional check after eviction - if still too large, reject
    if (stats_.current_memory_bytes + entry.result_size_bytes > config_.max_memory_bytes) {
        THEMIS_WARN("Cannot cache entry: would exceed memory limit");
        return Err<void>(errors::ErrorCode::ERR_CACHE_FULL,
                        "Cache memory limit reached");
    }
    
    // Remove old entry if exists (update case)
    auto it = cache_.find(fingerprint);
    if (it != cache_.end()) {
        // Remove from LRU list
        lru_list_.erase(it->second.lru_it);
        // Remove from dependency index
        removeFromDependencyIndex(fingerprint, it->second.entry.dependencies);
        // Update memory stats
        stats_.current_memory_bytes -= it->second.entry.result_size_bytes;
        cache_.erase(it);
        stats_.current_entries--;
    }
    
    // Add to LRU list (at front = most recent)
    lru_list_.push_front(fingerprint);
    
    // Create internal entry
    InternalCacheEntry internal_entry(std::move(entry));
    internal_entry.lru_it = lru_list_.begin();
    
    // Add to cache
    cache_.emplace(fingerprint, std::move(internal_entry));
    
    // Get reference to the newly inserted entry
    auto& inserted_entry = cache_[fingerprint].entry;
    
    // Add to dependency index
    addToDependencyIndex(fingerprint, dependencies);
    
    // Update stats
    stats_.current_entries++;
    stats_.current_memory_bytes += inserted_entry.result_size_bytes;
    
    THEMIS_DEBUG("Cached query: fingerprint={}, size={} bytes, deps={}", 
                fingerprint.substr(0, 16), inserted_entry.result_size_bytes, dependencies.size());
    
    return OkVoid();
}

Result<QueryCache::LookupResult> QueryCache::get(
    const std::string& query,
    const nlohmann::json& params
) {
    std::string fingerprint = generateFingerprint(query, params);
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = cache_.find(fingerprint);
    if (it == cache_.end()) {
        // Cache miss
        updateStats(false);
        THEMIS_DEBUG("Cache miss: fingerprint={}", fingerprint.substr(0, 16));
        return Ok(LookupResult(false));
    }
    
    auto& internal_entry = it->second;
    auto& entry = internal_entry.entry;
    
    // Check expiration
    if (config_.enable_ttl && entry.isExpired()) {
        THEMIS_DEBUG("Cache entry expired: fingerprint={}", fingerprint.substr(0, 16));
        
        // Remove from LRU list
        lru_list_.erase(internal_entry.lru_it);
        // Remove from dependency index
        removeFromDependencyIndex(fingerprint, entry.dependencies);
        // Update stats
        stats_.current_memory_bytes -= entry.result_size_bytes;
        stats_.current_entries--;
        stats_.expirations++;
        
        // Remove from cache
        cache_.erase(it);
        
        updateStats(false);
        return Ok(LookupResult(false));
    }
    
    // Cache hit!
    updateStats(true);
    
    // Update access metadata
    entry.last_accessed = std::chrono::system_clock::now();
    entry.access_count++;
    
    // Update LRU position (move to front)
    updateLRU(fingerprint);
    
    // Prepare result
    LookupResult result(true);
    result.result = entry.result;
    result.query_fingerprint = fingerprint;
    
    THEMIS_DEBUG("Cache hit: fingerprint={}, access_count={}", 
                fingerprint.substr(0, 16), entry.access_count);
    
    return Ok(result);
}

Result<size_t> QueryCache::invalidateByDependency(const std::string& dependency) {
    std::vector<std::string> to_invalidate;
    
    // Find all fingerprints with this dependency
    {
        std::lock_guard<std::mutex> lock(dependency_mutex_);
        auto it = dependency_index_.find(dependency);
        if (it != dependency_index_.end()) {
            to_invalidate = it->second;
            dependency_index_.erase(it);
        }
    }
    
    if (to_invalidate.empty()) {
        return Ok<size_t>(0);
    }
    
    // Remove from cache
    size_t count = 0;
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (const auto& fingerprint : to_invalidate) {
            auto it = cache_.find(fingerprint);
            if (it != cache_.end()) {
                // Remove from LRU list
                lru_list_.erase(it->second.lru_it);
                // Update stats
                stats_.current_memory_bytes -= it->second.entry.result_size_bytes;
                stats_.current_entries--;
                stats_.invalidations++;
                // Remove from cache
                cache_.erase(it);
                count++;
            }
        }
    }
    
    THEMIS_INFO("Invalidated {} cache entries for dependency: {}", count, dependency);
    return Ok(count);
}

Result<bool> QueryCache::invalidate(
    const std::string& query,
    const nlohmann::json& params
) {
    std::string fingerprint = generateFingerprint(query, params);
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = cache_.find(fingerprint);
    if (it == cache_.end()) {
        return Ok(false);
    }
    
    // Remove from LRU list
    lru_list_.erase(it->second.lru_it);
    
    // Remove from dependency index
    removeFromDependencyIndex(fingerprint, it->second.entry.dependencies);
    
    // Update stats
    stats_.current_memory_bytes -= it->second.entry.result_size_bytes;
    stats_.current_entries--;
    stats_.invalidations++;
    
    // Remove from cache
    cache_.erase(it);
    
    THEMIS_DEBUG("Invalidated cache entry: fingerprint={}", fingerprint.substr(0, 16));
    return Ok(true);
}

Result<void> QueryCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    size_t old_count = cache_.size();
    
    cache_.clear();
    lru_list_.clear();
    
    {
        std::lock_guard<std::mutex> dep_lock(dependency_mutex_);
        dependency_index_.clear();
    }
    
    stats_.current_entries = 0;
    stats_.current_memory_bytes = 0;
    
    THEMIS_INFO("Cache cleared: removed {} entries", old_count);
    return OkVoid();
}

Result<size_t> QueryCache::clearExpired() {
    if (!config_.enable_ttl) {
        return Ok<size_t>(0);
    }
    
    std::vector<std::string> to_remove;
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        for (const auto& [fingerprint, internal_entry] : cache_) {
            if (internal_entry.entry.isExpired()) {
                to_remove.push_back(fingerprint);
            }
        }
        
        // Remove expired entries
        for (const auto& fingerprint : to_remove) {
            auto it = cache_.find(fingerprint);
            if (it != cache_.end()) {
                // Remove from LRU list
                lru_list_.erase(it->second.lru_it);
                // Remove from dependency index
                removeFromDependencyIndex(fingerprint, it->second.entry.dependencies);
                // Update stats
                stats_.current_memory_bytes -= it->second.entry.result_size_bytes;
                stats_.current_entries--;
                stats_.expirations++;
                // Remove from cache
                cache_.erase(it);
            }
        }
    }
    
    if (!to_remove.empty()) {
        THEMIS_DEBUG("Cleared {} expired cache entries", to_remove.size());
    }
    
    return Ok<size_t>(to_remove.size());
}

QueryCache::CacheStats QueryCache::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

nlohmann::json QueryCache::getDetailedInfo() const {
    nlohmann::json info;
    
    auto stats = getStats();
    
    info["statistics"] = {
        {"total_requests", stats.total_requests},
        {"hits", stats.hits},
        {"misses", stats.misses},
        {"hit_rate", stats.hitRate()},
        {"evictions", stats.evictions},
        {"expirations", stats.expirations},
        {"invalidations", stats.invalidations}
    };
    
    info["memory"] = {
        {"current_entries", stats.current_entries},
        {"current_memory_bytes", stats.current_memory_bytes},
        {"current_memory_mb", stats.current_memory_bytes / (1024.0 * 1024.0)},
        {"max_entries", config_.max_entries},
        {"max_memory_bytes", config_.max_memory_bytes},
        {"max_memory_mb", config_.max_memory_bytes / (1024.0 * 1024.0)},
        {"memory_utilization", stats.memoryUtilization(config_.max_memory_bytes)}
    };
    
    info["configuration"] = {
        {"eviction_policy", config_.eviction_policy == EvictionPolicy::LRU ? "LRU" : "LFU"},
        {"default_ttl_seconds", config_.default_ttl.count()},
        {"enable_ttl", config_.enable_ttl},
        {"max_entry_size_bytes", config_.max_entry_size},
        {"memory_pressure_threshold", config_.memory_pressure_threshold}
    };
    
    return info;
}

void QueryCache::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_requests = 0;
    stats_.hits = 0;
    stats_.misses = 0;
    stats_.evictions = 0;
    stats_.expirations = 0;
    stats_.invalidations = 0;
    // Don't reset current_entries and current_memory_bytes
}

Result<void> QueryCache::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    config_ = config;
    
    // If new limits are lower, may need to evict
    while (shouldEvict() && !cache_.empty()) {
        evictOne();
    }
    
    THEMIS_INFO("QueryCache configuration updated");
    return OkVoid();
}

QueryCache::Config QueryCache::getConfig() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return config_;
}

// Private helper methods

void QueryCache::evictLRU() {
    if (lru_list_.empty() || cache_.empty()) {
        return;
    }
    
    // LRU = least recently used = back of list
    std::string fingerprint = lru_list_.back();
    
    auto it = cache_.find(fingerprint);
    if (it != cache_.end()) {
        // [W9-10-FIX: todo_as_productionlogic — query_cache.cpp:439]
        // Dependency index cleanup is performed synchronously here (inline,
        // under the caller-held write lock).  The alternative — an async
        // std::async / thread-pool dispatch — would reduce critical-section
        // duration for write-heavy workloads at the cost of:
        //   (a) requiring a second lock acquisition on the async thread, and
        //   (b) potential ABA races if a newly-inserted entry reuses the same
        //       fingerprint before the deferred cleanup runs.
        // For the current query-cache workload (eviction rate ≪ insert rate),
        // the synchronous path is both safe and sufficiently fast.  If profiling
        // identifies this as a bottleneck, migrate to a concurrent lock-free
        // dependency-index structure (e.g. tbb::concurrent_unordered_map) before
        // adding async dispatch.
        //
        // Remove from dependency index (synchronous; see note above)
        removeFromDependencyIndex(fingerprint, it->second.entry.dependencies);
        
        // Update stats
        stats_.current_memory_bytes -= it->second.entry.result_size_bytes;
        stats_.current_entries--;
        stats_.evictions++;
        
        THEMIS_DEBUG("Evicted LRU entry: fingerprint={}", fingerprint.substr(0, 16));
        
        // Remove from LRU list using the stored iterator for O(1) removal
        // instead of searching for the element (which would be O(n))
        if (it->second.lru_it != lru_list_.end()) {
            lru_list_.erase(it->second.lru_it);
        } else {
            // Fallback: if iterator is invalid, remove by value (O(n) but safe)
            THEMIS_WARN("QueryCache: lru_it iterator invalid, falling back to list search for fingerprint={}", 
                       fingerprint.substr(0, 16));
            lru_list_.remove(fingerprint);
        }
        
        // Remove from cache
        cache_.erase(it);
    } else {
        // INVARIANT VIOLATION: fingerprint in lru_list_ but not in cache_
        // This indicates a data structure inconsistency. Log and repair.
        THEMIS_WARN("QueryCache::evictLRU: fingerprint {} found in lru_list but not in cache_ "
                   "(data structure inconsistency - removing from lru_list)", 
                   fingerprint.substr(0, 16));
        lru_list_.pop_back();
    }
}

void QueryCache::evictLFU() {
    if (cache_.empty()) {
        return;
    }
    
    // Find entry with lowest access count
    auto min_it = cache_.begin();
    size_t min_count = min_it->second.entry.access_count;
    
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.entry.access_count < min_count) {
            min_count = it->second.entry.access_count;
            min_it = it;
        }
    }
    
    std::string fingerprint = min_it->first;
    
    // Remove from LRU list
    lru_list_.erase(min_it->second.lru_it);
    
    // Remove from dependency index
    removeFromDependencyIndex(fingerprint, min_it->second.entry.dependencies);
    
    // Update stats
    stats_.current_memory_bytes -= min_it->second.entry.result_size_bytes;
    stats_.current_entries--;
    stats_.evictions++;
    
    THEMIS_DEBUG("Evicted LFU entry: fingerprint={}, access_count={}", 
                fingerprint.substr(0, 16), min_count);
    
    // Remove from cache
    cache_.erase(min_it);
}

void QueryCache::evictOne() {
    if (config_.eviction_policy == EvictionPolicy::LRU) {
        evictLRU();
    } else {
        evictLFU();
    }
}

void QueryCache::updateLRU(const std::string& fingerprint) {
    auto it = cache_.find(fingerprint);
    if (it == cache_.end()) {
        return;
    }
    
    // Remove from current position
    lru_list_.erase(it->second.lru_it);
    
    // Add to front (most recent)
    lru_list_.push_front(fingerprint);
    it->second.lru_it = lru_list_.begin();
}

bool QueryCache::shouldEvict() const {
    // Check if we're over entry limit
    if (stats_.current_entries >= config_.max_entries) {
        return true;
    }
    
    // Check memory pressure
    if (config_.enable_memory_pressure_eviction) {
        double utilization = stats_.memoryUtilization(config_.max_memory_bytes);
        if (utilization >= config_.memory_pressure_threshold) {
            return true;
        }
    }
    
    return false;
}

size_t QueryCache::estimateEntrySize(const CacheEntry& entry) const {
    // Estimate memory footprint
    size_t size = 0;
    
    // Fingerprint, query, params
    size += entry.query_fingerprint.size();
    size += entry.original_query.size();
    size += entry.query_params.dump().size();
    
    // Result (approximate)
    size += entry.result.dump().size();
    
    // Dependencies
    for (const auto& dep : entry.dependencies) {
        size += dep.size();
    }
    
    // Overhead for data structures
    size += sizeof(CacheEntry);
    
    return size;
}

void QueryCache::updateStats([[maybe_unused]] bool hit) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_requests++;
    if (hit) {
        stats_.hits++;
    } else {
        stats_.misses++;
    }
}

void QueryCache::addToDependencyIndex(
    const std::string& fingerprint,
    const std::vector<std::string>& dependencies
) {
    if (dependencies.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(dependency_mutex_);
    for (const auto& dep : dependencies) {
        dependency_index_[dep].push_back(fingerprint);
    }
}

void QueryCache::removeFromDependencyIndex(
    const std::string& fingerprint,
    const std::vector<std::string>& dependencies
) {
    if (dependencies.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(dependency_mutex_);
    for (const auto& dep : dependencies) {
        auto it = dependency_index_.find(dep);
        if (it != dependency_index_.end()) {
            auto& vec = it->second;
            vec.erase(std::remove(vec.begin(), vec.end(), fingerprint), vec.end());
            if (vec.empty()) {
                dependency_index_.erase(it);
            }
        }
    }
}

} // namespace query
} // namespace themis
