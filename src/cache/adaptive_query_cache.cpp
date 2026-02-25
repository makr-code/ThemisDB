/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_query_cache.cpp                           ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     1294                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03f3c2a45  2026-02-22  feat(cache): warmup from query log and export snapshot – ... ║
    • d8bc55d98  2026-02-22  Add Admin API for cache operations and monitoring ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "cache/adaptive_query_cache.h"
#include "cache/cache_replication.h"
#include "cache/eviction_policy.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/zstd_codec.h"
#include "utils/logger.h"
#include "observability/metrics_collector.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <openssl/sha.h>
#include <regex>

namespace themis {

// Constants
constexpr size_t QUERY_CACHE_PREFIX_LEN = 12;  // Length of "query_cache:"
constexpr const char* QUERY_CACHE_PREFIX = "query_cache:";
constexpr int RETRY_BACKOFF_MULTIPLIER = 2;    // Exponential backoff multiplier
constexpr int64_t ADAPTIVE_TTL_WINDOW_MS = 5 * 60 * 1000;  // 5-minute sliding window

AdaptiveQueryCache::AdaptiveQueryCache(const Config& config)
    : config_(config) {
    
    // Phase 2: Validate configuration on startup
    std::string validation_error;
    if (!config_.validate(&validation_error)) {
        throw std::invalid_argument("Invalid cache configuration: " + validation_error);
    }
    
    THEMIS_INFO("AdaptiveQueryCache initialized: L1={} entries, L2={} entries, L3=RocksDB",
                config_.l1_max_entries, config_.l2_max_entries);
    
    // Phase 2: Log configuration for observability
    if (config_.enable_rate_limiting) {
        THEMIS_INFO("Rate limiting enabled: {} requests/sec", config_.max_requests_per_second);
    }
    if (config_.enable_tenant_isolation) {
        THEMIS_INFO("Tenant isolation enabled: {} bytes per tenant", config_.per_tenant_max_bytes);
    }
    if (config_.enable_write_through) {
        THEMIS_INFO("Write-through cache mode enabled: L1/L2 entries will also be persisted to L3");
    }
    
    // Phase 2: Initialize rate limiter
    if (config_.enable_rate_limiting) {
        cache::RateLimiter::Config rl_config;
        rl_config.max_requests_per_second = config_.max_requests_per_second;
        rate_limiter_ = std::make_unique<cache::RateLimiter>(rl_config);
        THEMIS_INFO("Rate limiting enabled: {} requests/sec", config_.max_requests_per_second);
    }
    
    // Initialize circuit breaker for L3 (Phase 1: Fault Isolation)
    if (config_.enable_circuit_breaker) {
        cache::CircuitBreaker::Config cb_config;
        cb_config.failure_threshold = config_.cb_failure_threshold;
        cb_config.timeout_ms = config_.cb_timeout_ms;
        l3_circuit_breaker_ = std::make_unique<cache::CircuitBreaker>(cb_config);
        THEMIS_INFO("Circuit breaker enabled for L3 cache (threshold={}, timeout={}ms)",
                    cb_config.failure_threshold, cb_config.timeout_ms);
    }

    // Initialize configurable eviction strategies for L1 and L2
    l1_eviction_strategy_ = cache::makeEvictionStrategy(
        config_.l1_eviction_policy, config_.l1_max_entries);
    l2_eviction_strategy_ = cache::makeEvictionStrategy(
        config_.l2_eviction_policy, config_.l2_max_entries);

    // Phase 4: Initialize predictive pre-fetcher
    if (config_.enable_predictive_prefetch) {
        cache::PredictivePrefetcher::Config pf_config;
        pf_config.max_tracked_keys       = config_.prefetch_max_tracked_keys;
        pf_config.max_predictions        = config_.prefetch_max_predictions;
        pf_config.min_transition_count   = config_.prefetch_min_transition_count;
        pf_config.min_confidence         = config_.prefetch_min_confidence;
        prefetcher_ = std::make_unique<cache::PredictivePrefetcher>(pf_config);
        THEMIS_INFO("Predictive pre-fetcher enabled: max_keys={}, max_predictions={}",
                    pf_config.max_tracked_keys, pf_config.max_predictions);
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
                retry_delay_ms *= RETRY_BACKOFF_MULTIPLIER;  // Exponential backoff
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
    // Phase 4: Deregister coordinator callbacks before releasing memory.
    // Any coordinator that outlives this cache would otherwise hold a [this]
    // lambda pointing to freed memory, causing use-after-free on the next
    // publication.
    setCoordinator(nullptr);
    clear();
}

std::string AdaptiveQueryCache::generateFingerprint(
    const std::string& query,
    const nlohmann::json& params,
    const std::string& tenant_id
) const {
    // Concatenate query + params for hashing
    std::string input = query;
    if (!params.empty()) {
        input += "::" + params.dump();
    }
    
    // Phase 2: Include tenant_id in fingerprint if provided
    if (!tenant_id.empty()) {
        input += "::tenant:" + tenant_id;
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
    const std::string& fingerprint,
    const std::string& tenant_id
) {
    // Phase 2: Create tenant-scoped key if tenant isolation enabled
    std::string key = (config_.enable_tenant_isolation && !tenant_id.empty())
                      ? makeTenantKey(fingerprint, tenant_id)
                      : fingerprint;
    // Phase 2: Check rate limiter
    if (rate_limiter_ && !rate_limiter_->tryAcquire()) {
        enhanced_metrics_.rate_limited_requests++;
        THEMIS_DEBUG("Request rate limited for fingerprint: {}", fingerprint.substr(0, 16));
        return std::nullopt;
    }
    
    int64_t now_ms = getCurrentTimeMs();
    
    // Try L1 (HOT) - fastest path
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        auto it = l1_cache_.find(key);
        if (it != l1_cache_.end()) {
            auto& entry = it->second;
            
            // Check expiration
            if (isExpired(entry.created_at_ms, entry.ttl_seconds)) {
                l1_eviction_strategy_->onRemove(key);
                l1_cache_.erase(it);
                stats_.evictions++;
                enhanced_metrics_.evictions++;
            } else {
                // Cache hit!
                entry.last_accessed_ms = now_ms;
                entry.access_count++;
                l1_eviction_strategy_->onAccess(key);
                
                // Phase 3: Adaptive TTL tuning via sliding 5-minute window
                if (config_.enable_adaptive_ttl) {
                    if (now_ms - entry.window_start_ms >= ADAPTIVE_TTL_WINDOW_MS) {
                        // Window elapsed: apply cold-key policy on previous window count
                        if (entry.window_start_ms > 0 && entry.window_count <= 1) {
                            int old_ttl = entry.ttl_seconds;
                            entry.ttl_seconds = std::max(
                                static_cast<int>(entry.ttl_seconds * 0.5),
                                config_.adaptive_ttl_min_seconds);
                            if (entry.ttl_seconds < old_ttl) {
                                enhanced_metrics_.ttl_shortened_total++;
                            }
                        }
                        entry.window_start_ms = now_ms;
                        entry.window_count = 1;
                    } else {
                        entry.window_count++;
                        // Hot-key policy: extend TTL when heavily accessed in window
                        if (entry.window_count >= 10) {
                            int old_ttl = entry.ttl_seconds;
                            entry.ttl_seconds = std::min(
                                static_cast<int>(entry.ttl_seconds * 1.5),
                                config_.adaptive_ttl_max_seconds);
                            if (entry.ttl_seconds > old_ttl) {
                                enhanced_metrics_.ttl_extended_total++;
                            }
                        } else {
                            entry.ttl_seconds = calculateAdaptiveTTL(entry.access_count);
                        }
                    }
                    entry.created_at_ms = now_ms;  // Reset TTL window on each access
                }
                
                stats_.l1_hits++;
                enhanced_metrics_.l1_hits++;
                
                // Phase 3: Track per-tenant hit
                if (config_.enable_tenant_isolation && !tenant_id.empty()) {
                    std::lock_guard<std::mutex> tlock(tenant_mutex_);
                    tenant_metrics_[tenant_id].hits++;
                }

                // Phase 4: Record access for predictive pre-fetching
                if (prefetcher_) {
                    prefetcher_->recordQueryAccess(fingerprint, tenant_id);
                }
                
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
        auto it = l2_cache_.find(key);
        if (it != l2_cache_.end()) {
            auto& entry = it->second;
            
            // Check expiration
            if (isExpired(entry.created_at_ms, entry.ttl_seconds)) {
                l2_eviction_strategy_->onRemove(it->first);
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
                    l2_eviction_strategy_->onAccess(it->first);
                    
                    // Phase 3: Adaptive TTL tuning via sliding 5-minute window
                    if (config_.enable_adaptive_ttl) {
                        if (now_ms - entry.window_start_ms >= ADAPTIVE_TTL_WINDOW_MS) {
                            // Window elapsed: apply cold-key policy on previous window count
                            if (entry.window_start_ms > 0 && entry.window_count <= 1) {
                                int old_ttl = entry.ttl_seconds;
                                entry.ttl_seconds = std::max(
                                    static_cast<int>(entry.ttl_seconds * 0.5),
                                    config_.adaptive_ttl_min_seconds);
                                if (entry.ttl_seconds < old_ttl) {
                                    enhanced_metrics_.ttl_shortened_total++;
                                }
                            }
                            entry.window_start_ms = now_ms;
                            entry.window_count = 1;
                        } else {
                            entry.window_count++;
                            // Hot-key policy: extend TTL when heavily accessed in window
                            if (entry.window_count >= 10) {
                                int old_ttl = entry.ttl_seconds;
                                entry.ttl_seconds = std::min(
                                    static_cast<int>(entry.ttl_seconds * 1.5),
                                    config_.adaptive_ttl_max_seconds);
                                if (entry.ttl_seconds > old_ttl) {
                                    enhanced_metrics_.ttl_extended_total++;
                                }
                            } else {
                                entry.ttl_seconds = calculateAdaptiveTTL(entry.access_count);
                            }
                        }
                        entry.created_at_ms = now_ms;  // Reset TTL window on each access
                    }
                    
                    stats_.l2_hits++;
                    enhanced_metrics_.l2_hits++;

                    // Phase 3: Track per-tenant hit
                    if (config_.enable_tenant_isolation && !tenant_id.empty()) {
                        std::lock_guard<std::mutex> tlock(tenant_mutex_);
                        tenant_metrics_[tenant_id].hits++;
                    }

                    // Phase 4: Record access for predictive pre-fetching
                    if (prefetcher_) {
                        prefetcher_->recordQueryAccess(fingerprint, tenant_id);
                    }
                    
                    // Promote to L1 if accessed frequently
                    if (entry.access_count >= 3 && decompressed.size() < config_.l1_max_entry_size) {
                        L1Entry l1_entry;
                        l1_entry.result = result;
                        l1_entry.created_at_ms = entry.created_at_ms;
                        l1_entry.last_accessed_ms = now_ms;
                        l1_entry.access_count = entry.access_count;
                        l1_entry.ttl_seconds = entry.ttl_seconds;
                        l1_entry.window_start_ms = entry.window_start_ms;
                        l1_entry.window_count = entry.window_count;
                        
                        std::lock_guard<std::mutex> l1_lock(l1_mutex_);
                        if (l1_cache_.size() >= config_.l1_max_entries) {
                            evictLRU(CacheLevel::HOT);
                        }
                        l2_eviction_strategy_->onRemove(key);
                        l1_cache_[key] = std::move(l1_entry);
                        l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
                        l2_cache_.erase(it);
                        stats_.promotions++;
                        enhanced_metrics_.promotions++;
                        
                        THEMIS_DEBUG("Promoted L2->L1: key={}", key.substr(0, 16));
                    }
                    
                    // Return entry
                    CacheEntry cache_entry;
                    cache_entry.query_fingerprint = key;
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

                    // Phase 3: Adaptive TTL tuning via sliding 5-minute window
                    if (config_.enable_adaptive_ttl) {
                        int64_t window_start_ms = entry_json.value("window_start_ms", (int64_t)0);
                        uint32_t window_count = entry_json.value("window_count", (uint32_t)0);

                        if (now_ms - window_start_ms >= ADAPTIVE_TTL_WINDOW_MS) {
                            // Window elapsed: apply cold-key policy on previous window count
                            if (window_start_ms > 0 && window_count <= 1) {
                                int old_ttl = ttl_seconds;
                                ttl_seconds = std::max(
                                    static_cast<int>(ttl_seconds * 0.5),
                                    config_.adaptive_ttl_min_seconds);
                                if (ttl_seconds < old_ttl) {
                                    enhanced_metrics_.ttl_shortened_total++;
                                }
                                entry_json["ttl_seconds"] = ttl_seconds;
                            }
                            entry_json["window_start_ms"] = now_ms;
                            entry_json["window_count"] = 1;
                        } else {
                            window_count++;
                            entry_json["window_count"] = window_count;
                            // Hot-key policy: extend TTL when heavily accessed in window
                            if (window_count >= 10) {
                                int old_ttl = ttl_seconds;
                                ttl_seconds = std::min(
                                    static_cast<int>(ttl_seconds * 1.5),
                                    config_.adaptive_ttl_max_seconds);
                                if (ttl_seconds > old_ttl) {
                                    enhanced_metrics_.ttl_extended_total++;
                                }
                                entry_json["ttl_seconds"] = ttl_seconds;
                            } else {
                                ttl_seconds = calculateAdaptiveTTL(access_count);
                                entry_json["ttl_seconds"] = ttl_seconds;
                            }
                        }
                        entry_json["created_at_ms"] = now_ms;  // Reset TTL window on each access
                        created_at_ms = now_ms;
                    }

                    l3_db_->put(key, entry_json.dump());
                    
                    stats_.l3_hits++;
                    enhanced_metrics_.l3_hits++;

                    // Phase 3: Track per-tenant hit
                    if (config_.enable_tenant_isolation && !tenant_id.empty()) {
                        std::lock_guard<std::mutex> tlock(tenant_mutex_);
                        tenant_metrics_[tenant_id].hits++;
                    }
                    
                    // Phase 1: Record success for circuit breaker
                    if (l3_circuit_breaker_) {
                        l3_circuit_breaker_->recordSuccess();
                        enhanced_metrics_.l3_circuit_breaker_open = false;
                    }

                    // Phase 4: Record access for predictive pre-fetching
                    if (prefetcher_) {
                        prefetcher_->recordQueryAccess(fingerprint, tenant_id);
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
    // Phase 3: Track per-tenant miss
    if (config_.enable_tenant_isolation && !tenant_id.empty()) {
        std::lock_guard<std::mutex> tlock(tenant_mutex_);
        tenant_metrics_[tenant_id].misses++;
    }
    // Phase 4: Record access for predictive pre-fetching even on miss
    if (prefetcher_) {
        prefetcher_->recordQueryAccess(fingerprint, tenant_id);
    }
    THEMIS_DEBUG("Cache miss: fingerprint={}", fingerprint.substr(0, 16));
    return std::nullopt;
}

bool AdaptiveQueryCache::put(
    const std::string& fingerprint,
    const nlohmann::json& query_params,
    const nlohmann::json& result,
    const std::string& tenant_id,
    const std::vector<std::string>& pii_uuids
) {
    // Phase 2: Check rate limiter
    if (rate_limiter_ && !rate_limiter_->tryAcquire()) {
        enhanced_metrics_.rate_limited_requests++;
        THEMIS_DEBUG("Put request rate limited for fingerprint: {}", fingerprint.substr(0, 16));
        return false;
    }

    // Phase 4: Capture replication listener (avoid holding replication_mutex_ during write)
    std::shared_ptr<cache::ICacheReplicationListener> rep_listener;
    {
        std::lock_guard<std::mutex> rep_lock(replication_mutex_);
        rep_listener = replication_listener_;
    }
    
    int64_t now_ms = getCurrentTimeMs();
    std::string result_str = result.dump();
    size_t result_size = result_str.size();
    
    // Phase 2: Check tenant quota
    if (!checkTenantQuota(tenant_id, result_size)) {
        THEMIS_WARN("Tenant {} quota exceeded, rejecting entry", tenant_id);
        enhanced_metrics_.size_limit_rejections++;  // Track as size rejection
        return false;
    }
    
    // Phase 1: Validate entry size
    if (config_.enable_size_limits && !isWithinSizeLimit(result_size)) {
        THEMIS_WARN("Rejected cache entry due to size limit: size={}, max={}",
                   result_size, config_.max_total_entry_size);
        enhanced_metrics_.size_limit_rejections++;
        return false;
    }
    
    // Phase 2: Create tenant-scoped key if tenant isolation enabled
    std::string key = (config_.enable_tenant_isolation && !tenant_id.empty())
                      ? makeTenantKey(fingerprint, tenant_id)
                      : fingerprint;
    
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
    if (config_.enable_adaptive_ttl) {
        // Phase 3: Use adaptive TTL based on future access patterns
        // For new entries, start with minimum TTL (access_count = 0)
        ttl_seconds = calculateAdaptiveTTL(0);
    } else {
        // Use tier-specific TTL
        if (level == CacheLevel::HOT) {
            ttl_seconds = config_.l1_ttl_seconds;
        } else if (level == CacheLevel::WARM) {
            ttl_seconds = config_.l2_ttl_seconds;
        } else {
            ttl_seconds = config_.l3_ttl_seconds;
        }
    }

    // Phase 4: Capture coordinator pointer once before acquiring any cache mutex.
    // Graceful degradation: failures in the coordinator never block the local store.
    std::shared_ptr<cache::ICacheCoordinator> repl_coord;
    if (config_.enable_replication) {
        std::lock_guard<std::mutex> lk(coordinator_mutex_);
        repl_coord = coordinator_;
    }
    // Use the tenant-scoped key so peer caches with the same isolation config
    // can look the entry up via get(fingerprint, tenant_id) without ambiguity.
    auto notifyCoordinator = [&]() {
        if (!repl_coord) return;
        try {
            repl_coord->publishEntry(key, result, ttl_seconds, tenant_id);
        } catch (const std::exception& e) {
            THEMIS_WARN("Cache replication publish failed: {}", e.what());
        }
    };

    // Store in appropriate level
    if (level == CacheLevel::HOT && result_size < config_.l1_max_entry_size) {
        {
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
            entry.window_start_ms = now_ms;
            entry.window_count = 0;
            
            l1_cache_[key] = std::move(entry);
            l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
            enhanced_metrics_.total_bytes_cached += result_size;
            
            // Phase 2/3: Update tenant size tracking
            if (config_.enable_tenant_isolation && !tenant_id.empty()) {
                std::lock_guard<std::mutex> lock(tenant_mutex_);
                tenant_metrics_[tenant_id].bytes_used += result_size;
            }
            
            THEMIS_DEBUG("Stored in L1: key={}, size={}", key.substr(0, 16), result_size);
        }  // l1_mutex_ released before write-through to avoid blocking L1 reads during L3 I/O

        // Phase 4: Write-through mode - persist to L3 outside L1 lock
        if (config_.enable_write_through && l3_db_) {
            writeThroughToL3(fingerprint, query_params, result, now_ms, ttl_seconds);
        }

    // Phase 4: Write-through mode – write to all applicable tiers simultaneously.
    // This ensures every entry is available at the closest tier on the first read,
    // eliminating inter-tier promotion latency for read-heavy workloads.
    if (config_.enable_write_through) {
        bool any_written = false;

        // Write to L1 if the entry fits
        if (result_size < config_.l1_max_entry_size) {
            int l1_ttl = config_.enable_adaptive_ttl ? calculateAdaptiveTTL(0) : config_.l1_ttl_seconds;
            std::lock_guard<std::mutex> l1_lock(l1_mutex_);
            if (l1_cache_.size() >= config_.l1_max_entries) {
                evictLRU(CacheLevel::HOT);
            }
            L1Entry l1_entry;
            l1_entry.result = result;
            l1_entry.created_at_ms = now_ms;
            l1_entry.last_accessed_ms = now_ms;
            l1_entry.access_count = 1;
            l1_entry.ttl_seconds = l1_ttl;
            l1_entry.window_start_ms = now_ms;
            l1_entry.window_count = 0;
            l1_cache_[key] = std::move(l1_entry);
            l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
            any_written = true;
            THEMIS_DEBUG("Write-through: stored in L1: key={}, size={}", key.substr(0, 16), result_size);
        }

        // GDPR: Register PII tags in reverse index for L1 entry
        if (!pii_uuids.empty()) {
            std::lock_guard<std::mutex> plock(pii_index_mutex_);
            for (const auto& pii_uuid : pii_uuids) {
                pii_key_index_[pii_uuid].insert(key);
            }
        }
        
        // Write to L2 if the entry fits
        if (result_size < config_.l2_max_entry_size) {
            auto compressed = utils::zstd_compress(result_str, config_.l2_compression_level);
            if (!compressed.empty()) {
                int l2_ttl = config_.enable_adaptive_ttl ? calculateAdaptiveTTL(0) : config_.l2_ttl_seconds;
                std::lock_guard<std::mutex> l2_lock(l2_mutex_);
                if (l2_cache_.size() >= config_.l2_max_entries) {
                    evictLRU(CacheLevel::WARM);
                }
                L2Entry l2_entry;
                l2_entry.compressed_result = std::move(compressed);
                l2_entry.created_at_ms = now_ms;
                l2_entry.last_accessed_ms = now_ms;
                l2_entry.access_count = 1;
                l2_entry.ttl_seconds = l2_ttl;
                l2_entry.window_start_ms = now_ms;
                l2_entry.window_count = 0;
                size_t compressed_size = l2_entry.compressed_result.size();
                l2_cache_[fingerprint] = std::move(l2_entry);
                l2_eviction_strategy_->onInsert(fingerprint, static_cast<uint64_t>(now_ms));
                enhanced_metrics_.total_bytes_compressed += compressed_size;
                any_written = true;
                THEMIS_DEBUG("Write-through: stored in L2: key={}, size={}, compressed={}",
                             key.substr(0, 16), result_size, compressed_size);
            } else {
                THEMIS_WARN("Write-through: failed to compress result for L2 cache");
                enhanced_metrics_.compression_failures++;
            }
        }

        // Write to L3 if available and circuit breaker allows
        if (l3_db_) {
            bool l3_cb_ok = !l3_circuit_breaker_ || l3_circuit_breaker_->allowRequest();
            if (l3_cb_ok) {
                int l3_ttl = config_.enable_adaptive_ttl ? calculateAdaptiveTTL(0) : config_.l3_ttl_seconds;
                nlohmann::json l3_entry_json;
                l3_entry_json["result"] = result;
                l3_entry_json["query_params"] = query_params;
                l3_entry_json["created_at_ms"] = now_ms;
                l3_entry_json["last_accessed_ms"] = now_ms;
                l3_entry_json["access_count"] = 1;
                l3_entry_json["ttl_seconds"] = l3_ttl;
                l3_entry_json["window_start_ms"] = now_ms;
                l3_entry_json["window_count"] = 0;
                std::string l3_key = QUERY_CACHE_PREFIX + fingerprint;
                std::lock_guard<std::mutex> l3_lock(l3_mutex_);
                try {
                    bool ok = l3_db_->put(l3_key, l3_entry_json.dump());
                    if (ok) {
                        if (l3_circuit_breaker_) {
                            l3_circuit_breaker_->recordSuccess();
                            enhanced_metrics_.l3_circuit_breaker_open = false;
                        }
                        any_written = true;
                        THEMIS_DEBUG("Write-through: stored in L3: fingerprint={}, size={}",
                                     fingerprint.substr(0, 16), result_size);
                    } else {
                        enhanced_metrics_.l3_write_errors++;
                        if (l3_circuit_breaker_) {
                            l3_circuit_breaker_->recordFailure();
                            if (l3_circuit_breaker_->isOpen()) {
                                enhanced_metrics_.l3_circuit_breaker_trips++;
                                enhanced_metrics_.l3_circuit_breaker_open = true;
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    THEMIS_WARN("Write-through: L3 cache write exception: {}", e.what());
                    enhanced_metrics_.l3_write_errors++;
                    if (l3_circuit_breaker_) {
                        l3_circuit_breaker_->recordFailure();
                        if (l3_circuit_breaker_->isOpen()) {
                            enhanced_metrics_.l3_circuit_breaker_trips++;
                            enhanced_metrics_.l3_circuit_breaker_open = true;
                        }
                    }
                }
            } else {
                THEMIS_DEBUG("Write-through: L3 circuit breaker open, skipping L3 write");
                enhanced_metrics_.l3_circuit_breaker_open = true;
            }
        }

        if (any_written) {
            enhanced_metrics_.total_bytes_cached += result_size;
            enhanced_metrics_.write_through_writes++;
            // Phase 2/3: Update tenant size tracking
            if (config_.enable_tenant_isolation && !tenant_id.empty()) {
                std::lock_guard<std::mutex> tlock(tenant_mutex_);
                tenant_metrics_[tenant_id].bytes_used += result_size;
            }
            THEMIS_DEBUG("Write-through put complete: key={}, tiers written", key.substr(0, 16));
        }
        return any_written;
    }

    // Store in appropriate level
    if (level == CacheLevel::HOT && result_size < config_.l1_max_entry_size) {
        {
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
            entry.window_start_ms = now_ms;
            entry.window_count = 0;
            
            l1_cache_[key] = std::move(entry);
            l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
            enhanced_metrics_.total_bytes_cached += result_size;
            
            // Phase 2/3: Update tenant size tracking
            if (config_.enable_tenant_isolation && !tenant_id.empty()) {
                std::lock_guard<std::mutex> tenant_lock(tenant_mutex_);
                tenant_metrics_[tenant_id].bytes_used += result_size;
            }
        }  // l1_mutex_ released before notification

        THEMIS_DEBUG("Stored in L1: key={}, size={}", key.substr(0, 16), result_size);
        notifyCoordinator();

        // Phase 4: Notify replication listener (outside tier lock)
        if (rep_listener) {
            cache::CacheReplicationEvent ev;
            ev.type = cache::CacheReplicationEventType::WRITE;
            ev.key = key;
            ev.payload = result_str;
            ev.tenant_id = tenant_id;
            ev.ttl_seconds = ttl_seconds;
            rep_listener->onReplicationEvent(ev);
        }

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
        entry.window_start_ms = now_ms;
        entry.window_count = 0;
        
        size_t compressed_size = entry.compressed_result.size();
        l2_cache_[fingerprint] = std::move(entry);
        l2_eviction_strategy_->onInsert(fingerprint, static_cast<uint64_t>(now_ms));
        enhanced_metrics_.total_bytes_cached += result_size;
        enhanced_metrics_.total_bytes_compressed += compressed_size;

        // GDPR: Register PII tags in reverse index for L2 entry
        if (!pii_uuids.empty()) {
            std::lock_guard<std::mutex> plock(pii_index_mutex_);
            for (const auto& pii_uuid : pii_uuids) {
                pii_key_index_[pii_uuid].insert(fingerprint);
            }
        }

        size_t compressed_size;
        {
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
            entry.window_start_ms = now_ms;
            entry.window_count = 0;
            
            size_t compressed_size = entry.compressed_result.size();
            compressed_size = entry.compressed_result.size();
            l2_cache_[fingerprint] = std::move(entry);
            l2_eviction_strategy_->onInsert(fingerprint, static_cast<uint64_t>(now_ms));
            enhanced_metrics_.total_bytes_cached += result_size;
            enhanced_metrics_.total_bytes_compressed += compressed_size;
            THEMIS_DEBUG("Stored in L2: fingerprint={}, size={}, compressed={}",
                        fingerprint.substr(0, 16), result_size, compressed_size);
        }  // l2_mutex_ released before write-through to avoid blocking L2 reads during L3 I/O

        // Phase 4: Write-through mode - persist to L3 outside L2 lock
        if (config_.enable_write_through && l3_db_) {
            writeThroughToL3(fingerprint, query_params, result, now_ms, ttl_seconds);
        }  // l2_mutex_ released before notification

        THEMIS_DEBUG("Stored in L2: fingerprint={}, size={}, compressed={}",
                    fingerprint.substr(0, 16), result_size, compressed_size);
        notifyCoordinator();

        // Phase 4: Notify replication listener (outside tier lock)
        if (rep_listener) {
            cache::CacheReplicationEvent ev;
            ev.type = cache::CacheReplicationEventType::WRITE;
            ev.key = fingerprint;
            ev.payload = result_str;
            ev.tenant_id = tenant_id;
            ev.ttl_seconds = ttl_seconds;
            rep_listener->onReplicationEvent(ev);
        }

        return true;
        
    } else if (l3_db_) {
        // Phase 1: Check circuit breaker before L3 operation
        if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
            THEMIS_WARN("L3 cache circuit breaker is open, rejecting write");
            enhanced_metrics_.l3_circuit_breaker_open = true;
            return false;
        }

        nlohmann::json entry_json;
        entry_json["result"] = result;
        entry_json["query_params"] = query_params;
        entry_json["created_at_ms"] = now_ms;
        entry_json["last_accessed_ms"] = now_ms;
        entry_json["access_count"] = 1;
        entry_json["ttl_seconds"] = ttl_seconds;
        entry_json["window_start_ms"] = now_ms;
        entry_json["window_count"] = 0;

        std::string key = QUERY_CACHE_PREFIX + fingerprint;
        std::string entry_payload;
        bool ok = false;

        {
            // Store in L3 (RocksDB)
            std::lock_guard<std::mutex> lock(l3_mutex_);

            try {
                entry_payload = entry_json.dump();
                ok = l3_db_->put(key, entry_payload);

                if (ok) {
                    // Phase 1: Record success for circuit breaker
                    if (l3_circuit_breaker_) {
                        l3_circuit_breaker_->recordSuccess();
                        enhanced_metrics_.l3_circuit_breaker_open = false;
                    }
                    enhanced_metrics_.total_bytes_cached += result_size;
                    THEMIS_DEBUG("Stored in L3: fingerprint={}, size={}", fingerprint.substr(0, 16), result_size);
                } else {
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
                }
                enhanced_metrics_.total_bytes_cached += result_size;
                // GDPR: Write PII reference index entries for L3
                for (const auto& pii_uuid : pii_uuids) {
                    l3_db_->put("pii_ref:" + pii_uuid + ":" + fingerprint, "");
                }
                THEMIS_DEBUG("Stored in L3: fingerprint={}, size={}", fingerprint.substr(0, 16), result_size);
                notifyCoordinator();
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
        }  // l3_mutex_ released before notification

        if (ok) {
            // Phase 4: Notify replication listener (outside tier lock)
            if (rep_listener) {
                cache::CacheReplicationEvent ev;
                ev.type = cache::CacheReplicationEventType::WRITE;
                ev.key = key;
                ev.payload = entry_payload;
                ev.tenant_id = tenant_id;
                ev.ttl_seconds = ttl_seconds;
                rep_listener->onReplicationEvent(ev);
            }
            return true;
        }
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
                l1_eviction_strategy_->onRemove(it->first);
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
                l2_eviction_strategy_->onRemove(it->first);
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

    // Phase 4: Propagate invalidation to peer nodes via replication coordinator
    if (config_.enable_replication) {
        std::shared_ptr<cache::ICacheCoordinator> coord;
        { std::lock_guard<std::mutex> lk(coordinator_mutex_); coord = coordinator_; }
        if (coord) {
            try { coord->publishInvalidation(pattern); }
            catch (const std::exception& e) {
                THEMIS_WARN("Cache replication invalidation publish failed: {}", e.what());
            }
    // Phase 4: Notify replication listener
    if (count > 0) {
        std::shared_ptr<cache::ICacheReplicationListener> rep_listener;
        {
            std::lock_guard<std::mutex> rep_lock(replication_mutex_);
            rep_listener = replication_listener_;
        }
        if (rep_listener) {
            cache::CacheReplicationEvent ev;
            ev.type = cache::CacheReplicationEventType::INVALIDATE;
            ev.pattern = pattern;
            rep_listener->onReplicationEvent(ev);
        }
    }

    return count;
}

void AdaptiveQueryCache::clear() {
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        l1_cache_.clear();
        l1_eviction_strategy_->clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        l2_cache_.clear();
        l2_eviction_strategy_->clear();
    }
    
    // GDPR: Clear in-memory PII reverse index
    {
        std::lock_guard<std::mutex> plock(pii_index_mutex_);
        pii_key_index_.clear();
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
            // GDPR: Also clear L3 PII reference index entries
            std::vector<std::string> pii_ref_keys;
            l3_db_->scanPrefix("pii_ref:", [&pii_ref_keys](std::string_view key, std::string_view) {
                pii_ref_keys.emplace_back(key);
                return true;
            });
            for (const auto& key : pii_ref_keys) {
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
                l1_eviction_strategy_->onRemove(it->first);
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
                l2_eviction_strategy_->onRemove(it->first);
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
            {"utilization", static_cast<double>(l1_cache_.size()) / config_.l1_max_entries},
            {"eviction_policy", std::string(l1_eviction_strategy_->getName())}
        };
    }
    
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        info["l2"] = {
            {"entries", l2_cache_.size()},
            {"max_entries", config_.l2_max_entries},
            {"utilization", static_cast<double>(l2_cache_.size()) / config_.l2_max_entries},
            {"eviction_policy", std::string(l2_eviction_strategy_->getName())}
        };
    }
    
    info["l3"] = {
        {"enabled", l3_db_ != nullptr},
        {"path", config_.l3_db_path}
    };
    
    // Phase 3: Adaptive TTL tuning metrics
    if (config_.enable_adaptive_ttl) {
        info["adaptive_ttl"] = {
            {"enabled", true},
            {"min_seconds", config_.adaptive_ttl_min_seconds},
            {"max_seconds", config_.adaptive_ttl_max_seconds},
            {"scaling_factor", config_.adaptive_ttl_scaling_factor},
            {"ttl_extended_total", enhanced_metrics_.ttl_extended_total.load()},
            {"ttl_shortened_total", enhanced_metrics_.ttl_shortened_total.load()}
        };
    } else {
        info["adaptive_ttl"] = {{"enabled", false}};
    }

    // Phase 4: Write-through mode info
    info["write_through"] = {
        {"enabled", config_.enable_write_through},
        {"total", enhanced_metrics_.write_through_total.load()},
        {"errors", enhanced_metrics_.write_through_errors.load()},
        {"writes", enhanced_metrics_.write_through_writes.load()}
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
    
    // Phase 3: Adaptive TTL with logarithmic scaling
    // More frequently accessed entries get longer TTL
    // Formula: TTL = base_ttl * (1 + log(access_count + 1) / scaling_factor)
    // This provides diminishing returns for very high access counts
    
    int base_ttl = config_.adaptive_ttl_min_seconds;
    double log_factor = std::log(static_cast<double>(access_count + 1)) / config_.adaptive_ttl_scaling_factor;
    int adaptive_ttl = static_cast<int>(base_ttl * (1.0 + log_factor));
    
    // Clamp to configured bounds
    adaptive_ttl = std::max(adaptive_ttl, config_.adaptive_ttl_min_seconds);
    adaptive_ttl = std::min(adaptive_ttl, config_.adaptive_ttl_max_seconds);
    
    return adaptive_ttl;
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
        if (l1_cache_.empty()) return;

        // Use the configured eviction strategy to select the victim key
        auto victim = l1_eviction_strategy_->selectVictim();
        if (victim && l1_cache_.count(*victim)) {
            l1_eviction_strategy_->onRemove(*victim);
            l1_cache_.erase(*victim);
        } else {
            // Fallback: score-based scan (strategy out of sync or returned nothing)
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
            l1_eviction_strategy_->onRemove(lru_it->first);
            l1_cache_.erase(lru_it);
        }
        stats_.evictions++;
        enhanced_metrics_.evictions++;

    } else if (level == CacheLevel::WARM) {
        if (l2_cache_.empty()) return;

        // Use the configured eviction strategy to select the victim key
        auto victim = l2_eviction_strategy_->selectVictim();
        if (victim && l2_cache_.count(*victim)) {
            l2_eviction_strategy_->onRemove(*victim);
            l2_cache_.erase(*victim);
        } else {
            // Fallback: score-based scan
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
            l2_eviction_strategy_->onRemove(lru_it->first);
            l2_cache_.erase(lru_it);
        }
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

// Phase 2: Config validation
bool AdaptiveQueryCache::Config::validate(std::string* error_msg) const {
    auto set_error = [error_msg](const std::string& msg) {
        if (error_msg) *error_msg = msg;
        return false;
    };
    
    // Validate L1 configuration
    if (l1_max_entries == 0) {
        return set_error("l1_max_entries must be greater than 0");
    }
    if (l1_max_entry_size == 0) {
        return set_error("l1_max_entry_size must be greater than 0");
    }
    if (l1_ttl_seconds < 0) {
        return set_error("l1_ttl_seconds must be non-negative");
    }
    
    // Validate L2 configuration
    if (l2_max_entries == 0) {
        return set_error("l2_max_entries must be greater than 0");
    }
    if (l2_max_entry_size == 0) {
        return set_error("l2_max_entry_size must be greater than 0");
    }
    if (l2_ttl_seconds < 0) {
        return set_error("l2_ttl_seconds must be non-negative");
    }
    if (l2_compression_level < 1 || l2_compression_level > 22) {
        return set_error("l2_compression_level must be between 1 and 22 (Zstd valid range)");
    }
    
    // Validate L3 configuration
    if (l3_ttl_seconds < 0) {
        return set_error("l3_ttl_seconds must be non-negative");
    }
    if (l3_db_path.empty()) {
        return set_error("l3_db_path must not be empty");
    }
    
    // Validate adaptive TTL (legacy + current fields)
    const int effective_min_ttl = (min_ttl_seconds != 60) ? min_ttl_seconds : adaptive_ttl_min_seconds;
    const int effective_max_ttl = (max_ttl_seconds != 86400) ? max_ttl_seconds : adaptive_ttl_max_seconds;
    if (effective_min_ttl <= 0) {
        return set_error("min_ttl_seconds must be greater than 0");
    }
    if (effective_max_ttl <= 0) {
        return set_error("max_ttl_seconds must be greater than 0");
    }
    if (effective_max_ttl < effective_min_ttl) {
        return set_error("max_ttl_seconds must be >= min_ttl_seconds");
    }
    
    // Validate eviction policy
    if (frequency_weight < 0.0f || frequency_weight > 1.0f) {
        return set_error("frequency_weight must be between 0.0 and 1.0");
    }
    
    // Validate size limits
    if (enable_size_limits) {
        if (max_total_entry_size == 0) {
            return set_error("max_total_entry_size must be greater than 0");
        }
        if (l1_max_entry_size > max_total_entry_size) {
            return set_error("l1_max_entry_size must be <= max_total_entry_size");
        }
        if (l2_max_entry_size > max_total_entry_size) {
            return set_error("l2_max_entry_size must be <= max_total_entry_size");
        }
    }
    
    // Validate circuit breaker
    if (enable_circuit_breaker) {
        if (cb_failure_threshold == 0) {
            return set_error("cb_failure_threshold must be greater than 0");
        }
        if (cb_timeout_ms == 0) {
            return set_error("cb_timeout_ms must be greater than 0");
        }
    }
    
    // Phase 2: Validate rate limiting
    if (enable_rate_limiting) {
        if (max_requests_per_second == 0) {
            return set_error("max_requests_per_second must be greater than 0");
        }
    }
    
    // Phase 2: Validate backpressure
    if (enable_backpressure) {
        if (l3_write_queue_size == 0) {
            return set_error("l3_write_queue_size must be greater than 0");
        }
    }
    
    // Phase 2: Validate tenant isolation
    if (enable_tenant_isolation) {
        if (per_tenant_max_bytes == 0) {
            return set_error("per_tenant_max_bytes must be greater than 0");
        }
    }
    
    // Phase 3: Validate adaptive TTL
    if (enable_adaptive_ttl) {
        if (effective_min_ttl <= 0) {
            return set_error("adaptive_ttl_min_seconds must be greater than 0");
        }
        if (effective_max_ttl <= 0) {
            return set_error("adaptive_ttl_max_seconds must be greater than 0");
        }
        if (effective_min_ttl >= effective_max_ttl) {
            return set_error("adaptive_ttl_min_seconds must be less than adaptive_ttl_max_seconds");
        }
        if (adaptive_ttl_scaling_factor <= 0.0) {
            return set_error("adaptive_ttl_scaling_factor must be greater than 0");
        }
    }
    
    return true;
}

// Phase 2: Tenant isolation helper methods
std::string AdaptiveQueryCache::makeTenantKey(
    const std::string& fingerprint,
    const std::string& tenant_id
) const {
    if (tenant_id.empty()) {
        return fingerprint;
    }
    return "tenant:" + tenant_id + ":" + fingerprint;
}

bool AdaptiveQueryCache::checkTenantQuota(
    const std::string& tenant_id,
    size_t additional_bytes
) {
    if (!config_.enable_tenant_isolation || tenant_id.empty()) {
        return true;  // No quotas if isolation disabled
    }
    
    std::lock_guard<std::mutex> lock(tenant_mutex_);
    size_t current_size = tenant_metrics_[tenant_id].bytes_used;
    size_t effective_quota = getEffectiveTenantQuota(tenant_id);
    
    if (effective_quota == 0) {
        return true;  // 0 means unlimited
    }
    
    if (current_size + additional_bytes > effective_quota) {
        THEMIS_WARN("Tenant {} quota exceeded: current={}, additional={}, limit={}",
                   tenant_id, current_size, additional_bytes, effective_quota);
        return false;
    }
    
    return true;
}

// ============================================================================
// Phase 4: Write-Through Cache Mode
// ============================================================================

bool AdaptiveQueryCache::writeThroughToL3(
    const std::string& fingerprint,
    const nlohmann::json& query_params,
    const nlohmann::json& result,
    int64_t now_ms,
    int ttl_seconds
) {
    if (!l3_db_) {
        return false;
    }

    // Check circuit breaker before L3 operation
    if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
        THEMIS_WARN("L3 circuit breaker is open, skipping write-through for fingerprint={}",
                    fingerprint.substr(0, 16));
        enhanced_metrics_.write_through_errors++;
        return false;
    }

    nlohmann::json entry_json;
    entry_json["result"] = result;
    entry_json["query_params"] = query_params;
    entry_json["created_at_ms"] = now_ms;
    entry_json["last_accessed_ms"] = now_ms;
    entry_json["access_count"] = 1;
    entry_json["ttl_seconds"] = ttl_seconds;
    entry_json["window_start_ms"] = now_ms;
    entry_json["window_count"] = 0;

    std::string l3_key = QUERY_CACHE_PREFIX + fingerprint;

    try {
        std::lock_guard<std::mutex> lock(l3_mutex_);
        bool ok = l3_db_->put(l3_key, entry_json.dump());
        if (ok) {
            if (l3_circuit_breaker_) {
                l3_circuit_breaker_->recordSuccess();
                enhanced_metrics_.l3_circuit_breaker_open = false;
            }
            enhanced_metrics_.write_through_total++;
            THEMIS_DEBUG("Write-through: persisted fingerprint={} to L3", fingerprint.substr(0, 16));
            return true;
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Write-through L3 write exception: {}", e.what());
        if (l3_circuit_breaker_) {
            l3_circuit_breaker_->recordFailure();
            if (l3_circuit_breaker_->isOpen()) {
                enhanced_metrics_.l3_circuit_breaker_trips++;
                enhanced_metrics_.l3_circuit_breaker_open = true;
            }
        }
    }

    enhanced_metrics_.write_through_errors++;
    return false;
}

// ============================================================================
// Phase 3: Admin API & Operational Tooling
// ============================================================================

nlohmann::json AdaptiveQueryCache::getStatsByTier() const {
    nlohmann::json stats;
    
    // L1 statistics
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        stats["l1"]["entries"] = l1_cache_.size();
        stats["l1"]["max_entries"] = config_.l1_max_entries;
        stats["l1"]["utilization"] = static_cast<double>(l1_cache_.size()) / config_.l1_max_entries;
        stats["l1"]["hits"] = enhanced_metrics_.l1_hits.load();
        stats["l1"]["eviction_policy"] = std::string(l1_eviction_strategy_->getName());
    }
    
    // L2 statistics
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        stats["l2"]["entries"] = l2_cache_.size();
        stats["l2"]["max_entries"] = config_.l2_max_entries;
        stats["l2"]["utilization"] = static_cast<double>(l2_cache_.size()) / config_.l2_max_entries;
        stats["l2"]["hits"] = enhanced_metrics_.l2_hits.load();
        stats["l2"]["eviction_policy"] = std::string(l2_eviction_strategy_->getName());
    }
    
    // L3 statistics
    stats["l3"]["enabled"] = (l3_db_ != nullptr);
    stats["l3"]["hits"] = enhanced_metrics_.l3_hits.load();
    if (l3_circuit_breaker_) {
        stats["l3"]["circuit_breaker_open"] = enhanced_metrics_.l3_circuit_breaker_open.load();
    }
    
    // Overall
    stats["overall"]["misses"] = enhanced_metrics_.misses.load();
    stats["overall"]["hit_rate"] = enhanced_metrics_.getHitRate();
    stats["overall"]["evictions"] = enhanced_metrics_.evictions.load();

    // Phase 4: Write-through mode status
    stats["write_through"]["enabled"] = config_.enable_write_through;
    stats["write_through"]["writes"] = enhanced_metrics_.write_through_writes.load();
    
    return stats;
}

nlohmann::json AdaptiveQueryCache::getHealthStatus() const {
    nlohmann::json health;
    health["healthy"] = true;
    health["warnings"] = nlohmann::json::array();

    // Per-tier status
    nlohmann::json tiers;

    // L1 tier
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        size_t entries = l1_cache_.size();
        double util = static_cast<double>(entries) / config_.l1_max_entries;
        std::string tier_status = (util > 0.9) ? "DEGRADED" : "OK";
        tiers["l1"] = {
            {"status",      tier_status},
            {"entries",     entries},
            {"max_entries", config_.l1_max_entries},
            {"utilization", util},
            {"ttl_seconds", config_.l1_ttl_seconds}
        };
        if (util > 0.9) {
            health["warnings"].push_back("L1 cache utilization high: " + std::to_string(util * 100) + "%");
        }
    }

    // L2 tier
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        size_t entries = l2_cache_.size();
        double util = static_cast<double>(entries) / config_.l2_max_entries;
        std::string tier_status = (util > 0.9) ? "DEGRADED" : "OK";
        tiers["l2"] = {
            {"status",      tier_status},
            {"entries",     entries},
            {"max_entries", config_.l2_max_entries},
            {"utilization", util},
            {"ttl_seconds", config_.l2_ttl_seconds}
        };
        if (util > 0.9) {
            health["warnings"].push_back("L2 cache utilization high: " + std::to_string(util * 100) + "%");
        }
    }

    // L3 tier
    {
        bool l3_open = enhanced_metrics_.l3_circuit_breaker_open.load();
        bool l3_enabled = (l3_db_ != nullptr);
        std::string tier_status = l3_open ? "UNAVAILABLE" : (l3_enabled ? "OK" : "DISABLED");
        tiers["l3"] = {
            {"status",      tier_status},
            {"enabled",     l3_enabled},
            {"path",        config_.l3_db_path},
            {"ttl_seconds", config_.l3_ttl_seconds}
        };
        if (l3_open) {
            health["healthy"] = false;
            health["warnings"].push_back("L3 circuit breaker is OPEN - RocksDB unavailable");
        }
    }

    health["tiers"] = tiers;

    // Embed circuit breaker details
    health["circuit_breaker"] = getCircuitBreakerStatus();

    // Phase 4: Write-through mode status
    health["write_through"] = {
        {"enabled", config_.enable_write_through},
        {"writes",  enhanced_metrics_.write_through_writes.load()}
    };

    // Check hit rate
    double hit_rate = enhanced_metrics_.getHitRate();
    if (hit_rate < 0.5) {
        health["warnings"].push_back("Low cache hit rate: " + std::to_string(hit_rate * 100) + "%");
    }

    // Check rate limiting
    uint64_t rate_limited = enhanced_metrics_.rate_limited_requests.load();
    if (rate_limited > 1000) {
        health["warnings"].push_back("High rate limiting: " + std::to_string(rate_limited) + " requests rejected");
    }

    return health;
}

std::vector<std::string> AdaptiveQueryCache::exportKeys(size_t max_keys) const {
    std::vector<std::string> keys;
    keys.reserve(max_keys);
    
    // Export L1 keys
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        for (const auto& [key, entry] : l1_cache_) {
            if (keys.size() >= max_keys) break;
            keys.push_back("L1:" + key.substr(0, 16) + "...");
        }
    }
    
    // Export L2 keys
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        for (const auto& [key, entry] : l2_cache_) {
            if (keys.size() >= max_keys) break;
            keys.push_back("L2:" + key.substr(0, 16) + "...");
        }
    }
    
    return keys;
}

nlohmann::json AdaptiveQueryCache::getTenantStats() const {
    nlohmann::json tenant_stats;
    
    if (!config_.enable_tenant_isolation) {
        tenant_stats["enabled"] = false;
        return tenant_stats;
    }
    
    tenant_stats["enabled"] = true;
    tenant_stats["quota_per_tenant"] = config_.per_tenant_max_bytes;
    
    std::lock_guard<std::mutex> lock(tenant_mutex_);
    for (const auto& [tenant_id, metrics] : tenant_metrics_) {
        nlohmann::json tenant_info;
        uint64_t total = metrics.hits + metrics.misses;
        size_t effective_quota = getEffectiveTenantQuota(tenant_id);
        tenant_info["bytes_used"]   = metrics.bytes_used;
        tenant_info["quota"]        = effective_quota;
        tenant_info["utilization"]  = effective_quota > 0 ? static_cast<double>(metrics.bytes_used) / effective_quota : 0.0;
        tenant_info["hits"]         = metrics.hits;
        tenant_info["misses"]       = metrics.misses;
        tenant_info["evictions"]    = metrics.evictions;
        tenant_info["hit_rate"]     = total > 0 ? static_cast<double>(metrics.hits) / total : 0.0;
        tenant_stats["tenants"][tenant_id] = tenant_info;
    }
    
    return tenant_stats;
}

nlohmann::json AdaptiveQueryCache::getTenantStatsForTenant(const std::string& tenant_id) const {
    if (!config_.enable_tenant_isolation || tenant_id.empty()) {
        nlohmann::json result;
        result["found"] = false;
        result["reason"] = "tenant isolation is disabled";
        return result;
    }

    std::lock_guard<std::mutex> lock(tenant_mutex_);
    auto it = tenant_metrics_.find(tenant_id);
    if (it == tenant_metrics_.end()) {
        nlohmann::json result;
        result["found"] = false;
        return result;
    }

    const auto& m = it->second;
    uint64_t total = m.hits + m.misses;
    size_t effective_quota = getEffectiveTenantQuota(tenant_id);
    nlohmann::json result;
    result["found"]       = true;
    result["tenant_id"]   = tenant_id;
    result["bytes_used"]  = m.bytes_used;
    result["quota"]       = effective_quota;
    result["utilization"] = effective_quota > 0 ? static_cast<double>(m.bytes_used) / effective_quota : 0.0;
    result["hits"]        = m.hits;
    result["misses"]      = m.misses;
    result["evictions"]   = m.evictions;
    result["hit_rate"]    = total > 0 ? static_cast<double>(m.hits) / total : 0.0;
    return result;
}

size_t AdaptiveQueryCache::bulkPut(
    const std::vector<std::tuple<std::string, nlohmann::json, nlohmann::json, std::string>>& entries
) {
    size_t successful = 0;
    
    for (const auto& [fingerprint, params, result, tenant_id] : entries) {
        if (put(fingerprint, params, result, tenant_id)) {
            successful++;
        }
    }
    
    THEMIS_INFO("Bulk put completed: {}/{} entries cached", successful, entries.size());
    return successful;
}

size_t AdaptiveQueryCache::invalidateTenant(const std::string& tenant_id) {
    if (tenant_id.empty()) {
        THEMIS_WARN("Invalid tenant_id for invalidation");
        return 0;
    }
    
    size_t count = 0;
    std::string tenant_prefix = "tenant:" + tenant_id + ":";
    
    // Invalidate L1
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
            if (it->first.find(tenant_prefix) == 0) {
                l1_eviction_strategy_->onRemove(it->first);
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
            if (it->first.find(tenant_prefix) == 0) {
                l2_eviction_strategy_->onRemove(it->first);
                it = l2_cache_.erase(it);
                count++;
            } else {
                ++it;
            }
        }
    }
    
    // Invalidate L3
    if (l3_db_ && config_.enable_tenant_isolation) {
        std::lock_guard<std::mutex> lock(l3_mutex_);
        
        if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
            THEMIS_WARN("L3 circuit breaker open, skipping L3 tenant invalidation");
        } else {
            try {
                std::vector<std::string> keys_to_delete;
                l3_db_->scanPrefix(QUERY_CACHE_PREFIX, [&](std::string_view key, std::string_view) {
                    std::string key_str(key);
                    if (key_str.find(tenant_prefix) != std::string::npos) {
                        keys_to_delete.emplace_back(key_str);
                    }
                    return true;
                });
                
                for (const auto& key : keys_to_delete) {
                    l3_db_->del(key);
                    count++;
                }
                
                if (l3_circuit_breaker_) {
                    l3_circuit_breaker_->recordSuccess();
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed to invalidate tenant in L3: {}", e.what());
                if (l3_circuit_breaker_) {
                    l3_circuit_breaker_->recordFailure();
                }
            }
        }
    }
    
    // Update tenant metrics: record evictions and reset byte count
    if (config_.enable_tenant_isolation) {
        std::lock_guard<std::mutex> lock(tenant_mutex_);
        tenant_metrics_[tenant_id].evictions += count;
        tenant_metrics_[tenant_id].bytes_used = 0;
    }
    
    THEMIS_INFO("Invalidated {} entries for tenant: {}", count, tenant_id);

    // Phase 4: Propagate tenant invalidation to peer nodes.
    // Use the same tenant-prefix pattern that the local invalidation uses
    // ("tenant:<id>:") so peer nodes performing regex matching evict exactly
    // the same set of L1/L2 keys.
    if (config_.enable_replication) {
        std::shared_ptr<cache::ICacheCoordinator> coord;
        { std::lock_guard<std::mutex> lk(coordinator_mutex_); coord = coordinator_; }
        if (coord) {
            try {
                std::string tenant_pattern = "^tenant:" + tenant_id + ":";
                coord->publishInvalidation(tenant_pattern, tenant_id);
            }
            catch (const std::exception& e) {
                THEMIS_WARN("Cache replication tenant invalidation publish failed: {}", e.what());
            }
        }
    }

    // Phase 4: Notify replication listener
    {
        std::shared_ptr<cache::ICacheReplicationListener> rep_listener;
        {
            std::lock_guard<std::mutex> rep_lock(replication_mutex_);
            rep_listener = replication_listener_;
        }
        if (rep_listener) {
            cache::CacheReplicationEvent ev;
            ev.type = cache::CacheReplicationEventType::INVALIDATE_TENANT;
            ev.tenant_id = tenant_id;
            rep_listener->onReplicationEvent(ev);
        }
    }

    return count;
}

size_t AdaptiveQueryCache::invalidatePII(const std::string& pii_uuid) {
    if (pii_uuid.empty()) {
        THEMIS_WARN("invalidatePII called with empty pii_uuid");
        return 0;
    }

    size_t count = 0;

    // --- L1 / L2: use in-memory PII reverse index ---
    std::unordered_set<std::string> keys_to_purge;
    {
        std::lock_guard<std::mutex> plock(pii_index_mutex_);
        auto it = pii_key_index_.find(pii_uuid);
        if (it != pii_key_index_.end()) {
            keys_to_purge = std::move(it->second);
            pii_key_index_.erase(it);
        }
    }

    if (!keys_to_purge.empty()) {
        {
            std::lock_guard<std::mutex> lock(l1_mutex_);
            for (const auto& k : keys_to_purge) {
                auto it = l1_cache_.find(k);
                if (it != l1_cache_.end()) {
                    l1_eviction_strategy_->onRemove(it->first);
                    l1_cache_.erase(it);
                    count++;
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(l2_mutex_);
            for (const auto& k : keys_to_purge) {
                auto it = l2_cache_.find(k);
                if (it != l2_cache_.end()) {
                    l2_eviction_strategy_->onRemove(it->first);
                    l2_cache_.erase(it);
                    count++;
                }
            }
        }
    }

    // --- L3: scan pii_ref:{pii_uuid}: prefix in RocksDB ---
    if (l3_db_) {
        std::lock_guard<std::mutex> lock(l3_mutex_);

        if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
            THEMIS_WARN("L3 circuit breaker open, skipping L3 PII invalidation for uuid={}", pii_uuid);
            enhanced_metrics_.l3_circuit_breaker_open = true;
        } else {
            try {
                const std::string pii_ref_prefix = "pii_ref:" + pii_uuid + ":";
                std::vector<std::string> pii_ref_keys;
                std::vector<std::string> cache_keys;

                l3_db_->scanPrefix(pii_ref_prefix, [&](std::string_view key, std::string_view) {
                    pii_ref_keys.emplace_back(key);
                    // Extract fingerprint: pii_ref:{uuid}:{fingerprint}
                    std::string k(key);
                    if (k.size() > pii_ref_prefix.size()) {
                        cache_keys.emplace_back(
                            QUERY_CACHE_PREFIX + k.substr(pii_ref_prefix.size()));
                    }
                    return true;
                });

                for (const auto& ck : cache_keys) {
                    if (l3_db_->del(ck)) {
                        count++;
                    }
                }
                for (const auto& rk : pii_ref_keys) {
                    l3_db_->del(rk);
                }

                if (l3_circuit_breaker_) {
                    l3_circuit_breaker_->recordSuccess();
                    enhanced_metrics_.l3_circuit_breaker_open = false;
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("Failed L3 PII invalidation for uuid={}: {}", pii_uuid, e.what());
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

    THEMIS_INFO("GDPR PII purge: invalidated {} cache entries for pii_uuid={}", count, pii_uuid);
    return count;
}

bool AdaptiveQueryCache::updateTenantQuota(const std::string& tenant_id, size_t quota_bytes) {
    if (!config_.enable_tenant_isolation || tenant_id.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(tenant_mutex_);
    if (quota_bytes == 0) {
        tenant_quota_overrides_.erase(tenant_id);
    } else {
        tenant_quota_overrides_[tenant_id] = quota_bytes;
    }
    // Ensure a metrics entry exists so the tenant appears in getTenantStats()
    tenant_metrics_[tenant_id];  // default-insert if absent
    THEMIS_INFO("Updated quota for tenant {}: {} bytes", tenant_id, quota_bytes);
    return true;
}

size_t AdaptiveQueryCache::getEffectiveTenantQuota(const std::string& tenant_id) const {
    // Precondition: caller must hold tenant_mutex_
    auto it = tenant_quota_overrides_.find(tenant_id);
    if (it != tenant_quota_overrides_.end()) {
        return it->second;
    }
    return config_.per_tenant_max_bytes;
}

nlohmann::json AdaptiveQueryCache::getCircuitBreakerStatus() const {
    nlohmann::json status;
    if (!l3_circuit_breaker_) {
        status["enabled"] = false;
        status["state"] = "CLOSED";
        status["failure_count"] = 0;
        return status;
    }

    status["enabled"] = true;
    status["failure_count"] = l3_circuit_breaker_->getFailureCount();

    switch (l3_circuit_breaker_->getState()) {
        case cache::CircuitBreaker::State::CLOSED:
            status["state"] = "CLOSED";
            break;
        case cache::CircuitBreaker::State::OPEN:
            status["state"] = "OPEN";
            break;
        case cache::CircuitBreaker::State::HALF_OPEN:
            status["state"] = "HALF_OPEN";
            break;
    }

    return status;
}

void AdaptiveQueryCache::resetCircuitBreaker() {
    if (!l3_circuit_breaker_) {
        return;
    }
    l3_circuit_breaker_->reset();
    enhanced_metrics_.l3_circuit_breaker_open = false;
    THEMIS_INFO("L3 circuit breaker reset to CLOSED by admin request");
}

// ---------------------------------------------------------------------------
// Phase 3: Cache Warmup and Snapshot helpers
// ---------------------------------------------------------------------------

namespace {

/// Decode standard (+ URL-safe) base64 to a string. Returns empty on error.
static std::string warmupBase64Decode(const std::string& input) {
    static const char kChars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::array<int, 256> T;
    T.fill(-1);
    for (int i = 0; i < 64; ++i) T[static_cast<unsigned char>(kChars[i])] = i;
    T[static_cast<unsigned char>('-')] = 62;
    T[static_cast<unsigned char>('_')] = 63;

    std::string out;
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (c == '=') break;
        int tv = T[c];
        if (tv == -1) break;
        val = (val << 6) + tv;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

/// Encode bytes to standard base64.
static std::string warmupBase64Encode(const std::string& input) {
    static const char kChars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(kChars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(kChars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

/// Return true iff `s` is a 64-character lowercase hex string (SHA-256 fingerprint).
static bool isValidFingerprint(const std::string& s) {
    if (s.size() != 64) return false;
    for (char c : s) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// warmupFromLog
// ---------------------------------------------------------------------------

AdaptiveQueryCache::WarmupResult
AdaptiveQueryCache::warmupFromLog(const std::string& log_path, size_t max_entries) {
    WarmupResult result;

    std::ifstream file(log_path);
    if (!file.is_open()) {
        result.ok    = false;
        result.error = "Cannot open warmup log: " + log_path;
        THEMIS_WARN("Cache warmup failed: {}", result.error);
        return result;
    }

    // Hard cap: l1_max_entries / 2 to reserve headroom, further capped by caller.
    const size_t headroom_cap = config_.l1_max_entries / 2;
    const size_t effective_cap = (max_entries > 0)
                                 ? std::min(max_entries, headroom_cap)
                                 : headroom_cap;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        ++result.entries_total;

        if (result.entries_loaded >= effective_cap) {
            ++result.entries_skipped;
            continue;
        }

        // --- Parse line ---
        nlohmann::json obj;
        try {
            obj = nlohmann::json::parse(line);
        } catch (...) {
            THEMIS_DEBUG("Cache warmup: skipping malformed JSON line {}", result.entries_total);
            ++result.entries_skipped;
            continue;
        }

        // Validate required fields
        if (!obj.contains("key") || !obj["key"].is_string() ||
            !obj.contains("value_b64") || !obj["value_b64"].is_string()) {
            ++result.entries_skipped;
            continue;
        }

        std::string fingerprint = obj["key"].get<std::string>();
        if (!isValidFingerprint(fingerprint)) {
            THEMIS_DEBUG("Cache warmup: invalid fingerprint '{}' at line {}", fingerprint.substr(0, 16), result.entries_total);
            ++result.entries_skipped;
            continue;
        }

        // Decode value.
        // Note: warmupBase64Decode() returns "" both on a real decode error and
        // when the input decodes to an empty byte sequence.  An empty decoded
        // string can never be valid JSON (the smallest valid JSON is "null",
        // "true", "{}", or "[]"), so skipping it is intentional and correct.
        std::string decoded = warmupBase64Decode(obj["value_b64"].get<std::string>());
        if (decoded.empty()) {
            ++result.entries_skipped;
            continue;
        }

        // Validate size against L1 limit (warmup targets L1)
        if (decoded.size() > config_.l1_max_entry_size) {
            THEMIS_DEBUG("Cache warmup: entry {} exceeds L1 size limit ({} > {})",
                         fingerprint.substr(0, 16), decoded.size(), config_.l1_max_entry_size);
            ++result.entries_skipped;
            continue;
        }

        nlohmann::json value_json;
        try {
            value_json = nlohmann::json::parse(decoded);
        } catch (...) {
            THEMIS_WARN("Cache warmup: skipping entry {} – invalid JSON after base64 decode",
                        fingerprint.substr(0, 16));
            ++result.entries_skipped;
            continue;
        }

        const std::string tenant_id = obj.value("tenant", std::string{});
        const int ttl_s = obj.value("ttl_remaining_s", config_.l1_ttl_seconds);
        if (ttl_s <= 0) {
            // Already expired
            ++result.entries_skipped;
            continue;
        }

        // --- Insert directly into L1, bypassing the rate limiter ---
        {
            std::string key = (config_.enable_tenant_isolation && !tenant_id.empty())
                              ? makeTenantKey(fingerprint, tenant_id)
                              : fingerprint;

            // Quota check (must still honour per-tenant quota)
            if (!checkTenantQuota(tenant_id, decoded.size())) {
                THEMIS_DEBUG("Cache warmup: tenant '{}' quota exceeded, skipping entry", tenant_id);
                ++result.entries_skipped;
                continue;
            }

            L1Entry entry;
            entry.result          = std::move(value_json);
            entry.created_at_ms   = getCurrentTimeMs();
            entry.last_accessed_ms = entry.created_at_ms;
            entry.access_count    = 0;
            entry.ttl_seconds     = ttl_s;

            std::lock_guard<std::mutex> lock(l1_mutex_);
            // Evict oldest L1 entry if at capacity
            if (l1_cache_.size() >= config_.l1_max_entries) {
                evictLRU(CacheLevel::HOT);
            }
            int64_t insert_ms = entry.created_at_ms;
            l1_cache_[key] = std::move(entry);
            l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(insert_ms));
        }

        ++result.entries_loaded;

        // Update tenant metrics (bytes only; warmup does not count as a hit)
        if (config_.enable_tenant_isolation && !tenant_id.empty()) {
            std::lock_guard<std::mutex> lock(tenant_mutex_);
            tenant_metrics_[tenant_id].bytes_used += decoded.size();
        }
    }

    // Report to Prometheus MetricsCollector
    auto& mc = observability::MetricsCollector::getInstance();
    mc.addCounter("themis_cache_warmup_entries_loaded_total",
                  static_cast<int64_t>(result.entries_loaded));

    THEMIS_INFO("Cache warmup complete: loaded={}, skipped={}, total_lines={}",
                result.entries_loaded, result.entries_skipped, result.entries_total);
    return result;
}

// ---------------------------------------------------------------------------
// exportSnapshot
// ---------------------------------------------------------------------------

AdaptiveQueryCache::WarmupResult
AdaptiveQueryCache::exportSnapshot(const std::string& out_path) const {
    WarmupResult result;

    // Prefix used for tenant-scoped keys: "tenant:{id}:{fingerprint}"
    static const std::string kTenantPrefix = "tenant:";

    std::ofstream file(out_path, std::ios::trunc);
    if (!file.is_open()) {
        result.ok    = false;
        result.error = "Cannot open snapshot file for writing: " + out_path;
        THEMIS_WARN("Cache snapshot export failed: {}", result.error);
        return result;
    }

    const int64_t now_ms = getCurrentTimeMs();

    // Export live L1 entries
    {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        for (const auto& [key, entry] : l1_cache_) {
            if (isExpired(entry.created_at_ms, entry.ttl_seconds)) continue;

            // Derive fingerprint: strip tenant prefix if present
            std::string fp = key;
            if (fp.rfind(kTenantPrefix, 0) == 0) {
                auto pos = fp.find(':', kTenantPrefix.size());
                if (pos != std::string::npos) fp = fp.substr(pos + 1);
            }

            int remaining_ttl = entry.ttl_seconds
                - static_cast<int>((now_ms - entry.created_at_ms) / 1000);
            if (remaining_ttl <= 0) continue;

            std::string value_str = entry.result.dump();
            std::string value_b64 = warmupBase64Encode(value_str);

            // Extract tenant from original key
            std::string tenant_id;
            if (key.rfind(kTenantPrefix, 0) == 0) {
                auto pos = key.find(':', kTenantPrefix.size());
                if (pos != std::string::npos)
                    tenant_id = key.substr(kTenantPrefix.size(), pos - kTenantPrefix.size());
            }

            nlohmann::json line_obj = {
                {"key",             fp},
                {"value_b64",       value_b64},
                {"ttl_remaining_s", remaining_ttl},
                {"tenant",          tenant_id}
            };
            file << line_obj.dump() << '\n';
            ++result.entries_written;
            ++result.entries_total;
        }
    }

    // Export live L2 entries (decompress to get original JSON)
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        for (const auto& [key, entry] : l2_cache_) {
            if (isExpired(entry.created_at_ms, entry.ttl_seconds)) continue;

            int remaining_ttl = entry.ttl_seconds
                - static_cast<int>((now_ms - entry.created_at_ms) / 1000);
            if (remaining_ttl <= 0) continue;

            // Decompress
            std::vector<uint8_t> raw = utils::zstd_decompress(entry.compressed_result);
            if (raw.empty()) continue;
            std::string value_str(raw.begin(), raw.end());

            // Validate JSON
            try {
                nlohmann::json::parse(value_str);
            } catch (...) {
                THEMIS_WARN("Cache snapshot export: skipping L2 entry with invalid JSON for key={}", key.substr(0, 16));
                continue;
            }

            // Derive fingerprint and tenant
            std::string fp = key;
            std::string tenant_id;
            if (fp.rfind(kTenantPrefix, 0) == 0) {
                auto pos = fp.find(':', kTenantPrefix.size());
                if (pos != std::string::npos) {
                    tenant_id = fp.substr(kTenantPrefix.size(), pos - kTenantPrefix.size());
                    fp = fp.substr(pos + 1);
                }
            }

            std::string value_b64 = warmupBase64Encode(value_str);
            nlohmann::json line_obj = {
                {"key",             fp},
                {"value_b64",       value_b64},
                {"ttl_remaining_s", remaining_ttl},
                {"tenant",          tenant_id}
            };
            file << line_obj.dump() << '\n';
            ++result.entries_written;
            ++result.entries_total;
        }
    }

    if (!file.good()) {
        result.ok    = false;
        result.error = "I/O error while writing snapshot to: " + out_path;
        THEMIS_WARN("Cache snapshot export I/O error: {}", out_path);
        return result;
    }

    THEMIS_INFO("Cache snapshot exported: {} entries to {}", result.entries_written, out_path);
    return result;
}

// ---------------------------------------------------------------------------
// Phase 4: Predictive Pre-Fetching
// ---------------------------------------------------------------------------

void AdaptiveQueryCache::recordQueryAccess(const std::string& fingerprint,
                                            const std::string& tenant_id) {
    if (prefetcher_) {
        prefetcher_->recordQueryAccess(fingerprint, tenant_id);
    }
}

std::vector<std::string> AdaptiveQueryCache::getPrefetchCandidates(
    const std::string& fingerprint,
    const std::string& tenant_id) const {
    if (!prefetcher_) return {};

    auto candidates = prefetcher_->getPrefetchCandidates(fingerprint, tenant_id);
    if (!candidates.empty()) {
        // enhanced_metrics_ is exposed via getEnhancedMetrics() (CacheMetrics format);
        // the prefetcher's internal counter is returned by getPrefetchStats().
        // Both are kept in sync here so each API surface is self-consistent.
        enhanced_metrics_.prefetch_candidates_generated++;
        prefetcher_->recordCandidatesGenerated();
    }
    return candidates;
}

nlohmann::json AdaptiveQueryCache::getPrefetchStats() const {
    if (!prefetcher_) {
        return {{"enabled", false}};
    }
    nlohmann::json j = prefetcher_->getStats();
    j["enabled"] = true;
    // Enrich with the hit counter maintained in enhanced_metrics_
    j["prefetch_hits_from_metrics"] = enhanced_metrics_.prefetch_hits.load();
    return j;
}

// ============================================================================
// Phase 4: Cache Replication for High-Availability Multi-Node Deployments
// ============================================================================

void AdaptiveQueryCache::setCoordinator(
    std::shared_ptr<cache::ICacheCoordinator> coordinator)
{
    std::lock_guard<std::mutex> lk(coordinator_mutex_);
    coordinator_ = coordinator;

    if (!coordinator_) {
        THEMIS_INFO("AdaptiveQueryCache: replication coordinator removed");
        return;
    }

    // Subscribe for entries replicated from peer nodes
    coordinator_->subscribeEntries(
        [this](const cache::ReplicationMessage& msg) {
            applyReplicatedEntry(msg);
        });

    // Subscribe for invalidations propagated from peer nodes
    coordinator_->subscribeInvalidations(
        [this](const cache::ReplicationMessage& msg) {
            applyReplicatedInvalidation(msg);
        });

    THEMIS_INFO("AdaptiveQueryCache: replication coordinator registered ({})",
                coordinator_->name());
}

nlohmann::json AdaptiveQueryCache::getReplicationStats() const {
    std::lock_guard<std::mutex> lk(coordinator_mutex_);
    if (!coordinator_) {
        return {{"enabled", false}};
    }
    auto stats = coordinator_->getStats();
    stats["enabled"] = true;
    return stats;
}

void AdaptiveQueryCache::applyReplicatedEntry(const cache::ReplicationMessage& msg) {
    // Replicate only L1/L2; L3 (RocksDB) is assumed shared or node-local
    // and does not need replication from the coordinator bus.
    if (msg.result.is_null() || !msg.result.is_object()) {
        return;
    }

    int64_t now_ms = getCurrentTimeMs();
    std::string result_str = msg.result.dump();
    size_t result_size = result_str.size();

    // Honour per-entry size limit and tenant quota checks
    if (config_.enable_size_limits && !isWithinSizeLimit(result_size)) {
        return;
    }
    if (!checkTenantQuota(msg.tenant_id, result_size)) {
        return;
    }

    const std::string& key = msg.key;
    int ttl_seconds = msg.ttl_seconds > 0 ? msg.ttl_seconds : config_.l1_ttl_seconds;

    if (result_size < config_.l1_max_entry_size) {
        std::lock_guard<std::mutex> lock(l1_mutex_);
        if (l1_cache_.count(key) == 0) {   // Don't overwrite a locally fresher entry
            if (l1_cache_.size() >= config_.l1_max_entries) {
                evictLRU(CacheLevel::HOT);
            }
            L1Entry entry;
            entry.result           = msg.result;
            entry.created_at_ms    = now_ms;
            entry.last_accessed_ms = now_ms;
            entry.access_count     = 0;
            entry.ttl_seconds      = ttl_seconds;
            entry.window_start_ms  = now_ms;
            entry.window_count     = 0;
            l1_cache_[key]         = std::move(entry);
            l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
            enhanced_metrics_.total_bytes_cached += result_size;
        }
    } else if (result_size < config_.l2_max_entry_size) {
        auto compressed = utils::zstd_compress(result_str, config_.l2_compression_level);
        if (compressed.empty()) return;

        std::lock_guard<std::mutex> lock(l2_mutex_);
        if (l2_cache_.count(key) == 0) {
            if (l2_cache_.size() >= config_.l2_max_entries) {
                evictLRU(CacheLevel::WARM);
            }
            L2Entry entry;
            entry.compressed_result = std::move(compressed);
            entry.created_at_ms     = now_ms;
            entry.last_accessed_ms  = now_ms;
            entry.access_count      = 0;
            entry.ttl_seconds       = ttl_seconds;
            entry.window_start_ms   = now_ms;
            entry.window_count      = 0;
            l2_cache_[key]          = std::move(entry);
            l2_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
            enhanced_metrics_.total_bytes_cached += result_size;
        }
    }
}

void AdaptiveQueryCache::applyReplicatedInvalidation(const cache::ReplicationMessage& msg) {
    // Peer invalidated a key/pattern – evict matching entries from L1 and L2 only.
    // L3 (RocksDB) is considered either shared or independently managed per-node.
    const std::string& pattern = msg.key;
    if (pattern.empty()) return;

    try {
        std::regex re(pattern);

        {
            std::lock_guard<std::mutex> lock(l1_mutex_);
            for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
                if (std::regex_search(it->first, re)) {
                    l1_eviction_strategy_->onRemove(it->first);
                    it = l1_cache_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(l2_mutex_);
            for (auto it = l2_cache_.begin(); it != l2_cache_.end();) {
                if (std::regex_search(it->first, re)) {
                    l2_eviction_strategy_->onRemove(it->first);
                    it = l2_cache_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    } catch (const std::regex_error& e) {
        THEMIS_WARN("CacheReplication: invalid pattern received from peer: {} ({})",
                    pattern, e.what());
    }
}

// ============================================================================
// Phase 4: Cache Replication for High-Availability
// ============================================================================

void AdaptiveQueryCache::setReplicationListener(
        std::shared_ptr<cache::ICacheReplicationListener> listener) {
    std::lock_guard<std::mutex> lock(replication_mutex_);
    replication_listener_ = std::move(listener);
    if (replication_listener_) {
        THEMIS_INFO("AdaptiveQueryCache: replication listener registered ({})",
                    replication_listener_->replicaId());
    } else {
        THEMIS_INFO("AdaptiveQueryCache: replication listener unregistered");
    }
}

} // namespace themis
