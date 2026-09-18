/**
 * @file cache_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace cache {

struct CacheMetrics {
    // Hit/Miss metrics per tier
    std::atomic<uint64_t> l1_hits{0};
    std::atomic<uint64_t> l2_hits{0};
    std::atomic<uint64_t> l3_hits{0};
    std::atomic<uint64_t> misses{0};
    
    // Eviction and promotion metrics
    std::atomic<uint64_t> evictions{0};
    std::atomic<uint64_t> promotions{0};
    std::atomic<uint64_t> demotions{0};
    
    // Error metrics
    std::atomic<uint64_t> l3_read_errors{0};
    std::atomic<uint64_t> l3_write_errors{0};
    std::atomic<uint64_t> compression_failures{0};
    std::atomic<uint64_t> decompression_failures{0};
    std::atomic<uint64_t> size_limit_rejections{0};
    
    // Size metrics
    std::atomic<uint64_t> total_bytes_cached{0};
    std::atomic<uint64_t> total_bytes_compressed{0};
    
    // Timing metrics (microseconds)
    std::atomic<uint64_t> l1_lookup_time_us{0};
    std::atomic<uint64_t> l2_lookup_time_us{0};
    std::atomic<uint64_t> l3_lookup_time_us{0};
    
    // Circuit breaker state
    std::atomic<uint64_t> l3_circuit_breaker_trips{0};
    std::atomic<bool> l3_circuit_breaker_open{false};
    
    // Phase 2: Rate limiting metrics
    std::atomic<uint64_t> rate_limited_requests{0};
    std::atomic<uint64_t> backpressure_events{0};

    // Phase 3: Warmup metrics (Prometheus: themis_cache_warmup_entries_loaded_total)
    std::atomic<uint64_t> warmup_entries_loaded{0};
    std::atomic<uint64_t> warmup_entries_skipped{0};
    std::atomic<uint64_t> warmup_entries_failed{0};

    // Phase 3: Adaptive TTL tuning metrics
    // themis_cache_ttl_extended_total: entries whose TTL was extended (hot-key policy)
    // themis_cache_ttl_shortened_total: entries whose TTL was shortened (cold-key policy)
    std::atomic<uint64_t> ttl_extended_total{0};
    std::atomic<uint64_t> ttl_shortened_total{0};

    // Phase 4: Predictive pre-fetching metrics
    // themis_cache_prefetch_candidates_generated_total: number of prefetch prediction calls
    // themis_cache_prefetch_hits_total: candidates that were already warm in cache
    std::atomic<uint64_t> prefetch_candidates_generated{0};
    std::atomic<uint64_t> prefetch_hits{0};
    // Phase 4: Write-through metrics
    // themis_cache_write_through_total: entries written through from L1/L2 to L3
    std::atomic<uint64_t> write_through_total{0};
    std::atomic<uint64_t> write_through_errors{0};
    // Phase 4: Write-through cache mode metrics
    // themis_cache_write_through_total: total writes that went to multiple tiers
    std::atomic<uint64_t> write_through_writes{0};

    CacheMetrics() = default;

    CacheMetrics(const CacheMetrics& other) {
        l1_hits.store(other.l1_hits.load());
        l2_hits.store(other.l2_hits.load());
        l3_hits.store(other.l3_hits.load());
        misses.store(other.misses.load());
        evictions.store(other.evictions.load());
        promotions.store(other.promotions.load());
        demotions.store(other.demotions.load());
        l3_read_errors.store(other.l3_read_errors.load());
        l3_write_errors.store(other.l3_write_errors.load());
        compression_failures.store(other.compression_failures.load());
        decompression_failures.store(other.decompression_failures.load());
        size_limit_rejections.store(other.size_limit_rejections.load());
        total_bytes_cached.store(other.total_bytes_cached.load());
        total_bytes_compressed.store(other.total_bytes_compressed.load());
        l1_lookup_time_us.store(other.l1_lookup_time_us.load());
        l2_lookup_time_us.store(other.l2_lookup_time_us.load());
        l3_lookup_time_us.store(other.l3_lookup_time_us.load());
        l3_circuit_breaker_trips.store(other.l3_circuit_breaker_trips.load());
        l3_circuit_breaker_open.store(other.l3_circuit_breaker_open.load());
        rate_limited_requests.store(other.rate_limited_requests.load());
        backpressure_events.store(other.backpressure_events.load());
        warmup_entries_loaded.store(other.warmup_entries_loaded.load());
        warmup_entries_skipped.store(other.warmup_entries_skipped.load());
        warmup_entries_failed.store(other.warmup_entries_failed.load());
        ttl_extended_total.store(other.ttl_extended_total.load());
        ttl_shortened_total.store(other.ttl_shortened_total.load());
        prefetch_candidates_generated.store(other.prefetch_candidates_generated.load());
        prefetch_hits.store(other.prefetch_hits.load());
        write_through_total.store(other.write_through_total.load());
        write_through_errors.store(other.write_through_errors.load());
        write_through_writes.store(other.write_through_writes.load());
    }

    CacheMetrics& operator=(const CacheMetrics& other) {
        if (this != &other) {
            l1_hits.store(other.l1_hits.load());
            l2_hits.store(other.l2_hits.load());
            l3_hits.store(other.l3_hits.load());
            misses.store(other.misses.load());
            evictions.store(other.evictions.load());
            promotions.store(other.promotions.load());
            demotions.store(other.demotions.load());
            l3_read_errors.store(other.l3_read_errors.load());
            l3_write_errors.store(other.l3_write_errors.load());
            compression_failures.store(other.compression_failures.load());
            decompression_failures.store(other.decompression_failures.load());
            size_limit_rejections.store(other.size_limit_rejections.load());
            total_bytes_cached.store(other.total_bytes_cached.load());
            total_bytes_compressed.store(other.total_bytes_compressed.load());
            l1_lookup_time_us.store(other.l1_lookup_time_us.load());
            l2_lookup_time_us.store(other.l2_lookup_time_us.load());
            l3_lookup_time_us.store(other.l3_lookup_time_us.load());
            l3_circuit_breaker_trips.store(other.l3_circuit_breaker_trips.load());
            l3_circuit_breaker_open.store(other.l3_circuit_breaker_open.load());
            rate_limited_requests.store(other.rate_limited_requests.load());
            backpressure_events.store(other.backpressure_events.load());
            warmup_entries_loaded.store(other.warmup_entries_loaded.load());
            warmup_entries_skipped.store(other.warmup_entries_skipped.load());
            warmup_entries_failed.store(other.warmup_entries_failed.load());
            ttl_extended_total.store(other.ttl_extended_total.load());
            ttl_shortened_total.store(other.ttl_shortened_total.load());
            prefetch_candidates_generated.store(other.prefetch_candidates_generated.load());
            prefetch_hits.store(other.prefetch_hits.load());
            write_through_total.store(other.write_through_total.load());
            write_through_errors.store(other.write_through_errors.load());
            write_through_writes.store(other.write_through_writes.load());
        }
        return *this;
    }
    
    double getHitRate() const {
        uint64_t total = l1_hits.load() + l2_hits.load() + l3_hits.load() + misses.load();
        return total > 0 ? static_cast<double>(l1_hits + l2_hits + l3_hits) / total : 0.0;
    }
    
    double getL1HitRate() const {
        uint64_t total = l1_hits.load() + l2_hits.load() + l3_hits.load() + misses.load();
        return total > 0 ? static_cast<double>(l1_hits) / total : 0.0;
    }
    
    double getCompressionRatio() const {
        uint64_t cached = total_bytes_cached.load();
        uint64_t compressed = total_bytes_compressed.load();
        if (compressed == 0 || cached == 0) {
            return 1.0;  // No compression applied or no data
        }
        return static_cast<double>(cached) / compressed;
    }
    
    nlohmann::json toJson() const {
        nlohmann::json j;
        
        // Hit/Miss metrics
        j["hits"]["l1"] = l1_hits.load();
        j["hits"]["l2"] = l2_hits.load();
        j["hits"]["l3"] = l3_hits.load();
        j["misses"] = misses.load();
        j["hit_rate"] = getHitRate();
        j["l1_hit_rate"] = getL1HitRate();
        
        // Eviction metrics
        j["evictions"] = evictions.load();
        j["promotions"] = promotions.load();
        j["demotions"] = demotions.load();
        
        // Error metrics
        j["errors"]["l3_read"] = l3_read_errors.load();
        j["errors"]["l3_write"] = l3_write_errors.load();
        j["errors"]["compression"] = compression_failures.load();
        j["errors"]["decompression"] = decompression_failures.load();
        j["errors"]["size_limit_rejections"] = size_limit_rejections.load();
        
        // Size metrics
        j["bytes"]["cached"] = total_bytes_cached.load();
        j["bytes"]["compressed"] = total_bytes_compressed.load();
        j["compression_ratio"] = getCompressionRatio();
        
        // Circuit breaker
        j["circuit_breaker"]["trips"] = l3_circuit_breaker_trips.load();
        j["circuit_breaker"]["open"] = l3_circuit_breaker_open.load();
        
        // Phase 2: Rate limiting
        j["rate_limiting"]["rejected_requests"] = rate_limited_requests.load();
        j["backpressure"]["events"] = backpressure_events.load();

        // Phase 3: Warmup metrics
        j["warmup"]["entries_loaded"] = warmup_entries_loaded.load();
        j["warmup"]["entries_skipped"] = warmup_entries_skipped.load();
        j["warmup"]["entries_failed"] = warmup_entries_failed.load();

        // Phase 3: Adaptive TTL tuning metrics
        j["adaptive_ttl"]["ttl_extended_total"] = ttl_extended_total.load();
        j["adaptive_ttl"]["ttl_shortened_total"] = ttl_shortened_total.load();

        // Phase 4: Predictive pre-fetching metrics
        j["prefetch"]["candidates_generated"] = prefetch_candidates_generated.load();
        j["prefetch"]["hits"]                 = prefetch_hits.load();
        // Phase 4: Write-through metrics
        j["write_through"]["total"] = write_through_total.load();
        j["write_through"]["errors"] = write_through_errors.load();
        // Phase 4: Write-through cache mode metrics
        j["write_through"]["writes"] = write_through_writes.load();
        
        return j;
    }
    
    /**
     * @brief Reset the modification detection flag.
     * @details Implements reset without additional internal calls.
     */
    void reset() {
        l1_hits = 0;
        l2_hits = 0;
        l3_hits = 0;
        misses = 0;
        evictions = 0;
        promotions = 0;
        demotions = 0;
        l3_read_errors = 0;
        l3_write_errors = 0;
        compression_failures = 0;
        decompression_failures = 0;
        size_limit_rejections = 0;
        total_bytes_cached = 0;
        total_bytes_compressed = 0;
        l1_lookup_time_us = 0;
        l2_lookup_time_us = 0;
        l3_lookup_time_us = 0;
        l3_circuit_breaker_trips = 0;
        l3_circuit_breaker_open = false;
        warmup_entries_loaded = 0;
        warmup_entries_skipped = 0;
        warmup_entries_failed = 0;
        ttl_extended_total = 0;
        ttl_shortened_total = 0;
        prefetch_candidates_generated = 0;
        prefetch_hits = 0;
        write_through_total = 0;
        write_through_errors = 0;
        write_through_writes = 0;
    }
};

