/**
 * @file query_cache_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/query_cache_manager.h"
#include "utils/logger.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

namespace themis {
namespace query {

// ============================================================================
// CacheStatistics Implementation
// ============================================================================

nlohmann::json QueryCacheManager::CacheStatistics::toJson() const {
    return {
        {"total_requests", total_requests},
        {"cache_hits", cache_hits},
        {"cache_misses", cache_misses},
        {"hit_rate", hitRate()},
        {"cache_stores", cache_stores},
        {"cache_evictions", cache_evictions},
        {"cache_invalidations", cache_invalidations},
        {"detected_workload", [this]() {
            switch (detected_workload) {
                case WorkloadType::OLTP: return "OLTP";
                case WorkloadType::OLAP: return "OLAP";
                case WorkloadType::MIXED: return "MIXED";
                case WorkloadType::STREAMING: return "STREAMING";
                case WorkloadType::UNKNOWN: return "UNKNOWN";
                default: return "UNKNOWN";
            }
        }()},
        {"oltp_queries", oltp_queries},
        {"olap_queries", olap_queries},
        {"mixed_queries", mixed_queries},
        {"avg_cache_hit_time_us", avg_cache_hit_time_us},
        {"avg_cache_miss_time_us", avg_cache_miss_time_us},
        {"total_time_saved_ms", total_time_saved_ms},
        {"current_memory_bytes", current_memory_bytes},
        {"max_memory_bytes", max_memory_bytes},
        {"memory_utilization", memoryUtilization()}
    };
}

// ============================================================================
// QueryCacheManager Implementation
// ============================================================================

QueryCacheManager::QueryCacheManager(const Config& config)
    : config_(config)
    , last_stats_report_(std::chrono::system_clock::now())
{
    THEMIS_INFO("QueryCacheManager initializing: caching_enabled={}, cache_type={}, workload_detection={}",
               config_.enable_caching,
               config_.cache_type == Config::CacheType::BASIC ? "BASIC" : "ADAPTIVE",
               config_.enable_workload_detection);
    
    if (!config_.enable_caching) {
        THEMIS_INFO("Query caching is disabled");
        return;
    }
    
    // Initialize workload strategy
    if (config_.enable_workload_detection) {
        WorkloadCacheStrategy::Config strategy_config;
        strategy_config.enable_workload_detection = true;
        strategy_config.detection_sample_rate = config_.workload_detection_sample_rate;
        workload_strategy_ = std::make_unique<WorkloadCacheStrategy>(strategy_config);
        THEMIS_INFO("Workload detection strategy initialized");
    }
    
    // Initialize cache implementation
    if (config_.cache_type == Config::CacheType::BASIC) {
        QueryCache::Config cache_config;
        if (workload_strategy_) {
            auto workload_config = workload_strategy_->getCacheConfig();
            cache_config.max_entries = workload_config.max_entries;
            cache_config.max_memory_bytes = workload_config.max_memory_bytes;
            cache_config.max_entry_size = workload_config.max_entry_size;
            cache_config.default_ttl = workload_config.default_ttl;
            cache_config.eviction_policy = workload_config.eviction_policy;
        }
        basic_cache_ = std::make_unique<QueryCache>(cache_config);
        stats_.max_memory_bytes = cache_config.max_memory_bytes;
        THEMIS_INFO("Basic query cache initialized: max_entries={}, max_memory={}MB",
                   cache_config.max_entries, cache_config.max_memory_bytes / (1024 * 1024));
    } else {
        AdaptiveQueryCache::Config cache_config;
        // Use defaults for now - can be customized based on workload
        adaptive_cache_ = std::make_unique<AdaptiveQueryCache>(cache_config);
        stats_.max_memory_bytes = cache_config.l1_max_entries * 1024 + 
                                  cache_config.l2_max_entries * 10240;
        THEMIS_INFO("Adaptive query cache initialized: L1={} entries, L2={} entries",
                   cache_config.l1_max_entries, cache_config.l2_max_entries);
    }
}

QueryCacheManager::~QueryCacheManager() {
    if (config_.enable_caching && config_.enable_detailed_stats) {
        THEMIS_INFO("QueryCacheManager final statistics: hit_rate={:.2f}%, total_requests={}, "
                   "time_saved={}s, memory_utilization={:.1f}%",
                   stats_.hitRate() * 100.0,
                   stats_.total_requests,
                   stats_.total_time_saved_ms / 1000,
                   stats_.memoryUtilization() * 100.0);
    }
}

std::optional<nlohmann::json> QueryCacheManager::get(
    const std::string& query,
    const nlohmann::json& params
) {
    if (!config_.enable_caching) {
        return std::nullopt;
    }
    
    auto start = std::chrono::steady_clock::now();
    
    std::optional<nlohmann::json> result;
    
    // Try to get from cache
    if (basic_cache_) {
        // For basic cache, let it compute the fingerprint internally
        auto cache_result = basic_cache_->get(query, params);
        if (cache_result.has_value() && cache_result->found) {
            result = cache_result->result;
        }
    } else if (adaptive_cache_) {
        // For adaptive cache, we need the fingerprint
        std::string fingerprint = generateFingerprint(query, params);
        auto cache_entry = adaptive_cache_->get(fingerprint, "");
        if (cache_entry.has_value()) {
            result = cache_entry->result;
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    bool hit = result.has_value();
    updateHitStats(hit, duration_us);
    
    if (hit) {
        THEMIS_DEBUG("Cache HIT: lookup_time={}us", duration_us);
    } else {
        THEMIS_DEBUG("Cache MISS");
    }
    
    reportStatsIfNeeded();
    
    return result;
}

bool QueryCacheManager::put(
    const std::string& query,
    const nlohmann::json& params,
    const nlohmann::json& result,
    const QueryCharacteristics& characteristics,
    const std::vector<std::string>& dependencies
) {
    if (!config_.enable_caching) {
        return false;
    }
    
    std::string fingerprint = generateFingerprint(query, params);
    
    // Record query for workload detection
    if (workload_strategy_) {
        workload_strategy_->recordQuery(fingerprint, characteristics);
        
        // Check if query should be cached
        if (!workload_strategy_->shouldCache(characteristics)) {
            THEMIS_DEBUG("Skipping cache for query: fingerprint={}", fingerprint.substr(0, 16));
            return false;
        }
    }
    
    // Calculate optimal TTL
    std::chrono::seconds ttl{3600};  // Default 1 hour
    if (workload_strategy_) {
        ttl = workload_strategy_->calculateTTL(characteristics);
    }
    
    // Store in cache
    bool success = false;
    if (basic_cache_) {
        success = putInBasicCache(fingerprint, query, params, result, dependencies, ttl);
    } else if (adaptive_cache_) {
        success = putInAdaptiveCache(fingerprint, params, result, ttl);
    }
    
    if (success) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.cache_stores++;
        
        // Track workload type
        if (workload_strategy_) {
            auto query_config = workload_strategy_->getCacheConfigForQuery(characteristics);
            switch (query_config.type) {
                case WorkloadType::OLTP:
                    stats_.oltp_queries++;
                    break;
                case WorkloadType::OLAP:
                    stats_.olap_queries++;
                    break;
                case WorkloadType::MIXED:
                    stats_.mixed_queries++;
                    break;
                default:
                    break;
            }
        }
        
        updateMemoryStats();
        
        THEMIS_DEBUG("Cached query: fingerprint={}, ttl={}s, size={}KB",
                    fingerprint.substr(0, 16),
                    ttl.count(),
                    characteristics.result_size_bytes / 1024);
    }
    
    return success;
}

size_t QueryCacheManager::invalidateByDependency(const std::string& dependency) {
    if (!config_.enable_caching) {
        return 0;
    }
    
    size_t count = 0;
    
    if (basic_cache_) {
        auto result = basic_cache_->invalidateByDependency(dependency);
        if (result.has_value()) {
            count = *result;
        }
    }
    // Note: AdaptiveQueryCache doesn't have dependency tracking yet
    // This could be added as an enhancement
    
    if (count > 0) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.cache_invalidations += count;
        
        THEMIS_INFO("Invalidated {} cache entries for dependency: {}", count, dependency);
    }
    
    return count;
}

bool QueryCacheManager::invalidate(
    const std::string& query,
    const nlohmann::json& params
) {
    if (!config_.enable_caching) {
        return false;
    }
    
    bool removed = false;
    
    if (basic_cache_) {
        auto result = basic_cache_->invalidate(query, params);
        removed = result.has_value() && *result;
    } else if (adaptive_cache_) {
        std::string fingerprint = generateFingerprint(query, params);
        auto result = adaptive_cache_->invalidate(fingerprint);
        removed = (result > 0);
    }
    
    if (removed) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.cache_invalidations++;
    }
    
    return removed;
}

void QueryCacheManager::clear() {
    if (!config_.enable_caching) {
        return;
    }
    
    if (basic_cache_) {
        (void)basic_cache_->clear();
    } else if (adaptive_cache_) {
        (void)adaptive_cache_->clear();
    }
    
    if (workload_strategy_) {
        workload_strategy_->reset();
    }
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    size_t max_mem = stats_.max_memory_bytes;  // Preserve max memory before reset
    stats_ = CacheStatistics();
    stats_.max_memory_bytes = max_mem;  // Restore preserved value
    
    THEMIS_INFO("Query cache cleared");
}

void QueryCacheManager::warmCache(const std::map<std::string, nlohmann::json>& query_results) {
    if (!config_.enable_caching || !config_.enable_cache_warming) {
        return;
    }
    
    size_t warmed = 0;
    for (const auto& [fingerprint, result] : query_results) {
        // Create default characteristics for warming
        QueryCharacteristics char_;
        char_.result_size_bytes = result.dump().size();
        char_.execution_time_ms = 100;  // Assume moderate execution time
        char_.access_count = 1;
        char_.first_seen = std::chrono::system_clock::now();
        char_.last_accessed = char_.first_seen;
        
        // Store with long TTL for warmed entries
        nlohmann::json params = nlohmann::json::object();
        
        if (adaptive_cache_) {
            if (adaptive_cache_->put(fingerprint, params, result)) {
                warmed++;
            }
        } else if (basic_cache_) {
            // For basic cache, we need a query string (use fingerprint as placeholder)
            std::vector<std::string> deps;
            auto put_result = basic_cache_->put(fingerprint, params, result, deps, 
                                               std::chrono::seconds(3600));
            if (put_result.has_value()) {
                warmed++;
            }
        }
    }
    
    THEMIS_INFO("Cache warming complete: {} entries preloaded", warmed);
}

QueryCacheManager::CacheStatistics QueryCacheManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    auto stats = stats_;
    
    // Update with current workload type
    if (workload_strategy_) {
        stats.detected_workload = workload_strategy_->getStats().detected_type;
    }
    
    return stats;
}

std::vector<std::string> QueryCacheManager::getHotQueries(size_t limit) const {
    if (!workload_strategy_) {
        return {};
    }
    
    return workload_strategy_->getHotQueries(limit);
}

WorkloadType QueryCacheManager::getCurrentWorkload() const {
    if (!workload_strategy_) {
        return WorkloadType::UNKNOWN;
    }
    
    return workload_strategy_->getStats().detected_type;
}

nlohmann::json QueryCacheManager::getMonitoringInfo() const {
    nlohmann::json info;
    
    // Overall statistics
    info["statistics"] = getStatistics().toJson();
    
    // Cache-specific info
    if (basic_cache_) {
        info["cache_type"] = "BASIC";
        info["cache_info"] = basic_cache_->getDetailedInfo();
    } else if (adaptive_cache_) {
        info["cache_type"] = "ADAPTIVE";
        info["cache_info"] = adaptive_cache_->getDetailedInfo();
    }
    
    // Workload strategy info
    if (workload_strategy_) {
        info["workload_strategy"] = workload_strategy_->getStats().toJson();
    }
    
    return info;
}

void QueryCacheManager::setConfig(const Config& config) {
    config_ = config;
    THEMIS_INFO("QueryCacheManager configuration updated");
}

QueryCacheManager::Config QueryCacheManager::getConfig() const {
    return config_;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

std::string QueryCacheManager::generateFingerprint(
    const std::string& query,
    const nlohmann::json& params
) const {
    // Concatenate query + params for hashing
    std::string input = query;
    if (!params.empty() && !params.is_null()) {
        input += "::";
        input += params.dump();
    }
    
    // Compute SHA256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()),
           input.size(), hash);
    
    // Convert to hex string
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    
    return ss.str();
}

void QueryCacheManager::updateHitStats(bool hit, int64_t lookup_time_us) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_requests++;
    
    if (hit) {
        stats_.cache_hits++;
        
        // Update average hit time
        if (stats_.cache_hits == 1) {
            stats_.avg_cache_hit_time_us = lookup_time_us;
        } else {
            stats_.avg_cache_hit_time_us = 
                (stats_.avg_cache_hit_time_us * (stats_.cache_hits - 1) + lookup_time_us) / 
                stats_.cache_hits;
        }
    } else {
        stats_.cache_misses++;
        
        // Update average miss time
        if (stats_.cache_misses == 1) {
            stats_.avg_cache_miss_time_us = lookup_time_us;
        } else {
            stats_.avg_cache_miss_time_us = 
                (stats_.avg_cache_miss_time_us * (stats_.cache_misses - 1) + lookup_time_us) / 
                stats_.cache_misses;
        }
    }
}

void QueryCacheManager::updateMemoryStats() {
    // Update memory statistics from underlying cache
    if (basic_cache_) {
        auto cache_stats = basic_cache_->getStats();
        stats_.current_memory_bytes = cache_stats.current_memory_bytes;
        stats_.cache_evictions = cache_stats.evictions;
    } else if (adaptive_cache_) {
        auto cache_stats = adaptive_cache_->getStats();
        stats_.cache_evictions = cache_stats.evictions;
        // Estimate memory usage for adaptive cache based on detailed info
        auto cache_info = adaptive_cache_->getDetailedInfo();
        if (cache_info.contains("total_memory_bytes")) {
            stats_.current_memory_bytes = cache_info["total_memory_bytes"].get<size_t>();
        } else {
            // Fallback: estimate based on entry counts and average sizes
            // L1: ~1KB, L2: ~10KB (compressed), L3: varies
            stats_.current_memory_bytes = 0;  // Use conservative estimate
        }
    }
}

void QueryCacheManager::reportStatsIfNeeded() {
    if (!config_.enable_detailed_stats) {
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_report_);
    
    if (elapsed >= config_.stats_report_interval) {
        auto stats = getStatistics();
        THEMIS_INFO("Query Cache Statistics: hit_rate={:.2f}%, requests={}, hits={}, misses={}, "
                   "memory={:.1f}MB/{:.1f}MB ({:.1f}%), workload={}",
                   stats.hitRate() * 100.0,
                   stats.total_requests,
                   stats.cache_hits,
                   stats.cache_misses,
                   stats.current_memory_bytes / (1024.0 * 1024.0),
                   stats.max_memory_bytes / (1024.0 * 1024.0),
                   stats.memoryUtilization() * 100.0,
                   [&stats]() {
                       switch (stats.detected_workload) {
                           case WorkloadType::OLTP: return "OLTP";
                           case WorkloadType::OLAP: return "OLAP";
                           case WorkloadType::MIXED: return "MIXED";
                           case WorkloadType::STREAMING: return "STREAMING";
                           default: return "UNKNOWN";
                       }
                   }());
        
        last_stats_report_ = now;
    }
}

bool QueryCacheManager::putInBasicCache(
    [[maybe_unused]] const std::string& fingerprint,
    const std::string& query,
    const nlohmann::json& params,
    const nlohmann::json& result,
    const std::vector<std::string>& dependencies,
    std::chrono::seconds ttl
) {
    auto put_result = basic_cache_->put(query, params, result, dependencies, ttl);
    return put_result.has_value();
}

bool QueryCacheManager::putInAdaptiveCache(
    const std::string& fingerprint,
    const nlohmann::json& params,
    const nlohmann::json& result,
    [[maybe_unused]] std::chrono::seconds ttl
) {
    // AdaptiveQueryCache doesn't support custom TTL in the current implementation
    // It uses its own adaptive TTL logic
    return adaptive_cache_->put(fingerprint, params, result);
}

} // namespace query
} // namespace themis
