/**
 * @file adaptive_query_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=36, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cache/adaptive_query_cache.h"
#include <stdexcept>
#include "cache/cache_replication.h"
#include "cache/eviction_policy.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/zstd_codec.h"
#include "utils/logger.h"
#include "observability/metrics_collector.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <regex>
#include <shared_mutex>
#include <sstream>
#include <thread>

#include "cache/cache_replication.h"
#include "cache/eviction_policy.h"
#include "observability/metrics_collector.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include "utils/zstd_codec.h"

namespace themis {

// Constants
constexpr size_t QUERY_CACHE_PREFIX_LEN  = 12; // Length of "query_cache:"
constexpr const char *QUERY_CACHE_PREFIX = "query_cache:";
constexpr int RETRY_BACKOFF_MULTIPLIER   = 2;             // Exponential backoff multiplier
constexpr int64_t ADAPTIVE_TTL_WINDOW_MS = 5 * 60 * 1000; // 5-minute sliding window

// C1: Timeout-safe L3 lock — 1 000 ms deadline before returning RESOURCE_EXHAUSTED.
constexpr auto kL3LockTimeoutMs = std::chrono::milliseconds(1000);

// C2: L3 initialization retry bounds — 3 attempts with 1 s / 2 s backoff (max 3 s total).
// The retry loop in the constructor uses these constants so the total init timeout is explicit.
constexpr int kL3InitMaxRetries       = 3;
constexpr int kL3InitRetryDelayMs     = 1000; // initial backoff
constexpr int kL3InitMaxTotalDelayMs  = 3000; // sum of all sleep_for calls across retries

// C4: AI/LLM safety constants — hard caps applied unconditionally in put().
constexpr size_t  kAbsoluteMaxEntrySizeBytes = 67108864; // 64 MiB
constexpr int     kAbsoluteMaxTTLSeconds      = 86400;       // 24 hours

AdaptiveQueryCache::AdaptiveQueryCache(const Config &config) : config_(config) {
    // Phase 2: Validate configuration on startup
    std::string validation_error = {};
    if (!config_.validate(&validation_error)) {
        throw std::invalid_argument("Invalid cache configuration: " + validation_error);
    }

    THEMIS_INFO("AdaptiveQueryCache initialized: L1={} entries, L2={} entries, L3=RocksDB", config_.l1_max_entries,
                config_.l2_max_entries);

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
        rate_limiter_                     = std::make_unique<cache::RateLimiter>(rl_config);
        THEMIS_INFO("Rate limiting enabled: {} requests/sec", config_.max_requests_per_second);
    }

    // Initialize circuit breaker for L3 (Phase 1: Fault Isolation)
    if (config_.enable_circuit_breaker) {
        cache::CircuitBreaker::Config cb_config;
        cb_config.failure_threshold = config_.cb_failure_threshold;
        cb_config.timeout_ms        = config_.cb_timeout_ms;
        l3_circuit_breaker_         = std::make_unique<cache::CircuitBreaker>(cb_config);
        THEMIS_INFO("Circuit breaker enabled for L3 cache (threshold={}, timeout={}ms)", cb_config.failure_threshold,
                    cb_config.timeout_ms);
    }

    // Initialize configurable eviction strategies for L1 and L2
    l1_eviction_strategy_ = cache::makeEvictionStrategy(config_.l1_eviction_policy, config_.l1_max_entries);
    l2_eviction_strategy_ = cache::makeEvictionStrategy(config_.l2_eviction_policy, config_.l2_max_entries);

    // Phase 4: Initialize predictive pre-fetcher
    if (config_.enable_predictive_prefetch) {
        cache::PredictivePrefetcher::Config pf_config;
        pf_config.max_tracked_keys             = config_.prefetch_max_tracked_keys;
        pf_config.max_predictions              = config_.prefetch_max_predictions;
        pf_config.min_transition_count         = config_.prefetch_min_transition_count;
        pf_config.min_confidence               = config_.prefetch_min_confidence;
        pf_config.enable_time_of_day_weighting = config_.prefetch_enable_time_of_day_weighting;
        pf_config.enable_ab_test               = config_.prefetch_enable_ab_test;
        prefetcher_                            = std::make_unique<cache::PredictivePrefetcher>(pf_config);
        THEMIS_INFO("Predictive pre-fetcher enabled: max_keys={}, max_predictions={}", pf_config.max_tracked_keys,
                    pf_config.max_predictions);
    }

    // Initialize L3 (RocksDB) cache with retry logic
    // Empty l3_db_path means L3 is disabled (l3_db_ stays null).
    if (!config_.l3_db_path.empty()) {
        // Bounded init: kL3InitMaxRetries attempts, max kL3InitMaxTotalDelayMs total sleep.
        int retry_count    = 0;
        int retry_delay_ms = kL3InitRetryDelayMs;

        while (retry_count < kL3InitMaxRetries) {
            try {
                RocksDBWrapper::Config db_config;
                db_config.db_path             = config_.l3_db_path;
                db_config.create_if_missing   = true;
                db_config.memtable_size_mb    = 64;  // 64MB write buffer
                db_config.block_cache_size_mb = 256; // small cache for query cache
                db_config.max_background_jobs = 2;

                l3_db_ = std::make_shared<RocksDBWrapper>(db_config);
                if (!l3_db_->open()) {
                    throw std::runtime_error("Failed to open L3 cache (RocksDB) at: " + config_.l3_db_path);
                }
                THEMIS_INFO("L3 cache (RocksDB) initialized at: {}", config_.l3_db_path);
                break; // Success
            } catch (const std::exception &e) {
                retry_count++;
                if (retry_count < kL3InitMaxRetries) {
                    THEMIS_WARN("Failed to initialize L3 cache (attempt {}/{}): {}. Retrying in {}ms...", retry_count,
                                kL3InitMaxRetries, e.what(), retry_delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
                    retry_delay_ms *= RETRY_BACKOFF_MULTIPLIER; // Exponential backoff
                } else {
                    THEMIS_WARN("Failed to initialize L3 cache after {} attempts: {}. L3 cache disabled.", kL3InitMaxRetries,
                                e.what());
                    l3_db_.reset();
                    if (l3_circuit_breaker_) {
                        l3_circuit_breaker_->recordFailure();
                        enhanced_metrics_.l3_circuit_breaker_trips++;
                    }
                }
            }
        } // while
    } // if !l3_db_path.empty()

    // Restore the Markov prefetch model from RocksDB (no-op if either is null)
    loadPrefetchModel();
}

AdaptiveQueryCache::~AdaptiveQueryCache() {
    // [C-4] Mark this object inactive BEFORE unregistering the coordinator.
    // Any in-flight callback that has already captured alive_guard_ will block
    // until this lock is released; once it sees alive=false it returns without
    // touching `this`. This acts as a synchronisation barrier between the
    // destructor and any concurrent coordinator callback.
    {
        std::lock_guard<std::mutex> alive_lock(alive_guard_->mutex);
        alive_guard_->alive = false;
    }
    // Phase 4: Deregister coordinator callbacks before releasing memory.
    // Any coordinator that outlives this cache would otherwise hold a [this]
    // lambda pointing to freed memory, causing use-after-free on the next
    // publication.
    setCoordinator(nullptr);
    clear();
}

std::string AdaptiveQueryCache::generateFingerprint(const std::string &query, const nlohmann::json &params,
                                                    const std::string &tenant_id) const {
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
    SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(), hash);

    // Convert to hex string
    std::ostringstream ss = {};
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }

    return ss.str();
}

std::optional<AdaptiveQueryCache::CacheEntry> AdaptiveQueryCache::get(const std::string &fingerprint,
                                                                      const std::string &tenant_id) {
    // Phase 2: Create tenant-scoped key if tenant isolation enabled
    std::string key
        = (config_.enable_tenant_isolation && !tenant_id.empty()) ? makeTenantKey(fingerprint, tenant_id) : fingerprint;
    // Phase 2: Check rate limiter
    if (rate_limiter_ && !rate_limiter_->tryAcquire()) {
        enhanced_metrics_.rate_limited_requests++;
        THEMIS_DEBUG("Request rate limited for fingerprint: {}", fingerprint.substr(0, 16));
        return std::nullopt;
    }

    int64_t now_ms = getCurrentTimeMs();

    // Try L1 (HOT) - lock-free concurrent reads via shared_lock
    {
        std::shared_lock<std::shared_mutex> lock(l1_mutex_);
        auto it = l1_cache_.find(key);
        if (it != l1_cache_.end()) {
            L1Entry *ptr = it->second.get();

            // Check expiry flag first (set by a previous reader via CAS)
            if (ptr->expired_flag.load(std::memory_order_relaxed)) {
                // Entry already marked for lazy cleanup; fall through to L2
            } else if (isExpired(ptr->created_at_ms.load(std::memory_order_relaxed),
                                 ptr->ttl_seconds.load(std::memory_order_relaxed))) {
                // Mark entry for lazy cleanup via CAS; only the first winner records the metric
                bool expected = false;
                if (ptr->expired_flag.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
                    stats_.evictions++;
                    enhanced_metrics_.evictions++;
                }
                // Fall through to L2
            } else {
                // Cache hit – update metadata lock-free
                ptr->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
                int64_t new_count = ptr->access_count.fetch_add(1, std::memory_order_relaxed) + 1;

                // Phase 3: Adaptive TTL tuning via sliding 5-minute window
                if (config_.enable_adaptive_ttl) {
                    int64_t ws = ptr->window_start_ms.load(std::memory_order_relaxed);
                    if (now_ms - ws >= ADAPTIVE_TTL_WINDOW_MS) {
                        // Window elapsed – attempt to claim the reset via CAS on window_start_ms
                        if (ptr->window_start_ms.compare_exchange_strong(ws, now_ms, std::memory_order_relaxed)) {
                            uint32_t wc = ptr->window_count.exchange(1, std::memory_order_relaxed);
                            if (ws > 0 && wc <= 1) {
                                int old_ttl = ptr->ttl_seconds.load(std::memory_order_relaxed);
                                int new_ttl
                                    = std::max(static_cast<int>(old_ttl * 0.5), config_.adaptive_ttl_min_seconds);
                                if (ptr->ttl_seconds.compare_exchange_strong(old_ttl, new_ttl,
                                                                             std::memory_order_relaxed)) {
                                    enhanced_metrics_.ttl_shortened_total++;
                                }
                            }
                            ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
                        }
                    } else {
                        uint32_t wc = ptr->window_count.fetch_add(1, std::memory_order_relaxed) + 1;
                        if (wc >= 10) {
                            int old_ttl = ptr->ttl_seconds.load(std::memory_order_relaxed);
                            int new_ttl = std::min(static_cast<int>(old_ttl * 1.5), config_.adaptive_ttl_max_seconds);
                            if (ptr->ttl_seconds.compare_exchange_strong(old_ttl, new_ttl, std::memory_order_relaxed)) {
                                enhanced_metrics_.ttl_extended_total++;
                            }
                        } else {
                            int new_ttl = calculateAdaptiveTTL(new_count);
                            ptr->ttl_seconds.store(new_ttl, std::memory_order_relaxed);
                        }
                        ptr->created_at_ms.store(now_ms, std::memory_order_relaxed);
                    }
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

                // Build result (copy result under shared_lock – result field is read-only after insert)
                CacheEntry cache_result;
                cache_result.query_fingerprint = fingerprint;
                cache_result.result            = ptr->result;
                cache_result.level             = CacheLevel::HOT;
                cache_result.created_at_ms     = ptr->created_at_ms.load(std::memory_order_relaxed);
                cache_result.last_accessed_ms  = ptr->last_accessed_ms.load(std::memory_order_relaxed);
                cache_result.access_count      = ptr->access_count.load(std::memory_order_relaxed);
                cache_result.ttl_seconds       = ptr->ttl_seconds.load(std::memory_order_relaxed);

                THEMIS_DEBUG("L1 cache hit: fingerprint={}", fingerprint.substr(0, 16));
                return cache_result;
            }
        }
    }

    // Try L2 (WARM) - compressed
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        auto it = l2_cache_.find(key);
        if (it != l2_cache_.end()) {
            auto &entry = it->second;

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
                                int old_ttl       = entry.ttl_seconds;
                                entry.ttl_seconds = std::max(static_cast<int>(entry.ttl_seconds * 0.5),
                                                             config_.adaptive_ttl_min_seconds);
                                if (entry.ttl_seconds < old_ttl) {
                                    enhanced_metrics_.ttl_shortened_total++;
                                }
                            }
                            entry.window_start_ms = now_ms;
                            entry.window_count    = 1;
                        } else {
                            entry.window_count++;
                            // Hot-key policy: extend TTL when heavily accessed in window
                            if (entry.window_count >= 10) {
                                int old_ttl       = entry.ttl_seconds;
                                entry.ttl_seconds = std::min(static_cast<int>(entry.ttl_seconds * 1.5),
                                                             config_.adaptive_ttl_max_seconds);
                                if (entry.ttl_seconds > old_ttl) {
                                    enhanced_metrics_.ttl_extended_total++;
                                }
                            } else {
                                entry.ttl_seconds = calculateAdaptiveTTL(entry.access_count);
                            }
                        }
                        entry.created_at_ms = now_ms; // Reset TTL window on each access
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

                    // Promote frequently accessed entries to L1.
                    if (entry.access_count >= 3) {
                        auto l1_entry    = std::make_unique<L1Entry>();
                        l1_entry->result = result;
                        l1_entry->created_at_ms.store(entry.created_at_ms, std::memory_order_relaxed);
                        l1_entry->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
                        l1_entry->access_count.store(entry.access_count, std::memory_order_relaxed);
                        l1_entry->ttl_seconds.store(entry.ttl_seconds, std::memory_order_relaxed);
                        l1_entry->window_start_ms.store(entry.window_start_ms, std::memory_order_relaxed);
                        l1_entry->window_count.store(entry.window_count, std::memory_order_relaxed);

                        std::unique_lock<std::shared_mutex> l1_lock(l1_mutex_);
                        if (static_cast<int>(l1_cache_.size()) > = config_.l1_max_entries) {
                            evictLRU(CacheLevel::HOT);
                        }
                        l2_eviction_strategy_->onRemove(key);
                        l1_cache_[key] = std::move(l1_entry);
                        {
                            std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                            l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
                        }
                        l2_cache_.erase(it);
                        stats_.promotions++;
                        enhanced_metrics_.promotions++;

                        THEMIS_DEBUG("Promoted L2->L1: key={}", key.substr(0, 16));
                    }

                    // Return entry
                    CacheEntry cache_entry;
                    cache_entry.query_fingerprint = key;
                    cache_entry.result            = result;
                    cache_entry.level             = CacheLevel::WARM;
                    cache_entry.created_at_ms     = entry.created_at_ms;
                    cache_entry.last_accessed_ms  = entry.last_accessed_ms;
                    cache_entry.access_count      = entry.access_count;
                    cache_entry.ttl_seconds       = entry.ttl_seconds;

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

        // C1: Timeout-safe L3 lock — avoid indefinite blocking on slow RocksDB I/O.
        {
            auto l3_deadline = std::chrono::steady_clock::now() + kL3LockTimeoutMs;
            if (!l3_mutex_.try_lock_until(l3_deadline)) {
                THEMIS_WARN("{{\"event\":\"cache_lock_timeout\",\"operation\":\"l3_cache_access\",\"timeout_ms\":1000}}");
                stats_.misses++;
                enhanced_metrics_.misses++;
                return std::nullopt;
            }
        }
        std::unique_lock<std::timed_mutex> lock(l3_mutex_, std::adopt_lock);

        std::string l3_key = QUERY_CACHE_PREFIX + fingerprint;
        std::optional<std::vector<uint8_t>> result;

        try {
            result = l3_db_->get(l3_key);
        } catch (const std::exception &e) {
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
                int ttl_seconds       = entry_json["ttl_seconds"];

                // Check expiration
                if (isExpired(created_at_ms, ttl_seconds)) {
                    l3_db_->del(key);
                    stats_.evictions++;
                    enhanced_metrics_.evictions++;
                } else {
                    // Update access stats
                    int64_t access_count           = entry_json["access_count"].get<int64_t>() + 1;
                    entry_json["access_count"]     = access_count;
                    entry_json["last_accessed_ms"] = now_ms;

                    // Phase 3: Adaptive TTL tuning via sliding 5-minute window
                    if (config_.enable_adaptive_ttl) {
                        int64_t window_start_ms = entry_json.value("window_start_ms", (int64_t)0);
                        uint32_t window_count   = entry_json.value("window_count", (uint32_t)0);

                        if (now_ms - window_start_ms >= ADAPTIVE_TTL_WINDOW_MS) {
                            // Window elapsed: apply cold-key policy on previous window count
                            if (window_start_ms > 0 && window_count <= 1) {
                                int old_ttl = ttl_seconds;
                                ttl_seconds
                                    = std::max(static_cast<int>(ttl_seconds * 0.5), config_.adaptive_ttl_min_seconds);
                                if (ttl_seconds < old_ttl) {
                                    enhanced_metrics_.ttl_shortened_total++;
                                }
                                entry_json["ttl_seconds"] = ttl_seconds;
                            }
                            entry_json["window_start_ms"] = now_ms;
                            entry_json["window_count"]    = 1;
                        } else {
                            window_count++;
                            entry_json["window_count"] = window_count;
                            // Hot-key policy: extend TTL when heavily accessed in window
                            if (window_count >= 10) {
                                int old_ttl = ttl_seconds;
                                ttl_seconds
                                    = std::min(static_cast<int>(ttl_seconds * 1.5), config_.adaptive_ttl_max_seconds);
                                if (ttl_seconds > old_ttl) {
                                    enhanced_metrics_.ttl_extended_total++;
                                }
                                entry_json["ttl_seconds"] = ttl_seconds;
                            } else {
                                ttl_seconds               = calculateAdaptiveTTL(access_count);
                                entry_json["ttl_seconds"] = ttl_seconds;
                            }
                        }
                        entry_json["created_at_ms"] = now_ms; // Reset TTL window on each access
                        created_at_ms               = now_ms;
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
                    cache_entry.result            = entry_json["result"];
                    cache_entry.level             = CacheLevel::COLD;
                    cache_entry.created_at_ms     = created_at_ms;
                    cache_entry.last_accessed_ms  = now_ms;
                    cache_entry.access_count      = access_count;
                    cache_entry.ttl_seconds       = ttl_seconds;

                    THEMIS_DEBUG("L3 cache hit: fingerprint={}", fingerprint.substr(0, 16));
                    return cache_entry;
                }
            } catch (const std::exception &e) {
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

bool AdaptiveQueryCache::put(const std::string &fingerprint, const nlohmann::json &query_params,
                             const nlohmann::json &result, const std::string &tenant_id,
                             const std::vector<std::string> &pii_uuids) {
    if (rate_limiter_ && !rate_limiter_->tryAcquire()) {
        enhanced_metrics_.rate_limited_requests++;
        THEMIS_DEBUG("Put request rate limited for fingerprint: {}", fingerprint.substr(0, 16));
        return false;
    }

    std::shared_ptr<cache::ICacheReplicationListener> rep_listener;
    {
        std::lock_guard<std::mutex> rep_lock(replication_mutex_);
        rep_listener = replication_listener_;
    }

    const int64_t now_ms         = getCurrentTimeMs();
    const std::string result_str = result.dump();
    const size_t result_size     = result_str.size();

    if (!checkTenantQuota(tenant_id, result_size)) {
        THEMIS_WARN("Tenant {} quota exceeded, rejecting entry", tenant_id);
        enhanced_metrics_.size_limit_rejections++;
        return false;
    }

    if (config_.enable_size_limits && !isWithinSizeLimit(result_size)) {
        THEMIS_WARN("Rejected cache entry due to size limit: size={}, max={}", result_size,
                    config_.max_total_entry_size);
        enhanced_metrics_.size_limit_rejections++;
        return false;
    }

    // C4: AI/LLM safety — absolute size hard-cap (64 MiB), independent of config.
    if (result_size > kAbsoluteMaxEntrySizeBytes) {
        THEMIS_WARN("{{\"event\":\"entry_size_exceeded\",\"operation\":\"cache_put\","
                    "\"size_bytes\":{},\"max_bytes\":{}}}",
                    result_size, kAbsoluteMaxEntrySizeBytes);
        enhanced_metrics_.size_limit_rejections++;
        return false;
    }

    // C4: AI/LLM safety — result must be a JSON object or array (not null/primitive).
    if (!result.is_object() && !result.is_array()) {
        THEMIS_WARN("{{\"event\":\"invalid_entry_type\",\"operation\":\"cache_put\","
                    "\"type\":\"{}\"}}",
                    result.type_name());
        enhanced_metrics_.size_limit_rejections++;
        return false;
    }

    const std::string key
        = (config_.enable_tenant_isolation && !tenant_id.empty()) ? makeTenantKey(fingerprint, tenant_id) : fingerprint;

    const CacheLevel level = selectCacheLevel(result_size);
    if (config_.enable_size_limits && !validateEntrySize(result_size, level)) {
        THEMIS_WARN("Entry size {} exceeds limit for cache level", result_size);
        enhanced_metrics_.size_limit_rejections++;
        return false;
    }

    const int ttl_seconds = config_.enable_adaptive_ttl
                                ? calculateAdaptiveTTL(0)
                                : (level == CacheLevel::HOT
                                       ? config_.l1_ttl_seconds
                                       : (level == CacheLevel::WARM ? config_.l2_ttl_seconds : config_.l3_ttl_seconds));

    // C4: AI/LLM safety — computed TTL must be in (0, 24h].
    if (ttl_seconds <= 0 || ttl_seconds > kAbsoluteMaxTTLSeconds) {
        THEMIS_WARN("{{\"event\":\"invalid_ttl\",\"operation\":\"cache_put\","
                    "\"ttl_seconds\":{}}}",
                    ttl_seconds);
        enhanced_metrics_.size_limit_rejections++;
        return false;
    }

    std::shared_ptr<cache::ICacheCoordinator> repl_coord = {};

    if (config_.enable_replication) {
        std::lock_guard<std::mutex> lk(coordinator_mutex_);
        repl_coord = coordinator_;
    }
    auto notifyCoordinator = [&]() {
        if (!repl_coord) {
            return;
        }
        try {
            repl_coord->publishEntry(key, result, ttl_seconds, tenant_id);
        } catch (const std::exception &e) {
            THEMIS_WARN("Cache replication publish failed: {}", e.what());
        }
    };

    if (config_.enable_write_through) {
        bool any_written = false;

        if (result_size < config_.l1_max_entry_size) {
            std::unique_lock<std::shared_mutex> l1_lock(l1_mutex_);
            if (static_cast<int>(l1_cache_.size()) > = config_.l1_max_entries) {
                evictLRU(CacheLevel::HOT);
            }
            auto l1_entry    = std::make_unique<L1Entry>();
            l1_entry->result = result;
            l1_entry->created_at_ms.store(now_ms, std::memory_order_relaxed);
            l1_entry->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
            l1_entry->access_count.store(1, std::memory_order_relaxed);
            l1_entry->ttl_seconds.store(config_.enable_adaptive_ttl ? calculateAdaptiveTTL(0) : config_.l1_ttl_seconds,
                                        std::memory_order_relaxed);
            l1_entry->window_start_ms.store(now_ms, std::memory_order_relaxed);
            l1_entry->window_count.store(0, std::memory_order_relaxed);
            l1_cache_[key] = std::move(l1_entry);
            {
                std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
            }
            any_written = true;
        }

        if (result_size < config_.l2_max_entry_size) {
            auto compressed = utils::zstd_compress(result_str, config_.l2_compression_level);
            if (!compressed.empty()) {
                std::lock_guard<std::mutex> l2_lock(l2_mutex_);
                if (static_cast<int>(l2_cache_.size()) > = config_.l2_max_entries) {
                    evictLRU(CacheLevel::WARM);
                }
                L2Entry l2_entry;
                l2_entry.compressed_result = std::move(compressed);
                l2_entry.created_at_ms     = now_ms;
                l2_entry.last_accessed_ms  = now_ms;
                l2_entry.access_count      = 1;
                l2_entry.ttl_seconds = config_.enable_adaptive_ttl ? calculateAdaptiveTTL(0) : config_.l2_ttl_seconds;
                l2_entry.window_start_ms = now_ms;
                l2_entry.window_count    = 0;
                enhanced_metrics_.total_bytes_compressed += l2_entry.compressed_result.size();
                l2_cache_[key] = std::move(l2_entry);
                l2_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
                any_written = true;
            } else {
                enhanced_metrics_.compression_failures++;
            }
        }

        if (l3_db_) {
            const bool l3_cb_ok = !l3_circuit_breaker_ || l3_circuit_breaker_->allowRequest();
            if (l3_cb_ok) {
                nlohmann::json l3_entry_json;
                l3_entry_json["result"]           = result;
                l3_entry_json["query_params"]     = query_params;
                l3_entry_json["created_at_ms"]    = now_ms;
                l3_entry_json["last_accessed_ms"] = now_ms;
                l3_entry_json["access_count"]     = 1;
                l3_entry_json["ttl_seconds"]
                    = config_.enable_adaptive_ttl ? calculateAdaptiveTTL(0) : config_.l3_ttl_seconds;
                l3_entry_json["window_start_ms"] = now_ms;
                l3_entry_json["window_count"]    = 0;

                // C1: Timeout-safe L3 lock for write-through path.
                auto l3_wt_deadline = std::chrono::steady_clock::now() + kL3LockTimeoutMs;
                if (!l3_mutex_.try_lock_until(l3_wt_deadline)) {
                    THEMIS_WARN("{{\"event\":\"cache_lock_timeout\",\"operation\":\"l3_write_through\",\"timeout_ms\":1000}}");
                    enhanced_metrics_.l3_write_errors++;
                    // Skip L3 for this write; L1/L2 writes may still succeed.
                } else {
                    std::unique_lock<std::timed_mutex> l3_lock(l3_mutex_, std::adopt_lock);
                    try {
                        const bool ok = l3_db_->put(QUERY_CACHE_PREFIX + fingerprint, l3_entry_json.dump());
                        if (ok) {
                            if (l3_circuit_breaker_) {
                                l3_circuit_breaker_->recordSuccess();
                                enhanced_metrics_.l3_circuit_breaker_open = false;
                            }
                            any_written = true;
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
                    } catch (const std::exception &e) {
                        THEMIS_WARN("Write-through: L3 cache write exception: {}", e.what());
                        enhanced_metrics_.l3_write_errors++;
                    }
                }
            }
        }

        if (any_written) {
            enhanced_metrics_.total_bytes_cached += result_size;
            enhanced_metrics_.write_through_total++;
            enhanced_metrics_.write_through_writes++;
            if (!pii_uuids.empty()) {
                std::lock_guard<std::mutex> plock(pii_index_mutex_);
                for (const auto &pii_uuid : pii_uuids) {
                    pii_key_index_[pii_uuid].insert(key);
                    pii_key_index_[pii_uuid].insert(fingerprint);
                }
            }
            if (config_.enable_tenant_isolation && !tenant_id.empty()) {
                std::lock_guard<std::mutex> tlock(tenant_mutex_);
                tenant_metrics_[tenant_id].bytes_used += result_size;
            }
            notifyCoordinator();
            if ([[maybe_unused]] rep_listener) {
                cache::CacheReplicationEvent ev;
                ev.type        = cache::CacheReplicationEventType::WRITE;
                ev.key         = key;
                ev.payload     = result_str;
                ev.tenant_id   = tenant_id;
                ev.ttl_seconds = ttl_seconds;
                (void)rep_listener->onReplicationEvent(ev);
            }
        }

        return any_written;
    }

    if (level == CacheLevel::HOT && result_size < config_.l1_max_entry_size) {
        {
            std::unique_lock<std::shared_mutex> lock(l1_mutex_);
            if (static_cast<int>(l1_cache_.size()) > = config_.l1_max_entries) {
                evictLRU(CacheLevel::HOT);
            }
            auto entry    = std::make_unique<L1Entry>();
            entry->result = result;
            entry->created_at_ms.store(now_ms, std::memory_order_relaxed);
            entry->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
            entry->access_count.store(1, std::memory_order_relaxed);
            entry->ttl_seconds.store(ttl_seconds, std::memory_order_relaxed);
            entry->window_start_ms.store(now_ms, std::memory_order_relaxed);
            entry->window_count.store(0, std::memory_order_relaxed);
            l1_cache_[key] = std::move(entry);
            {
                std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
            }
            enhanced_metrics_.total_bytes_cached += result_size;
        }
        if (config_.enable_tenant_isolation && !tenant_id.empty()) {
            std::lock_guard<std::mutex> tenant_lock(tenant_mutex_);
            tenant_metrics_[tenant_id].bytes_used += result_size;
        }
        if (!pii_uuids.empty()) {
            std::lock_guard<std::mutex> plock(pii_index_mutex_);
            for (const auto &pii_uuid : pii_uuids) {
                static_cast<void>(pii_key_index_[pii_uuid].insert(key));         // tenanted version
                static_cast<void>(pii_key_index_[pii_uuid].insert(fingerprint)); // untenanted version
            }
        }
        notifyCoordinator();
        if ([[maybe_unused]] rep_listener) {
            cache::CacheReplicationEvent ev;
            ev.type        = cache::CacheReplicationEventType::WRITE;
            ev.key         = key;
            ev.payload     = result_str;
            ev.tenant_id   = tenant_id;
            ev.ttl_seconds = ttl_seconds;
            (void)rep_listener->onReplicationEvent(ev);
        }
        return true;
    }

    if (level == CacheLevel::WARM && result_size < config_.l2_max_entry_size) {
        auto compressed = utils::zstd_compress(result_str, config_.l2_compression_level);
        if (compressed.empty()) {
            enhanced_metrics_.compression_failures++;
            return false;
        }
        size_t compressed_size = 0;
        {
            std::lock_guard<std::mutex> lock(l2_mutex_);
            if (static_cast<int>(l2_cache_.size()) > = config_.l2_max_entries) {
                evictLRU(CacheLevel::WARM);
            }
            L2Entry entry;
            entry.compressed_result = std::move(compressed);
            entry.created_at_ms     = now_ms;
            entry.last_accessed_ms  = now_ms;
            entry.access_count      = 1;
            entry.ttl_seconds       = ttl_seconds;
            entry.window_start_ms   = now_ms;
            entry.window_count      = 0;
            compressed_size         = entry.compressed_result.size();
            l2_cache_[key]          = std::move(entry);
            l2_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
            enhanced_metrics_.total_bytes_cached += result_size;
            enhanced_metrics_.total_bytes_compressed += compressed_size;
        }
        if (!pii_uuids.empty()) {
            std::lock_guard<std::mutex> plock(pii_index_mutex_);
            for (const auto &pii_uuid : pii_uuids) {
                static_cast<void>(pii_key_index_[pii_uuid].insert(fingerprint)); // untenanted version
                static_cast<void>(pii_key_index_[pii_uuid].insert(key));         // tenanted version
            }
        }
        notifyCoordinator();
        if ([[maybe_unused]] rep_listener) {
            cache::CacheReplicationEvent ev;
            ev.type        = cache::CacheReplicationEventType::WRITE;
            ev.key         = fingerprint;
            ev.payload     = result_str;
            ev.tenant_id   = tenant_id;
            ev.ttl_seconds = ttl_seconds;
            (void)rep_listener->onReplicationEvent(ev);
        }
        return true;
    }

    if (l3_db_) {
        if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
            enhanced_metrics_.l3_circuit_breaker_open = true;
            return false;
        }

        nlohmann::json entry_json;
        entry_json["result"]           = result;
        entry_json["query_params"]     = query_params;
        entry_json["created_at_ms"]    = now_ms;
        entry_json["last_accessed_ms"] = now_ms;
        entry_json["access_count"]     = 1;
        entry_json["ttl_seconds"]      = ttl_seconds;
        entry_json["window_start_ms"]  = now_ms;
        entry_json["window_count"]     = 0;

        const std::string l3_key  = QUERY_CACHE_PREFIX + fingerprint;
        const std::string payload = entry_json.dump();
        bool ok                   = false;
        {
            // C1: Timeout-safe L3 lock — 1 000 ms deadline.
            auto l3_deadline = std::chrono::steady_clock::now() + kL3LockTimeoutMs;
            if (!l3_mutex_.try_lock_until(l3_deadline)) {
                THEMIS_WARN("{{\"event\":\"cache_lock_timeout\",\"operation\":\"l3_cache_write\",\"timeout_ms\":1000}}");
                enhanced_metrics_.l3_write_errors++;
                return false;
            }
            std::unique_lock<std::timed_mutex> lock(l3_mutex_, std::adopt_lock);
            try {
                ok = l3_db_->put(l3_key, payload);
                if (ok) {
                    if (l3_circuit_breaker_) {
                        l3_circuit_breaker_->recordSuccess();
                        enhanced_metrics_.l3_circuit_breaker_open = false;
                    }
                    enhanced_metrics_.total_bytes_cached += result_size;
                    for (const auto &pii_uuid : pii_uuids) {
                        static_cast<void>(l3_db_->put("pii_ref:" + pii_uuid + ":" + fingerprint, ""));
                    }
                } else {
                    enhanced_metrics_.l3_write_errors++;
                }
            } catch (const std::exception &e) {
                THEMIS_WARN("L3 cache write exception: {}", e.what());
                enhanced_metrics_.l3_write_errors++;
                return false;
            }
        }
        if (ok) {
            notifyCoordinator();
            if ([[maybe_unused]] rep_listener) {
                cache::CacheReplicationEvent ev;
                ev.type        = cache::CacheReplicationEventType::WRITE;
                ev.key         = l3_key;
                ev.payload     = payload;
                ev.tenant_id   = tenant_id;
                ev.ttl_seconds = ttl_seconds;
                (void)rep_listener->onReplicationEvent(ev);
            }
            return true;
        }
    }

    return false;
}

size_t AdaptiveQueryCache::invalidate(const std::string &pattern) {
    size_t count = 0;

    // C-3: Limit pattern length to mitigate ReDoS; C++ stdlib regex has no
    // backtracking budget API, so length capping is the primary mitigation.
    constexpr size_t kMaxRegexPatternLen = 256;
    if (static_cast<int>(pattern.size()) > kMaxRegexPatternLen) {
        THEMIS_WARN([[maybe_unused]] "Cache invalidate: pattern too long ({} chars), rejecting to prevent ReDoS", pattern.size());
        return 0;
    }
    std::regex re = {};
    try {
        re = std::regex(pattern, std::regex::ECMAScript | std::regex::optimize);
    } catch (const std::regex_error &e) {
        THEMIS_WARN("Cache invalidate: invalid regex pattern '{}': {}", pattern, e.what());
        return 0;
    }

    // Invalidate L1
    {
        std::scoped_lock<std::shared_mutex, std::mutex> lock(l1_mutex_, l1_eviction_mutex_);
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
        // C1: Timeout-safe initial L3 lock acquisition.
        auto l3_inv_deadline = std::chrono::steady_clock::now() + kL3LockTimeoutMs;
        if (!l3_mutex_.try_lock_until(l3_inv_deadline)) {
            THEMIS_WARN("{{\"event\":\"cache_lock_timeout\",\"operation\":\"l3_invalidate\",\"timeout_ms\":1000}}");
            enhanced_metrics_.l3_read_errors++;
        } else {
        std::unique_lock<std::timed_mutex> lock(l3_mutex_, std::adopt_lock);

        // Phase 1: Check circuit breaker before L3 operation
        if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
            THEMIS_WARN("L3 cache circuit breaker is open, skipping L3 invalidation");
            enhanced_metrics_.l3_circuit_breaker_open = true;
        } else {
            try {
                // Scan to collect keys under the lock (protects l3_db_ pointer).
                std::vector<std::string> keys_to_delete;
                l3_db_->scanPrefix(QUERY_CACHE_PREFIX, [&](std::string_view key, std::string_view) {
                    // Extract fingerprint from key (remove prefix)
                    std::string fingerprint(key.substr(QUERY_CACHE_PREFIX_LEN));
                    if (std::regex_search(fingerprint, re)) {
                        keys_to_delete.emplace_back(key);
                    }
                    return true; // Continue iteration
                });

                // [C-2] Copy l3_db_ into a local shared_ptr while holding the lock so
                // that the pointer remains valid even if a concurrent circuit-breaker
                // trip resets l3_db_ between the unlock and the bulk delete loop.
                auto local_l3_db = l3_db_;

                // Release lock before issuing bulk deletes so readers are not
                // blocked during the I/O-intensive delete phase.
                lock.unlock();

                for (const auto &key : keys_to_delete) {
                    local_l3_db->del(key);
                    count++;
                }

                // C1: Re-acquire with timeout instead of blocking lock().
                {
                    auto relock_deadline = std::chrono::steady_clock::now() + kL3LockTimeoutMs;
                    if (lock.try_lock_until(relock_deadline)) {
                        if (l3_circuit_breaker_) {
                            l3_circuit_breaker_->recordSuccess();
                            enhanced_metrics_.l3_circuit_breaker_open = false;
                        }
                    } else {
                        THEMIS_WARN("{{\"event\":\"cache_lock_timeout\",\"operation\":\"l3_invalidate_relock\",\"timeout_ms\":1000}}");
                        // Circuit breaker not updated; deletes already completed.
                    }
                }

                THEMIS_DEBUG("Invalidated {} L3 cache entries", keys_to_delete.size());
            } catch (const std::exception &e) {
                if (!lock.owns_lock()) {
                    // C1: Re-acquire with timeout in error path.
                    auto err_deadline = std::chrono::steady_clock::now() + kL3LockTimeoutMs;
                    if (!lock.try_lock_until(err_deadline)) {
                        THEMIS_WARN("{{\"event\":\"cache_lock_timeout\",\"operation\":\"l3_invalidate_error_relock\",\"timeout_ms\":1000}}");
                    }
                }
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
        } // end: l3_mutex_ try_lock_until block
    }

    THEMIS_INFO("Invalidated {} cache entries matching pattern: {}", count, pattern);

    // Phase 4: Propagate invalidation to peer nodes via replication coordinator
    if (config_.enable_replication) {
        std::shared_ptr<cache::ICacheCoordinator> coord;
        {
            std::lock_guard<std::mutex> lk(coordinator_mutex_);
            coord = coordinator_;
        }
        if (coord) {
            try {
                coord->publishInvalidation(pattern);
            } catch (const std::exception &e) {
                THEMIS_WARN("Cache replication invalidation publish failed: {}", e.what());
            }
        }
    }

    // Phase 4: Notify replication listener
    if (count > 0) {
        std::shared_ptr<cache::ICacheReplicationListener> rep_listener;
        {
            std::lock_guard<std::mutex> rep_lock(replication_mutex_);
            rep_listener = replication_listener_;
        }
        if ([[maybe_unused]] rep_listener) {
            cache::CacheReplicationEvent ev;
            ev.type    = cache::CacheReplicationEventType::INVALIDATE;
            ev.pattern = pattern;
            (void)rep_listener->onReplicationEvent(ev);
        }
    }

    return count;
}

void AdaptiveQueryCache::clear() {
    {
        std::unique_lock<std::shared_mutex> lock(l1_mutex_);
        l1_cache_.clear();
        {
            std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
            l1_eviction_strategy_->clear();
        }
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
        // Scan under lock to collect keys, then delete outside lock.
        std::vector<std::string> keys;
        std::vector<std::string> pii_ref_keys;
        try {
            {
                std::lock_guard<std::timed_mutex> lock(l3_mutex_);
                static_cast<void>(l3_db_->scanPrefix(QUERY_CACHE_PREFIX, [&keys](std::string_view key, std::string_view) {
                    keys.emplace_back(key);
                    return true;
                }));
                // GDPR: Also collect L3 PII reference index entries
                static_cast<void>(l3_db_->scanPrefix("pii_ref:", [&pii_ref_keys](std::string_view key, std::string_view) {
                    pii_ref_keys.emplace_back(key);
                    return true;
                }));
            }
            // Deletes happen outside l3_mutex_ to avoid blocking readers.
            for (const auto &del_key : keys) {
                static_cast<void>(l3_db_->del(del_key));
            }
            for (const auto &del_key : pii_ref_keys) {
                static_cast<void>(l3_db_->del(del_key));
            }
        } catch (const std::exception &e) {
            THEMIS_WARN("Failed to clear L3 cache: {}", e.what());
        }
    }

    THEMIS_INFO("Cache cleared");
}

uint64_t AdaptiveQueryCache::clearExpired() {
    uint64_t count = 0;
    // Clear expired L1 entries
    {
        std::scoped_lock<std::shared_mutex, std::mutex> lock(l1_mutex_, l1_eviction_mutex_);
        for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
            if (isExpired(it->second->created_at_ms.load(std::memory_order_relaxed),
                          it->second->ttl_seconds.load(std::memory_order_relaxed))) {
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

    info["stats"]
        = {{"l1_hits", stats_.l1_hits},     {"l2_hits", stats_.l2_hits},       {"l3_hits", stats_.l3_hits},
           {"misses", stats_.misses},       {"hit_rate", stats_.getHitRate()}, {"l1_hit_rate", stats_.getL1HitRate()},
           {"evictions", stats_.evictions}, {"promotions", stats_.promotions}, {"demotions", stats_.demotions}};

    {
        std::shared_lock<std::shared_mutex> lock(l1_mutex_);
        std::string eviction_name = {};
        {
            std::lock_guard<std::mutex> evl(l1_eviction_mutex_);
            eviction_name = std::string(l1_eviction_strategy_->getName());
        }
        info["l1"] = {{"entries", l1_cache_.size()},
                      {"max_entries", config_.l1_max_entries},
                      {"utilization", static_cast<double>(l1_cache_.size()) / config_.l1_max_entries},
                      {"eviction_policy", eviction_name}};
    }

    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        info["l2"] = {{"entries", l2_cache_.size()},
                      {"max_entries", config_.l2_max_entries},
                      {"utilization", static_cast<double>(l2_cache_.size()) / config_.l2_max_entries},
                      {"eviction_policy", std::string(l2_eviction_strategy_->getName())}};
    }

    info["l3"] = {{"enabled", l3_db_ != nullptr}, {"path", config_.l3_db_path}};

    // Phase 3: Adaptive TTL tuning metrics
    if (config_.enable_adaptive_ttl) {
        info["adaptive_ttl"] = {{"enabled", true},
                                {"min_seconds", config_.adaptive_ttl_min_seconds},
                                {"max_seconds", config_.adaptive_ttl_max_seconds},
                                {"scaling_factor", config_.adaptive_ttl_scaling_factor},
                                {"ttl_extended_total", enhanced_metrics_.ttl_extended_total.load()},
                                {"ttl_shortened_total", enhanced_metrics_.ttl_shortened_total.load()}};
    } else {
        info["adaptive_ttl"] = {{"enabled", false}};
    }

    // Phase 4: Write-through mode info
    info["write_through"] = {{"enabled", config_.enable_write_through},
                             {"total", enhanced_metrics_.write_through_total.load()},
                             {"errors", enhanced_metrics_.write_through_errors.load()},
                             {"writes", enhanced_metrics_.write_through_writes.load()}};

    return info;
}

// Private helper methods

int64_t AdaptiveQueryCache::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool AdaptiveQueryCache::isExpired(int64_t created_at_ms, int ttl_seconds) const {
    int64_t now_ms    = getCurrentTimeMs();
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

    int base_ttl      = config_.adaptive_ttl_min_seconds;
    double log_factor = std::log(static_cast<double>(access_count + 1)) / config_.adaptive_ttl_scaling_factor;
    int adaptive_ttl  = static_cast<int>(base_ttl * (1.0 + log_factor));

    // Clamp to configured bounds
    adaptive_ttl = std::max(adaptive_ttl, config_.adaptive_ttl_min_seconds);
    adaptive_ttl = std::min(adaptive_ttl, config_.adaptive_ttl_max_seconds);

    return adaptive_ttl;
}

AdaptiveQueryCache::CacheLevel AdaptiveQueryCache::selectCacheLevel([[maybe_unused]] size_t result_size) const {
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
        // First: purge any entries already marked expired
        for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
            if (it->second->expired_flag.load(std::memory_order_relaxed)) {
                {
                    std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                    l1_eviction_strategy_->onRemove(it->first);
                }
                // Emit eviction event before erasing
                auto last_access_ms = it->second->last_accessed_ms.load(std::memory_order_relaxed);
                auto access_count = it->second->access_count.load(std::memory_order_relaxed);
                auto result_size = it->second->result.dump().size();
                emitEvictionEvent(it->first, access_model::TierLevel::L1_WORKING,
                                result_size, access_count, last_access_ms, "expired");
                
                it = l1_cache_.erase(it);
                stats_.evictions++;
                enhanced_metrics_.evictions++;
            } else {
                ++it;
            }
        }
        if (l1_cache_.empty()) {
            return;
        }

        // Use the configured eviction strategy to select the victim key
        std::optional<std::string> victim;
        {
            std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
            victim = l1_eviction_strategy_->selectVictim();
        }
        if (victim && l1_cache_.count(*victim)) {
            auto entry = l1_cache_.at(*victim).get();
            auto last_access_ms = entry->last_accessed_ms.load(std::memory_order_relaxed);
            auto access_count = entry->access_count.load(std::memory_order_relaxed);
            auto result_size = entry->result.dump().size();
            
            {
                std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                l1_eviction_strategy_->onRemove(*victim);
            }
            
            // Emit eviction event before erasing
            emitEvictionEvent(*victim, access_model::TierLevel::L1_WORKING,
                            result_size, access_count, last_access_ms, "lru_selection");
            
            l1_cache_.erase(*victim);
        } else {
            // Fallback: score-based scan using per-entry atomic counters
            auto lru_it      = l1_cache_.begin();
            double min_score = calculateLRUScore(lru_it->second->last_accessed_ms.load(std::memory_order_relaxed),
                                                 lru_it->second->access_count.load(std::memory_order_relaxed));
            for (auto it = l1_cache_.begin(); it != l1_cache_.end(); ++it) {
                double score = calculateLRUScore(it->second->last_accessed_ms.load(std::memory_order_relaxed),
                                                 it->second->access_count.load(std::memory_order_relaxed));
                if (score < min_score) {
                    min_score = score;
                    lru_it    = it;
                }
            }
            
            auto entry = lru_it->second.get();
            auto last_access_ms = entry->last_accessed_ms.load(std::memory_order_relaxed);
            auto access_count = entry->access_count.load(std::memory_order_relaxed);
            auto result_size = entry->result.dump().size();
            
            {
                std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                l1_eviction_strategy_->onRemove(lru_it->first);
            }
            
            // Emit eviction event before erasing
            emitEvictionEvent(lru_it->first, access_model::TierLevel::L1_WORKING,
                            result_size, access_count, last_access_ms, "lru_fallback");
            
            l1_cache_.erase(lru_it);
        }
        stats_.evictions++;
        enhanced_metrics_.evictions++;

    } else if (level == CacheLevel::WARM) {
        if (l2_cache_.empty()) {
            return;
        }

        // Use the configured eviction strategy to select the victim key
        auto victim = l2_eviction_strategy_->selectVictim();
        if (victim && l2_cache_.count(*victim)) {
            auto& entry = l2_cache_.at(*victim);
            auto last_access_ms = entry.last_accessed_ms;
            auto access_count = entry.access_count;
            auto size_bytes = entry.compressed_result.size();
            
            l2_eviction_strategy_->onRemove(*victim);
            
            // Emit eviction event before erasing
            emitEvictionEvent(*victim, access_model::TierLevel::L2_EPISODIC,
                            size_bytes, access_count, last_access_ms, "lru_selection");
            
            l2_cache_.erase(*victim);
        } else {
            // Fallback: score-based scan
            auto lru_it      = l2_cache_.begin();
            double min_score = calculateLRUScore(lru_it->second.last_accessed_ms, lru_it->second.access_count);
            for (auto it = l2_cache_.begin(); it != l2_cache_.end(); ++it) {
                double score = calculateLRUScore(it->second.last_accessed_ms, it->second.access_count);
                if (score < min_score) {
                    min_score = score;
                    lru_it    = it;
                }
            }
            
            auto& entry = lru_it->second;
            auto last_access_ms = entry.last_accessed_ms;
            auto access_count = entry.access_count;
            auto size_bytes = entry.compressed_result.size();
            
            l2_eviction_strategy_->onRemove(lru_it->first);
            
            // Emit eviction event before erasing
            emitEvictionEvent(lru_it->first, access_model::TierLevel::L2_EPISODIC,
                            size_bytes, access_count, last_access_ms, "lru_fallback");
            
            l2_cache_.erase(lru_it);
        }
        stats_.evictions++;
        enhanced_metrics_.evictions++;
    }
}

// ============================================================================
// Phase 5: BLOCK 2 Cache Integration — Eviction Event Emission
// ============================================================================

void AdaptiveQueryCache::emitEvictionEvent(const std::string& key, access_model::TierLevel tier,
                                          std::size_t size_bytes, uint64_t access_count,
                                          int64_t last_access_ms, std::string_view reason) {
    // Snapshot the listener pointer under the lock to avoid holding it during the callback.
    // Calling the callback while holding the lock can deadlock if the listener (re-)enters
    // cache or coordinator code that tries to acquire the same or a dependent lock.
    access_model::EvictionListener* listener = nullptr;
    {
        std::lock_guard<std::mutex> lock([[maybe_unused]] eviction_listener_mutex_);
        listener = eviction_listener_;
    }
    if ([[maybe_unused]] !listener) {
        return;  // No listener registered
    }

    // Calculate time since last access
    int64_t now_ms = getCurrentTimeMs();
    int64_t age_ms = (now_ms > last_access_ms) ? (now_ms - last_access_ms) : 0;
    auto age_secs = std::chrono::seconds(age_ms / 1000);

    // Emit event to coordinator (invoked outside the lock)
    listener->onCacheEvicted(key, tier, size_bytes, access_count, age_secs, reason);
}

double AdaptiveQueryCache::calculateLRUScore(int64_t last_accessed_ms, int64_t access_count) const {
    int64_t now_ms = getCurrentTimeMs();
    int64_t age_ms = now_ms - last_accessed_ms;

    // Score = frequency_weight * access_count - (1 - frequency_weight) * age
    // Higher score = more valuable entry
    double score = config_.frequency_weight * access_count - (1.0 - config_.frequency_weight) * (age_ms / 1000.0);

    return score;
}

// Phase 1: Size validation helpers
bool AdaptiveQueryCache::isWithinSizeLimit([[maybe_unused]] size_t size) const {
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
bool AdaptiveQueryCache::Config::validate(std::string *error_msg) const {
    auto set_error = [error_msg](const std::string &msg) {
        if (error_msg) {
            *error_msg = msg;
        }
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
    // Validate legacy TTL aliases only when they are explicitly overridden.
    const bool legacy_min_overridden = (min_ttl_seconds != 60);
    const bool legacy_max_overridden = (max_ttl_seconds != 86400);
    if (legacy_min_overridden || legacy_max_overridden) {
        if (min_ttl_seconds <= 0) {
            return set_error("min_ttl_seconds must be greater than 0");
        }
        if (max_ttl_seconds <= 0) {
            return set_error("max_ttl_seconds must be greater than 0");
        }
        if (max_ttl_seconds < min_ttl_seconds) {
            return set_error("max_ttl_seconds must be >= min_ttl_seconds");
        }
    }

    // Resolve effective TTL bounds for runtime paths.
    const int effective_min_ttl = legacy_min_overridden ? min_ttl_seconds : adaptive_ttl_min_seconds;
    const int effective_max_ttl = legacy_max_overridden ? max_ttl_seconds : adaptive_ttl_max_seconds;

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
            return set_error("min_seconds must be less than max_seconds");
        }
        if (adaptive_ttl_scaling_factor <= 0.0) {
            return set_error("adaptive_ttl_scaling_factor must be greater than 0");
        }
    }

    return true;
}

// Phase 2: Tenant isolation helper methods
std::string AdaptiveQueryCache::makeTenantKey(const std::string &fingerprint, const std::string &tenant_id) const {
    if (tenant_id.empty()) {
        return fingerprint;
    }
    return "tenant:" + tenant_id + ":" + fingerprint;
}

bool AdaptiveQueryCache::checkTenantQuota(const std::string &tenant_id, size_t additional_bytes) {
    if (!config_.enable_tenant_isolation || tenant_id.empty()) {
        return true; // No quotas if isolation disabled
    }

    std::lock_guard<std::mutex> lock(tenant_mutex_);
    size_t current_size    = tenant_metrics_[tenant_id].bytes_used;
    size_t effective_quota = getEffectiveTenantQuota(tenant_id);

    if (effective_quota == 0) {
        return true; // 0 means unlimited
    }

    if (current_size + additional_bytes > effective_quota) {
        THEMIS_WARN("Tenant {} quota exceeded: current={}, additional={}, limit={}", tenant_id, current_size,
                    additional_bytes, effective_quota);
        return false;
    }

    return true;
}

// ============================================================================
// Phase 4: Write-Through Cache Mode
// ============================================================================

bool AdaptiveQueryCache::writeThroughToL3(const std::string &fingerprint, const nlohmann::json &query_params,
                                          const nlohmann::json &result, int64_t now_ms, int ttl_seconds) {
    if (!l3_db_) {
        return false;
    }

    // Check circuit breaker before L3 operation
    if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
        THEMIS_WARN("L3 circuit breaker is open, skipping write-through for fingerprint={}", fingerprint.substr(0, 16));
        enhanced_metrics_.write_through_errors++;
        return false;
    }

    nlohmann::json entry_json;
    entry_json["result"]           = result;
    entry_json["query_params"]     = query_params;
    entry_json["created_at_ms"]    = now_ms;
    entry_json["last_accessed_ms"] = now_ms;
    entry_json["access_count"]     = 1;
    entry_json["ttl_seconds"]      = ttl_seconds;
    entry_json["window_start_ms"]  = now_ms;
    entry_json["window_count"]     = 0;

    std::string l3_key = QUERY_CACHE_PREFIX + fingerprint;

    try {
        std::lock_guard<std::timed_mutex> lock(l3_mutex_);
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
    } catch (const std::exception &e) {
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
        std::shared_lock<std::shared_mutex> lock(l1_mutex_);
        stats["l1"]["entries"]     = l1_cache_.size();
        stats["l1"]["max_entries"] = config_.l1_max_entries;
        stats["l1"]["utilization"] = static_cast<double>(l1_cache_.size()) / config_.l1_max_entries;
        stats["l1"]["hits"]        = enhanced_metrics_.l1_hits.load();
        {
            std::lock_guard<std::mutex> evl(l1_eviction_mutex_);
            stats["l1"]["eviction_policy"] = std::string(l1_eviction_strategy_->getName());
        }
    }

    // L2 statistics
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        stats["l2"]["entries"]         = l2_cache_.size();
        stats["l2"]["max_entries"]     = config_.l2_max_entries;
        stats["l2"]["utilization"]     = static_cast<double>(l2_cache_.size()) / config_.l2_max_entries;
        stats["l2"]["hits"]            = enhanced_metrics_.l2_hits.load();
        stats["l2"]["eviction_policy"] = std::string(l2_eviction_strategy_->getName());
    }

    // L3 statistics
    stats["l3"]["enabled"] = (l3_db_ != nullptr);
    stats["l3"]["hits"]    = enhanced_metrics_.l3_hits.load();
    if (l3_circuit_breaker_) {
        stats["l3"]["circuit_breaker_open"] = enhanced_metrics_.l3_circuit_breaker_open.load();
    }

    // Overall
    stats["overall"]["misses"]    = enhanced_metrics_.misses.load();
    stats["overall"]["hit_rate"]  = enhanced_metrics_.getHitRate();
    stats["overall"]["evictions"] = enhanced_metrics_.evictions.load();

    // Phase 4: Write-through mode status
    stats["write_through"]["enabled"] = config_.enable_write_through;
    stats["write_through"]["writes"]  = enhanced_metrics_.write_through_writes.load();

    return stats;
}

nlohmann::json AdaptiveQueryCache::getHealthStatus() const {
    nlohmann::json health;
    health["healthy"]  = true;
    health["warnings"] = nlohmann::json::array();

    // Per-tier status
    nlohmann::json tiers;

    // L1 tier
    {
        std::shared_lock<std::shared_mutex> lock(l1_mutex_);
        size_t entries          = l1_cache_.size();
        double util             = static_cast<double>(entries) / config_.l1_max_entries;
        std::string tier_status = (util > 0.9) ? "DEGRADED" : "OK";
        tiers["l1"]             = {{"status", tier_status},
                                   {"entries", entries},
                                   {"max_entries", config_.l1_max_entries},
                                   {"utilization", util},
                                   {"ttl_seconds", config_.l1_ttl_seconds}};
        if (util > 0.9) {
            health["warnings"].push_back("L1 cache utilization high: " + std::to_string(util * 100) + "%");
        }
    }

    // L2 tier
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        size_t entries          = l2_cache_.size();
        double util             = static_cast<double>(entries) / config_.l2_max_entries;
        std::string tier_status = (util > 0.9) ? "DEGRADED" : "OK";
        tiers["l2"]             = {{"status", tier_status},
                                   {"entries", entries},
                                   {"max_entries", config_.l2_max_entries},
                                   {"utilization", util},
                                   {"ttl_seconds", config_.l2_ttl_seconds}};
        if (util > 0.9) {
            health["warnings"].push_back("L2 cache utilization high: " + std::to_string(util * 100) + "%");
        }
    }

    // L3 tier
    {
        bool l3_open            = enhanced_metrics_.l3_circuit_breaker_open.load();
        bool l3_enabled         = (l3_db_ != nullptr);
        std::string tier_status = l3_open ? "UNAVAILABLE" : (l3_enabled ? "OK" : "DISABLED");
        tiers["l3"]             = {{"status", tier_status},
                                   {"enabled", l3_enabled},
                                   {"path", config_.l3_db_path},
                                   {"ttl_seconds", config_.l3_ttl_seconds}};
        if (l3_open) {
            health["healthy"] = false;
            health["warnings"].push_back("L3 circuit breaker is OPEN - RocksDB unavailable");
        }
    }

    health["tiers"] = tiers;

    // Embed circuit breaker details
    health["circuit_breaker"] = getCircuitBreakerStatus();

    // Cache coordinator connection status (observable via health endpoint)
    {
        std::lock_guard<std::mutex> lk(coordinator_mutex_);
        if (coordinator_) {
            bool coord_connected  = coordinator_->isConnected();
            health["coordinator"] = {{"enabled", true}, {"connected", coord_connected}, {"name", coordinator_->name()}};
            if (!coord_connected) {
                health["warnings"].push_back("Cache coordinator disconnected: " + coordinator_->name());
            }
        } else {
            health["coordinator"] = {{"enabled", false}};
        }
    }

    // Phase 4: Write-through mode status
    health["write_through"]
        = {{"enabled", config_.enable_write_through}, {"writes", enhanced_metrics_.write_through_writes.load()}};

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

std::vector<std::string> AdaptiveQueryCache::exportKeys([[maybe_unused]] size_t max_keys) const {
    std::vector<std::string> keys;
    keys.reserve(max_keys);

    // Export L1 keys
    {
        std::shared_lock<std::shared_mutex> lock(l1_mutex_);
        for (const auto &[key, entry] : l1_cache_) {
            if (static_cast<int>(keys.size()) > = max_keys) {
                break;
            }
            keys.push_back("L1:" + key.substr(0, 16) + "...");
        }
    }

    // Export L2 keys
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        for (const auto &[key, entry] : l2_cache_) {
            if (static_cast<int>(keys.size()) > = max_keys) {
                break;
            }
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

    tenant_stats["enabled"]          = true;
    tenant_stats["quota_per_tenant"] = config_.per_tenant_max_bytes;

    std::lock_guard<std::mutex> lock(tenant_mutex_);
    for (const auto &[tenant_id, metrics] : tenant_metrics_) {
        nlohmann::json tenant_info;
        uint64_t total            = metrics.hits + metrics.misses;
        size_t effective_quota    = getEffectiveTenantQuota(tenant_id);
        tenant_info["bytes_used"] = metrics.bytes_used;
        tenant_info["quota"]      = effective_quota;
        tenant_info["utilization"]
            = effective_quota > 0 ? static_cast<double>(metrics.bytes_used) / effective_quota : 0.0;
        tenant_info["hits"]                = metrics.hits;
        tenant_info["misses"]              = metrics.misses;
        tenant_info["evictions"]           = metrics.evictions;
        tenant_info["hit_rate"]            = total > 0 ? static_cast<double>(metrics.hits) / total : 0.0;
        tenant_stats["tenants"][tenant_id] = tenant_info;
    }

    return tenant_stats;
}

nlohmann::json AdaptiveQueryCache::getTenantStatsForTenant(const std::string &tenant_id) const {
    if (!config_.enable_tenant_isolation || tenant_id.empty()) {
        nlohmann::json result;
        result["found"]  = false;
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

    const auto &m          = it->second;
    uint64_t total         = m.hits + m.misses;
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
    const std::vector<std::tuple<std::string, nlohmann::json, nlohmann::json, std::string>> &entries) {
    size_t successful = 0;

    for (const auto &[fingerprint, params, result, tenant_id] : entries) {
        if (put(fingerprint, params, result, tenant_id)) {
            successful++;
        }
    }

    THEMIS_INFO("Bulk put completed: {}/{} entries cached", successful, entries.size());
    return successful;
}

size_t AdaptiveQueryCache::invalidateTenant(const std::string &tenant_id) {
    if (tenant_id.empty()) {
        THEMIS_WARN("Invalid tenant_id for invalidation");
        return 0;
    }

    size_t count              = 0;
    std::string tenant_prefix = "tenant:" + tenant_id + ":";

    // Invalidate L1
    {
        std::unique_lock<std::shared_mutex> lock(l1_mutex_);
        for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
            if (it->first.find(tenant_prefix) == 0) {
                {
                    std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                    l1_eviction_strategy_->onRemove(it->first);
                }
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
        std::vector<std::string> keys_to_delete;
        bool cb_allowed = false;
        {
            std::unique_lock<std::timed_mutex> lock(l3_mutex_);
            cb_allowed = !l3_circuit_breaker_ || l3_circuit_breaker_->allowRequest();
            if (!cb_allowed) {
                THEMIS_WARN("L3 circuit breaker open, skipping L3 tenant invalidation");
            } else {
                try {
                    // Collect keys under lock; deletes happen outside.
                    l3_db_->scanPrefix(QUERY_CACHE_PREFIX, [&](std::string_view key, std::string_view) {
                        std::string key_str(key);
                        if (key_str.find(tenant_prefix) != std::string::npos) {
                            keys_to_delete.emplace_back(key_str);
                        }
                        return true;
                    });
                } catch (const std::exception &e) {
                    THEMIS_WARN("Failed to scan tenant keys in L3: {}", e.what());
                    if (l3_circuit_breaker_) {
                        l3_circuit_breaker_->recordFailure();
                    }
                    cb_allowed = false;
                }
            }
        }
        if (cb_allowed && !keys_to_delete.empty()) {
            try {
                for (const auto &key : keys_to_delete) {
                    l3_db_->del(key);
                    count++;
                }
                std::lock_guard<std::timed_mutex> lock(l3_mutex_);
                if (l3_circuit_breaker_) {
                    l3_circuit_breaker_->recordSuccess();
                }
            } catch (const std::exception &e) {
                THEMIS_WARN("Failed to invalidate tenant in L3: {}", e.what());
                std::lock_guard<std::timed_mutex> lock(l3_mutex_);
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
        {
            std::lock_guard<std::mutex> lk(coordinator_mutex_);
            coord = coordinator_;
        }
        if (coord) {
            try {
                std::string tenant_pattern = "^tenant:" + tenant_id + ":";
                coord->publishInvalidation(tenant_pattern, tenant_id);
            } catch (const std::exception &e) {
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
        if ([[maybe_unused]] rep_listener) {
            cache::CacheReplicationEvent ev;
            ev.type      = cache::CacheReplicationEventType::INVALIDATE_TENANT;
            ev.tenant_id = tenant_id;
            (void)rep_listener->onReplicationEvent(ev);
        }
    }

    return count;
}

size_t AdaptiveQueryCache::invalidatePII(const std::string &pii_uuid) {
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
        } else {
            THEMIS_DEBUG("invalidatePII: No keys found in index for uuid={}", pii_uuid);
        }
    }

    if (!keys_to_purge.empty()) {
        {
            std::unique_lock<std::shared_mutex> lock(l1_mutex_);
            for (const auto &k : keys_to_purge) {
                auto it = l1_cache_.find(k);
                if (it != l1_cache_.end()) {
                    {
                        std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                        l1_eviction_strategy_->onRemove(it->first);
                    }
                    l1_cache_.erase(it);
                    count++;
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(l2_mutex_);
            for (const auto &k : keys_to_purge) {
                // L2 is indexed by fingerprint, not tenanted key
                // Extract fingerprint from k if it's a tenanted key, otherwise use k as-is
                std::string l2_key       = k;
                const std::string prefix = "tenant:";
                if (k.compare(0, prefix.length(), prefix) == 0) {
                    // Extract fingerprint from tenanted key format: tenant:xxx:fingerprint
                    size_t last_colon = k.rfind(':');
                    if (last_colon != std::string::npos) {
                        l2_key = k.substr(last_colon + 1);
                    }
                }
                auto it = l2_cache_.find(l2_key);
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
        std::vector<std::string> pii_ref_keys;
        std::vector<std::string> cache_keys;
        bool cb_allowed = false;

        {
            std::unique_lock<std::timed_mutex> lock(l3_mutex_);
            if (l3_circuit_breaker_ && !l3_circuit_breaker_->allowRequest()) {
                THEMIS_WARN("L3 circuit breaker open, skipping L3 PII invalidation for uuid={}", pii_uuid);
                enhanced_metrics_.l3_circuit_breaker_open = true;
            } else {
                cb_allowed = true;
                try {
                    const std::string pii_ref_prefix = "pii_ref:" + pii_uuid + ":";
                    // Scan under lock to snapshot keys; deletes happen outside.
                    l3_db_->scanPrefix(pii_ref_prefix, [&](std::string_view key, std::string_view) {
                        pii_ref_keys.emplace_back(key);
                        std::string k(key);
                        if (static_cast<int>(k.size()) > pii_ref_prefix.size()) {
                            cache_keys.emplace_back(QUERY_CACHE_PREFIX + k.substr(pii_ref_prefix.size()));
                        }
                        return true;
                    });
                } catch (const std::exception &e) {
                    THEMIS_WARN("Failed L3 PII scan for uuid={}: {}", pii_uuid, e.what());
                    enhanced_metrics_.l3_read_errors++;
                    if (l3_circuit_breaker_) {
                        l3_circuit_breaker_->recordFailure();
                        if (l3_circuit_breaker_->isOpen()) {
                            enhanced_metrics_.l3_circuit_breaker_trips++;
                            enhanced_metrics_.l3_circuit_breaker_open = true;
                        }
                    }
                    cb_allowed = false;
                }
            }
        }

        if (cb_allowed) {
            try {
                for (const auto &ck : cache_keys) {
                    if (l3_db_->del(ck)) {
                        count++;
                    }
                }
                for (const auto &rk : pii_ref_keys) {
                    static_cast<void>(l3_db_->del(rk));
                }
                std::lock_guard<std::timed_mutex> lock(l3_mutex_);
                if (l3_circuit_breaker_) {
                    l3_circuit_breaker_->recordSuccess();
                    enhanced_metrics_.l3_circuit_breaker_open = false;
                }
            } catch (const std::exception &e) {
                THEMIS_WARN("Failed L3 PII invalidation for uuid={}: {}", pii_uuid, e.what());
                std::lock_guard<std::timed_mutex> lock(l3_mutex_);
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

bool AdaptiveQueryCache::updateTenantQuota(const std::string &tenant_id, size_t quota_bytes) {
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
    tenant_metrics_[tenant_id]; // default-insert if absent
    THEMIS_INFO("Updated quota for tenant {}: {} bytes", tenant_id, quota_bytes);
    return true;
}

size_t AdaptiveQueryCache::getEffectiveTenantQuota(const std::string &tenant_id) const {
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
        status["enabled"]       = false;
        status["state"]         = "CLOSED";
        status["failure_count"] = 0;
        return status;
    }

    status["enabled"]       = true;
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
// Phase 4: Predictive Pre-Fetching
// ---------------------------------------------------------------------------

void AdaptiveQueryCache::recordQueryAccess(const std::string &fingerprint, const std::string &tenant_id) {
    if (prefetcher_) {
        prefetcher_->recordQueryAccess(fingerprint, tenant_id);
    }
}

std::vector<std::string> AdaptiveQueryCache::getPrefetchCandidates(const std::string &fingerprint,
                                                                   const std::string &tenant_id) const {
    if (!prefetcher_) {
        return {};
    }

    auto candidates = prefetcher_->getPrefetchCandidates(fingerprint, tenant_id);
    if (!candidates.empty()) {
        // enhanced_metrics_ is exposed via getEnhancedMetrics() (CacheMetrics format);
        // the prefetcher's internal counter is returned by getPrefetchStats().
        // Both are kept in sync here so each API surface is self-consistent.
        enhanced_metrics_.prefetch_candidates_generated++;
        prefetcher_->recordCandidatesGenerated(candidates.size(), tenant_id);
    }
    return candidates;
}

nlohmann::json AdaptiveQueryCache::getPrefetchStats() const {
    if (!prefetcher_) {
        return {{"enabled", false}};
    }
    nlohmann::json j = prefetcher_->getStats();
    j["enabled"]     = true;
    // Enrich with the hit counter maintained in enhanced_metrics_
    j["prefetch_hits_from_metrics"] = enhanced_metrics_.prefetch_hits.load();
    return j;
}

void AdaptiveQueryCache::recordPrefetchOverheadBytes([[maybe_unused]] uint64_t bytes) {
    if (prefetcher_) {
        prefetcher_->recordOverheadBytes(bytes);
    }
}

void AdaptiveQueryCache::savePrefetchModel() {
    if (prefetcher_ && l3_db_) {
        prefetcher_->saveModel(l3_db_.get());
    }
}

void AdaptiveQueryCache::loadPrefetchModel() {
    if (prefetcher_ && l3_db_) {
        prefetcher_->loadModel(l3_db_.get());
    }
}

// ============================================================================
// Phase 4: Cache Replication for High-Availability Multi-Node Deployments
// ============================================================================

void AdaptiveQueryCache::setCoordinator(std::shared_ptr<cache::ICacheCoordinator> coordinator) {
    std::lock_guard<std::mutex> lk(coordinator_mutex_);
    coordinator_ = coordinator;

    if (!coordinator_) {
        THEMIS_INFO("AdaptiveQueryCache: replication coordinator removed");
        return;
    }

    // Subscribe for entries replicated from peer nodes.
    // [C-4] Capture alive_guard_ by value (shared_ptr copy) so the guard struct
    // outlives the callback. The lock+flag check prevents use-after-free when the
    // AdaptiveQueryCache destructor has started but this callback fires concurrently.
    auto guard = alive_guard_;
    coordinator_->subscribeEntries([this, guard](const cache::ReplicationMessage &msg) {
        std::lock_guard<std::mutex> alive_lock(guard->mutex);
        if (!guard->alive) {
            return;
        }
        applyReplicatedEntry(msg);
    });

    // Subscribe for invalidations propagated from peer nodes
    coordinator_->subscribeInvalidations([this, guard](const cache::ReplicationMessage &msg) {
        std::lock_guard<std::mutex> alive_lock(guard->mutex);
        if (!guard->alive) {
            return;
        }
        applyReplicatedInvalidation(msg);
    });

    THEMIS_INFO("AdaptiveQueryCache: replication coordinator registered ({})", coordinator_->name());
}

nlohmann::json AdaptiveQueryCache::getReplicationStats() const {
    std::lock_guard<std::mutex> lk(coordinator_mutex_);
    if (!coordinator_) {
        return {{"enabled", false}};
    }
    auto stats       = coordinator_->getStats();
    stats["enabled"] = true;
    return stats;
}

void AdaptiveQueryCache::applyReplicatedEntry(const cache::ReplicationMessage &msg) {
    // Replicate only L1/L2; L3 (RocksDB) is assumed shared or node-local
    // and does not need replication from the coordinator bus.
    if (msg.result.is_null() || !msg.result.is_object()) {
        return;
    }

    int64_t now_ms         = getCurrentTimeMs();
    std::string result_str = msg.result.dump();
    size_t result_size     = result_str.size();

    // Honour per-entry size limit and tenant quota checks
    if (config_.enable_size_limits && !isWithinSizeLimit(result_size)) {
        return;
    }
    if (!checkTenantQuota(msg.tenant_id, result_size)) {
        return;
    }

    const std::string &key = msg.key;
    int ttl_seconds        = msg.ttl_seconds > 0 ? msg.ttl_seconds : config_.l1_ttl_seconds;

    if (result_size < config_.l1_max_entry_size) {
        std::unique_lock<std::shared_mutex> lock(l1_mutex_);
        if (l1_cache_.count(key) == 0) { // Don't overwrite a locally fresher entry
            if (static_cast<int>(l1_cache_.size()) > = config_.l1_max_entries) {
                evictLRU(CacheLevel::HOT);
            }
            auto entry    = std::make_unique<L1Entry>();
            entry->result = msg.result;
            entry->created_at_ms.store(now_ms, std::memory_order_relaxed);
            entry->last_accessed_ms.store(now_ms, std::memory_order_relaxed);
            entry->access_count.store(0, std::memory_order_relaxed);
            entry->ttl_seconds.store(ttl_seconds, std::memory_order_relaxed);
            entry->window_start_ms.store(now_ms, std::memory_order_relaxed);
            entry->window_count.store(0, std::memory_order_relaxed);
            l1_cache_[key] = std::move(entry);
            {
                std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                l1_eviction_strategy_->onInsert(key, static_cast<uint64_t>(now_ms));
            }
            enhanced_metrics_.total_bytes_cached += result_size;
        }
    } else if (result_size < config_.l2_max_entry_size) {
        auto compressed = utils::zstd_compress(result_str, config_.l2_compression_level);
        if (compressed.empty()) {
            return;
        }

        std::lock_guard<std::mutex> lock(l2_mutex_);
        if (l2_cache_.count(key) == 0) {
            if (static_cast<int>(l2_cache_.size()) > = config_.l2_max_entries) {
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

void AdaptiveQueryCache::applyReplicatedInvalidation(const cache::ReplicationMessage &msg) {
    // Peer invalidated a key/pattern – evict matching entries from L1 and L2 only.
    // L3 (RocksDB) is considered either shared or independently managed per-node.
    const std::string &pattern = msg.key;
    if (pattern.empty()) {
        return;
    }

    try {
        std::regex re(pattern);

        {
            std::unique_lock<std::shared_mutex> lock(l1_mutex_);
            for (auto it = l1_cache_.begin(); it != l1_cache_.end();) {
                if (std::regex_search(it->first, re)) {
                    {
                        std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                        l1_eviction_strategy_->onRemove(it->first);
                    }
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
    } catch (const std::regex_error &e) {
        THEMIS_WARN("CacheReplication: invalid pattern received from peer: {} ({})", pattern, e.what());
    }
}

// ============================================================================
// Phase 4: Cache Replication for High-Availability
// ============================================================================

void AdaptiveQueryCache::setReplicationListener([[maybe_unused]] std::shared_ptr<cache::ICacheReplicationListener> listener) {
    std::lock_guard<std::mutex> lock(replication_mutex_);
    replication_listener_ = std::move([[maybe_unused]] listener);
    if ([[maybe_unused]] replication_listener_) {
        THEMIS_INFO([[maybe_unused]] "AdaptiveQueryCache: replication listener registered ({})", replication_listener_->replicaId());
    } else {
        THEMIS_INFO([[maybe_unused]] "AdaptiveQueryCache: replication listener unregistered");
    }
}

// ============================================================================
// Phase 5: BLOCK 2 Cache Integration — AccessCoordinator Listener
// ============================================================================

void AdaptiveQueryCache::setEvictionListener([[maybe_unused]] access_model::EvictionListener* listener) noexcept {
    std::lock_guard<std::mutex> lock([[maybe_unused]] eviction_listener_mutex_);
    eviction_listener_ = listener;
    if ([[maybe_unused]] eviction_listener_) {
        THEMIS_INFO([[maybe_unused]] "AdaptiveQueryCache: eviction listener registered for AccessCoordinator");
    } else {
        THEMIS_INFO([[maybe_unused]] "AdaptiveQueryCache: eviction listener unregistered");
    }
}

} // namespace themis

// ============================================================================
// ICacheBackend<std::string, nlohmann::json> adapter implementations
// ============================================================================

namespace themis {

std::optional<nlohmann::json> AdaptiveQueryCache::get(const std::string &fingerprint) {
    auto entry = get(fingerprint, "");
    if (!entry.has_value()) {
        return std::nullopt;
    }
    return entry->result;
}

void AdaptiveQueryCache::put(const std::string &fingerprint, nlohmann::json result, uint32_t /*ttl_seconds*/) {
    // Note: ttl_seconds is not honoured here — AdaptiveQueryCache applies its
    // own tier-based TTL policy (l1/l2/l3_ttl_seconds from Config).
    // Use the rich put() overload directly when per-call TTL control is needed.
    put(fingerprint, nlohmann::json{}, std::move(result), "");
}