class CircuitBreaker {
public:
    enum class State {
        CLOSED,     // Normal operation
        OPEN,       // Failing, reject requests
        HALF_OPEN   // Testing if service recovered
    };
    
    struct Config {
        uint32_t failure_threshold = 5;        // Failures before opening
        uint32_t success_threshold = 2;        // Successes before closing from half-open
        uint32_t timeout_ms = 60000;           // Time before trying half-open (1 minute)
        uint32_t half_open_max_calls = 3;      // Max calls in half-open state
        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };
    
    explicit CircuitBreaker(const Config& config = Config::defaults())
        : config_(config)
        , state_(State::CLOSED)
        , failure_count_(0)
        , success_count_(0)
        , last_failure_time_(std::chrono::steady_clock::time_point::min())
        , half_open_calls_(0) {}
    
    /**
     * @brief Allow Request.
     * @return True when the operation succeeds.
     * @details Calls: std::chrono::steady_clock::now(), load(), count().
     */
    bool allowRequest() {
        auto now = std::chrono::steady_clock::now();
        
        if (state_ == State::OPEN) {
            // Check if timeout expired
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_failure_time_.load()
            ).count();
            
            if (elapsed >= config_.timeout_ms) {
                // Transition to HALF_OPEN
                state_ = State::HALF_OPEN;
                half_open_calls_ = 0;
                success_count_ = 0;
            } else {
                return false;  // Still open, reject
            }
        }
        
