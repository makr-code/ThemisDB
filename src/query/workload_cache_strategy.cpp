/**
 * @file workload_cache_strategy.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/workload_cache_strategy.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>

namespace themis {
namespace query {

// ============================================================================
// WorkloadCacheConfig Implementation
// ============================================================================

WorkloadCacheConfig WorkloadCacheConfig::forWorkload(WorkloadType type) {
    WorkloadCacheConfig config;
    config.type = type;
    
    switch (type) {
        case WorkloadType::OLTP:
            // OLTP: High-frequency, small results, short TTL
            config.max_entries = 50000;                    // More entries for frequent queries
            config.max_memory_bytes = 200 * 1024 * 1024;   // 200MB
            config.max_entry_size = 100 * 1024;            // 100KB max per entry
            config.default_ttl = std::chrono::seconds(300); // 5 minutes
            config.min_ttl = std::chrono::seconds(30);      // 30 seconds
            config.max_ttl = std::chrono::seconds(3600);    // 1 hour
            config.eviction_policy = QueryCache::EvictionPolicy::LRU;  // LRU for recency
            config.enable_adaptive_ttl = true;
            config.enable_frequency_weighting = true;
            config.high_frequency_threshold = 10.0;         // 10 queries/min
            config.small_result_threshold = 50 * 1024;      // 50KB
            THEMIS_INFO("Created OLTP cache configuration: max_entries={}, default_ttl={}s",
                       config.max_entries, config.default_ttl.count());
            break;
            
        case WorkloadType::OLAP:
            // OLAP: Low-frequency, large results, long TTL
            config.max_entries = 5000;                      // Fewer entries for large results
            config.max_memory_bytes = 500 * 1024 * 1024;    // 500MB for large results
            config.max_entry_size = 50 * 1024 * 1024;       // 50MB max per entry
            config.default_ttl = std::chrono::seconds(7200); // 2 hours
            config.min_ttl = std::chrono::seconds(600);      // 10 minutes
            config.max_ttl = std::chrono::seconds(86400);    // 24 hours
            config.eviction_policy = QueryCache::EvictionPolicy::LFU;  // LFU for expensive queries
            config.enable_adaptive_ttl = true;
            config.enable_frequency_weighting = false;       // Size matters more than frequency
            config.low_frequency_threshold = 0.1;            // 0.1 queries/min
            config.large_result_threshold = 5 * 1024 * 1024; // 5MB
            THEMIS_INFO("Created OLAP cache configuration: max_entries={}, default_ttl={}s",
                       config.max_entries, config.default_ttl.count());
            break;
            
        case WorkloadType::MIXED:
            // MIXED: Adaptive behavior, balanced settings
            config.max_entries = 20000;
            config.max_memory_bytes = 300 * 1024 * 1024;    // 300MB
            config.max_entry_size = 10 * 1024 * 1024;       // 10MB max per entry
            config.default_ttl = std::chrono::seconds(1800); // 30 minutes
            config.min_ttl = std::chrono::seconds(60);       // 1 minute
            config.max_ttl = std::chrono::seconds(43200);    // 12 hours
            config.eviction_policy = QueryCache::EvictionPolicy::LRU;
            config.enable_adaptive_ttl = true;
            config.enable_frequency_weighting = true;
            config.high_frequency_threshold = 5.0;           // 5 queries/min
            config.low_frequency_threshold = 0.5;            // 0.5 queries/min
            config.small_result_threshold = 100 * 1024;      // 100KB
            config.large_result_threshold = 2 * 1024 * 1024; // 2MB
            THEMIS_INFO("Created MIXED cache configuration: max_entries={}, default_ttl={}s",
                       config.max_entries, config.default_ttl.count());
            break;
            
        case WorkloadType::STREAMING:
            // STREAMING: Minimal caching for real-time data
            config.max_entries = 1000;                       // Very few entries
            config.max_memory_bytes = 10 * 1024 * 1024;      // 10MB
            config.max_entry_size = 100 * 1024;              // 100KB max per entry
            config.default_ttl = std::chrono::seconds(10);   // 10 seconds only
            config.min_ttl = std::chrono::seconds(1);        // 1 second
            config.max_ttl = std::chrono::seconds(60);       // 1 minute max
            config.eviction_policy = QueryCache::EvictionPolicy::LRU;
            config.enable_adaptive_ttl = false;              // Fixed short TTL
            config.enable_frequency_weighting = false;
            THEMIS_INFO("Created STREAMING cache configuration: max_entries={}, default_ttl={}s",
                       config.max_entries, config.default_ttl.count());
            break;
            
        case WorkloadType::UNKNOWN:
        [[fallthrough]];
        default:
            // UNKNOWN: Conservative defaults
            config.max_entries = 10000;
            config.max_memory_bytes = 100 * 1024 * 1024;     // 100MB
            config.max_entry_size = 10 * 1024 * 1024;        // 10MB
            config.default_ttl = std::chrono::seconds(3600); // 1 hour
            config.eviction_policy = QueryCache::EvictionPolicy::LRU;
            THEMIS_INFO("Created UNKNOWN cache configuration (using defaults): max_entries={}, default_ttl={}s",
                       config.max_entries, config.default_ttl.count());
            break;
    }
    
    return config;
}

// ============================================================================
// WorkloadCacheStrategy::WorkloadStats Implementation
// ============================================================================

nlohmann::json WorkloadCacheStrategy::WorkloadStats::toJson() const {
    return {
        {"detected_type", [this]() {
            switch (detected_type) {
                case WorkloadType::OLTP: return "OLTP";
                case WorkloadType::OLAP: return "OLAP";
                case WorkloadType::MIXED: return "MIXED";
                case WorkloadType::STREAMING: return "STREAMING";
                case WorkloadType::UNKNOWN: return "UNKNOWN";
                default: return "UNKNOWN";
            }
        }()},
        {"total_queries", total_queries},
        {"cached_queries", cached_queries},
        {"cache_hits", cache_hits},
        {"cache_misses", cache_misses},
        {"hit_rate", hit_rate()},
        {"avg_query_frequency", avg_query_frequency},
        {"avg_result_size", avg_result_size},
        {"avg_execution_time_ms", avg_execution_time_ms}
    };
}

// ============================================================================
// WorkloadCacheStrategy Implementation
// ============================================================================

WorkloadCacheStrategy::WorkloadCacheStrategy(const Config& config)
    : config_(config)
    , last_detection_(std::chrono::system_clock::now())
{
    THEMIS_INFO("WorkloadCacheStrategy initialized: detection_enabled={}, sample_rate={}",
               config_.enable_workload_detection, config_.detection_sample_rate);
}

void WorkloadCacheStrategy::recordQuery(
    const std::string& query_fingerprint,
    const QueryCharacteristics& characteristics
) {
    // Apply sampling rate
    if (config_.enable_workload_detection && config_.detection_sample_rate < 1.0) {
        static thread_local std::mt19937 rng(std::random_device{}());
        static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) > config_.detection_sample_rate) {
            return;  // Skip this query based on sampling rate
        }
    }
    
    bool should_detect = false;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Update or create query pattern entry
        auto it = query_patterns_.find(query_fingerprint);
        if (it == query_patterns_.end()) {
            // New query pattern
            QueryCharacteristics new_char = characteristics;
            new_char.first_seen = std::chrono::system_clock::now();
            new_char.last_accessed = new_char.first_seen;
            new_char.access_count = 1;
            query_patterns_[query_fingerprint] = new_char;
        } else {
            // Update existing pattern
            auto& pattern = it->second;
            pattern.last_accessed = std::chrono::system_clock::now();
            int64_t count = pattern.access_count;
            pattern.access_count++;
            
            // Update cumulative averages (proper weighted average)
            pattern.result_size_bytes = 
                (pattern.result_size_bytes * count + characteristics.result_size_bytes) / (count + 1);
            pattern.execution_time_ms = 
                (pattern.execution_time_ms * count + characteristics.execution_time_ms) / (count + 1);
            pattern.rows_scanned = 
                (pattern.rows_scanned * count + characteristics.rows_scanned) / (count + 1);
            pattern.rows_returned = 
                (pattern.rows_returned * count + characteristics.rows_returned) / (count + 1);
        }
        
        // Update global stats
        stats_.total_queries++;
        
        // Check if workload detection should run (without holding lock)
        should_detect = config_.enable_workload_detection && shouldRunDetection();
    }
    
    // Trigger workload detection if needed (outside the lock to avoid deadlock)
    if (should_detect) {
        detectWorkload();
    }
}

WorkloadType WorkloadCacheStrategy::detectWorkload() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (static_cast<int>(query_patterns_.size()) < config_.min_samples_for_detection) {
        THEMIS_DEBUG("Insufficient samples for workload detection: {} < {}",
                    query_patterns_.size(), config_.min_samples_for_detection);
        return WorkloadType::UNKNOWN;
    }
    
    WorkloadType detected = classifyWorkload();
    
    if (detected != current_workload_.load()) {
        THEMIS_INFO("Workload type changed: {} -> {}", 
                   static_cast<int>(current_workload_.load()),
                   static_cast<int>(detected));
        current_workload_.store(detected);
        stats_.detected_type = detected;
    }
    
    updateStats();
    last_detection_ = std::chrono::system_clock::now();
    
    return detected;
}

WorkloadType WorkloadCacheStrategy::classifyWorkload() const {
    // Calculate aggregate metrics
    size_t total_patterns = query_patterns_.size();
    if (total_patterns == 0) {
        return WorkloadType::UNKNOWN;
    }
    double avg_frequency = 0.0;
    size_t avg_result_size = 0;
    size_t high_freq_count = 0;
    size_t low_freq_count = 0;
    size_t large_result_count = 0;
    size_t small_result_count = 0;
    
    for (const auto& [fp, pattern] : query_patterns_) {
        double freq = pattern.frequency_per_minute();
        avg_frequency += freq;
        avg_result_size += pattern.result_size_bytes;
        
        if (freq > config_.oltp_frequency_threshold) {
            high_freq_count++;
        } else if (freq < config_.olap_frequency_threshold) {
            low_freq_count++;
        }
        
        if (pattern.result_size_bytes > config_.olap_result_size_threshold) {
            large_result_count++;
        } else if (pattern.result_size_bytes < config_.oltp_result_size_threshold) {
            small_result_count++;
        }
    }
    
    avg_frequency /= total_patterns;
    avg_result_size /= total_patterns;
    
    // Classification logic
    double high_freq_ratio = static_cast<double>(high_freq_count) / total_patterns;
    double low_freq_ratio = static_cast<double>(low_freq_count) / total_patterns;
    double small_result_ratio = static_cast<double>(small_result_count) / total_patterns;
    double large_result_ratio = static_cast<double>(large_result_count) / total_patterns;
    
    THEMIS_DEBUG("Workload classification metrics: avg_freq={:.2f}/min, avg_size={}KB, "
                "high_freq_ratio={:.2f}, large_result_ratio={:.2f}",
                avg_frequency, avg_result_size / 1024, high_freq_ratio, large_result_ratio);
    
    // OLTP: High frequency + small results
    if (high_freq_ratio > 0.6 && small_result_ratio > 0.7) {
        THEMIS_INFO("Detected OLTP workload: high_freq={:.1f}%, small_results={:.1f}%",
                   high_freq_ratio * 100, small_result_ratio * 100);
        return WorkloadType::OLTP;
    }
    
    // OLAP: Low frequency + large results
    if (low_freq_ratio > 0.6 && large_result_ratio > 0.5) {
        THEMIS_INFO("Detected OLAP workload: low_freq={:.1f}%, large_results={:.1f}%",
                   low_freq_ratio * 100, large_result_ratio * 100);
        return WorkloadType::OLAP;
    }
    
    // STREAMING: Very high frequency + very small results + low TTL preference
    if (avg_frequency > 50.0 && avg_result_size < 10 * 1024) {
        THEMIS_INFO("Detected STREAMING workload: avg_freq={:.1f}/min, avg_size={}KB",
                   avg_frequency, avg_result_size / 1024);
        return WorkloadType::STREAMING;
    }
    
    // Default to MIXED for unclear patterns
    THEMIS_INFO("Detected MIXED workload: avg_freq={:.1f}/min, avg_size={}KB",
               avg_frequency, avg_result_size / 1024);
    return WorkloadType::MIXED;
}

WorkloadCacheConfig WorkloadCacheStrategy::getCacheConfig() const {
    WorkloadType type = current_workload_.load();
    return WorkloadCacheConfig::forWorkload(type);
}

WorkloadCacheConfig WorkloadCacheStrategy::getCacheConfigForQuery(
    const QueryCharacteristics& characteristics
) const {
    // Analyze individual query to determine best caching strategy
    double freq = characteristics.frequency_per_minute();
    size_t result_size = characteristics.result_size_bytes;
    
    WorkloadType query_type = WorkloadType::MIXED;
    
    // High frequency + small result = OLTP-like
    if (freq > config_.oltp_frequency_threshold && 
        result_size < config_.oltp_result_size_threshold) {
        query_type = WorkloadType::OLTP;
    }
    // Low frequency + large result = OLAP-like
    else if (freq < config_.olap_frequency_threshold && 
             result_size > config_.olap_result_size_threshold) {
        query_type = WorkloadType::OLAP;
    }
    // Very high frequency = streaming-like
    else if (freq > 50.0) {
        query_type = WorkloadType::STREAMING;
    }
    
    return WorkloadCacheConfig::forWorkload(query_type);
}

bool WorkloadCacheStrategy::shouldCache(
    const QueryCharacteristics& characteristics
) const {
    // Don't cache extremely large results (memory pressure)
    if (characteristics.result_size_bytes > 100 * 1024 * 1024) {  // 100MB
        THEMIS_DEBUG("Skipping cache: result too large ({}MB)",
                    characteristics.result_size_bytes / (1024 * 1024));
        return false;
    }
    
    // Don't cache very fast queries (overhead not worth it)
    if (characteristics.execution_time_ms < 5) {  // 5ms
        THEMIS_DEBUG("Skipping cache: query too fast ({}ms)",
                    characteristics.execution_time_ms);
        return false;
    }
    
    // Don't cache queries with very low selectivity (full table scans)
    if (characteristics.selectivity() > 0.9 && 
        characteristics.rows_scanned > 1000000) {
        THEMIS_DEBUG("Skipping cache: low selectivity full scan");
        return false;
    }
    
    return true;
}

std::chrono::seconds WorkloadCacheStrategy::calculateTTL(
    const QueryCharacteristics& characteristics
) const {
    auto config = getCacheConfigForQuery(characteristics);
    
    if (!config.enable_adaptive_ttl) {
        return config.default_ttl;
    }
    
    // Adaptive TTL based on frequency
    double freq = characteristics.frequency_per_minute();
    
    // Use config thresholds instead of hard-coded values
    double high_freq_threshold = config.high_frequency_threshold > 0 
        ? config.high_frequency_threshold 
        : 10.0;
    double low_freq_threshold = config.low_frequency_threshold > 0 
        ? config.low_frequency_threshold 
        : 0.1;
    
    // High frequency = shorter TTL (data changes more often)
    // Low frequency = longer TTL (expensive to recompute)
    std::chrono::seconds ttl;
    
    if (freq > high_freq_threshold) {
        // High frequency: short TTL
        ttl = config.min_ttl;
    } else if (freq < low_freq_threshold) {
        // Low frequency: long TTL
        ttl = config.max_ttl;
    } else {
        // Scale between min and max based on frequency
        // Use logarithmic scale for better distribution
        // Compute log values based on actual thresholds
        double log_min = std::log10(low_freq_threshold + 1.0);
        double log_max = std::log10(high_freq_threshold + 1.0);
        double log_range = log_max - log_min;
        
        double log_freq = std::log10(freq + 1.0);
        double ratio = (log_freq - log_min) / log_range;
        ratio = std::clamp(ratio, 0.0, 1.0);
        
        // Inverse relationship: higher frequency = lower TTL
        ratio = 1.0 - ratio;
        
        long long ttl_seconds = config.min_ttl.count() + 
            static_cast<long long>(ratio * (config.max_ttl.count() - config.min_ttl.count()));
        ttl = std::chrono::seconds(ttl_seconds);
    }
    
    THEMIS_DEBUG("Calculated adaptive TTL: freq={:.2f}/min -> ttl={}s",
                freq, ttl.count());
    
    return ttl;
}

std::vector<std::string> WorkloadCacheStrategy::getHotQueries(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create vector of (fingerprint, access_count) pairs
    std::vector<std::pair<std::string, int64_t>> query_frequencies;
    query_frequencies.reserve(query_patterns_.size());
    
    for (const auto& [fp, pattern] : query_patterns_) {
        query_frequencies.emplace_back(fp, pattern.access_count);
    }
    
    const size_t top_n = std::min(limit, query_frequencies.size());

    // Sort by access count (descending)
    std::partial_sort(
        query_frequencies.begin(),
        query_frequencies.begin() + top_n,
        query_frequencies.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; }
    );
    
    // Extract fingerprints
    std::vector<std::string> hot_queries = {};

    hot_queries.reserve(top_n);
    
    for (size_t i = 0; i < top_n; ++i) {
        hot_queries.push_back(query_frequencies[i].first);
    }
    
    THEMIS_INFO("Identified {} hot queries from {} total patterns",
               hot_queries.size(),static_cast<int>(query_patterns_.size()));
    
    return hot_queries;
}

WorkloadCacheStrategy::WorkloadStats WorkloadCacheStrategy::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void WorkloadCacheStrategy::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    query_patterns_.clear();
    stats_ = WorkloadStats();
    current_workload_.store(WorkloadType::UNKNOWN);
    last_detection_ = std::chrono::system_clock::now();
    
    THEMIS_INFO("WorkloadCacheStrategy reset");
}

void WorkloadCacheStrategy::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    THEMIS_INFO("WorkloadCacheStrategy config updated");
}

WorkloadCacheStrategy::Config WorkloadCacheStrategy::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void WorkloadCacheStrategy::updateStats() {
    // Calculate aggregate statistics from query patterns
    if (query_patterns_.empty()) {
        return;
    }
    
    double total_frequency = 0.0;
    size_t total_result_size = 0;
    int64_t total_exec_time = 0;
    
    for (const auto& [fp, pattern] : query_patterns_) {
        total_frequency += pattern.frequency_per_minute();
        total_result_size += pattern.result_size_bytes;
        total_exec_time += pattern.execution_time_ms;
    }
    
    stats_.avg_query_frequency = total_frequency / query_patterns_.size();
    stats_.avg_result_size = total_result_size / query_patterns_.size();
    stats_.avg_execution_time_ms = total_exec_time / query_patterns_.size();
}

bool WorkloadCacheStrategy::shouldRunDetection() const {
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_detection_);
    return elapsed >= config_.detection_window;
}

} // namespace query
} // namespace themis