bool AdaptiveQueryCache::remove(const std::string &fingerprint) {
    bool found = false;

    // L1 tier
    {
        std::unique_lock<std::shared_mutex> lock(l1_mutex_);
        auto it = l1_cache_.find(fingerprint);
        if (it != l1_cache_.end()) {
            {
                std::lock_guard<std::mutex> evict_lock(l1_eviction_mutex_);
                l1_eviction_strategy_->onRemove(fingerprint);
            }
            l1_cache_.erase(it);
            found = true;
        }
    }

    // L2 tier
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        auto it = l2_cache_.find(fingerprint);
        if (it != l2_cache_.end()) {
            l2_eviction_strategy_->onRemove(fingerprint);
            l2_cache_.erase(it);
            found = true;
        }
    }

    // L3 tier
    if (l3_db_) {
        const std::string l3_key = QUERY_CACHE_PREFIX + fingerprint;
        bool l3_found            = false;
        try {
            {
                std::lock_guard<std::timed_mutex> lock(l3_mutex_);
                l3_found = l3_db_->get(l3_key).has_value();
            }
            if (l3_found) {
                l3_db_->del(l3_key);
                found = true;
            }
        } catch (const std::exception &e) {
            THEMIS_WARN("AdaptiveQueryCache::remove: L3 operation failed: {}", e.what());
        }
    }

    return found;
}