        if (state_ == State::HALF_OPEN) {
            if (half_open_calls_.load() >= config_.half_open_max_calls) {
                return false;  // Too many test calls
            }
            half_open_calls_++;
        }
        
        return true;  // CLOSED or HALF_OPEN with capacity
    }
    
    /**
     * @brief Record Success.
     * @details Implements recordSuccess without additional internal calls.
     */
    void recordSuccess() {
        if (state_ == State::HALF_OPEN) {
            success_count_++;
            if (success_count_ >= config_.success_threshold) {
                // Transition to CLOSED
                state_ = State::CLOSED;
                failure_count_ = 0;
                success_count_ = 0;
            }
        } else if (state_ == State::CLOSED) {
            failure_count_ = 0;  // Reset failure count on success
        }
    }
    
    /**
     * @brief Record Failure.
     * @details Calls: std::chrono::steady_clock::now().
     */
    void recordFailure() {
        last_failure_time_ = std::chrono::steady_clock::now();
        
        if (state_ == State::HALF_OPEN) {
            // Immediate transition back to OPEN
            state_ = State::OPEN;
            success_count_ = 0;
        } else if (state_ == State::CLOSED) {
            failure_count_++;
            if (failure_count_ >= config_.failure_threshold) {
                // Transition to OPEN
                state_ = State::OPEN;
            }
        }
    }
    
    State getState() const {
        return state_;
    }
    
    bool isOpen() const {
        return state_ == State::OPEN;
    }
    
    /**
     * @brief Reset the modification detection flag.
     * @details Implements reset without additional internal calls.
     */
    void reset() {
        state_ = State::CLOSED;
        failure_count_ = 0;
        success_count_ = 0;
        half_open_calls_ = 0;
    }

    uint32_t getFailureCount() const {
        return failure_count_.load();
    }

private:
    Config config_;
    std::atomic<State> state_;
    std::atomic<uint32_t> failure_count_;
    std::atomic<uint32_t> success_count_;
    std::atomic<std::chrono::steady_clock::time_point> last_failure_time_;
    std::atomic<uint32_t> half_open_calls_;
};

class RateLimiter {
public:
    struct Config {
        uint32_t max_requests_per_second = 10000;  // Rate limit
        uint32_t burst_size = 0;                    // Burst size (0 = same as rate)
        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };
    
    explicit RateLimiter(const Config& config = Config::defaults())
        : config_(config)
        , tokens_(config.max_requests_per_second)
        , last_refill_time_(std::chrono::steady_clock::now()) {
        if (config_.burst_size == 0) {
            config_.burst_size = config_.max_requests_per_second;
        }
    }
    
    /**
     * @brief Try Acquire.
     * @return True when the operation succeeds.
     * @details Calls: refillTokens(), load(), compare_exchange_weak().
     */
    bool tryAcquire() {
        refillTokens();
        
        uint32_t current = tokens_.load();
        while (current > 0) {
            if (tokens_.compare_exchange_weak(current, current - 1)) {
                return true;  // Acquired token
            }
        }
        return false;  // Rate limited
    }
    
    uint32_t availableTokens() const {
        return tokens_.load();
    }
    
    /**
     * @brief Reset the modification detection flag.
     * @details Calls: std::chrono::steady_clock::now().
     */
    void reset() {
        tokens_ = config_.max_requests_per_second;
        last_refill_time_ = std::chrono::steady_clock::now();
    }

private:
    /**
     * @brief Refill Tokens.
     * @details Calls: std::chrono::steady_clock::now(), load(), count(), std::min(), compare_exchange_strong().
     */
    void refillTokens() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_refill_time_.load()
        );
        
        if (elapsed.count() >= 1000) {  // Refill every second
            uint32_t current = tokens_.load();
            uint32_t new_tokens = std::min(
                current + config_.max_requests_per_second,
                config_.burst_size
            );
            
            if (tokens_.compare_exchange_strong(current, new_tokens)) {
                last_refill_time_ = now;
            }
        }
    }
    
    Config config_;
    std::atomic<uint32_t> tokens_;
    std::atomic<std::chrono::steady_clock::time_point> last_refill_time_;
};

} // namespace cache
} // namespace themis