bool AdaptiveQueryCache::contains(const std::string &fingerprint) const {
    // L1 — shared lock, no LRU update
    {
        std::shared_lock<std::shared_mutex> lock(l1_mutex_);
        auto it = l1_cache_.find(fingerprint);
        if (it != l1_cache_.end()) {
            const L1Entry *e   = it->second.get();
            const bool expired = e->expired_flag.load(std::memory_order_acquire);
            if (!expired) {
                return true;
            }
        }
    }

    // L2 — plain mutex, no stats update
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        if (l2_cache_.count(fingerprint)) {
            return true;
        }
    }

    // L3 — best-effort check
    if (l3_db_) {
        try {
            std::lock_guard<std::timed_mutex> lock(l3_mutex_);
            return l3_db_->get(QUERY_CACHE_PREFIX + fingerprint).has_value();
        } catch (const std::exception &e) {
            THEMIS_DEBUG("AdaptiveQueryCache::contains: L3 lookup failed: {}", e.what());
        } catch (const std::string &e) {
            THEMIS_DEBUG("AdaptiveQueryCache::contains: L3 lookup failed: {}", e);
        } catch (const char *e) {
            THEMIS_DEBUG("AdaptiveQueryCache::contains: L3 lookup failed: {}", (e ? e : "<null>"));
        }
    }

    return false;
}

std::size_t AdaptiveQueryCache::size() const {
    std::size_t l1_sz = 0;
    std::size_t l2_sz = 0;

    {
        std::shared_lock<std::shared_mutex> lock(l1_mutex_);
        l1_sz = l1_cache_.size();
    }
    {
        std::lock_guard<std::mutex> lock(l2_mutex_);
        l2_sz = l2_cache_.size();
    }
    // L3 is not counted to avoid an O(n) RocksDB scan.
    return l1_sz + l2_sz;
}

} // namespace themis
