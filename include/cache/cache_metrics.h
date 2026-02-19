#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace cache {

/**
 * @brief Enhanced cache metrics for observability and monitoring
 * 
 * Thread-safe metrics collection for cache operations.
 * Supports Prometheus-style metrics export.
 */
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
    
    /**
     * @brief Calculate overall hit rate
     */
    double getHitRate() const {
        uint64_t total = l1_hits.load() + l2_hits.load() + l3_hits.load() + misses.load();
        return total > 0 ? static_cast<double>(l1_hits + l2_hits + l3_hits) / total : 0.0;
    }
    
    /**
     * @brief Calculate L1 hit rate
     */
    double getL1HitRate() const {
        uint64_t total = l1_hits.load() + l2_hits.load() + l3_hits.load() + misses.load();
        return total > 0 ? static_cast<double>(l1_hits) / total : 0.0;
    }
    
    /**
     * @brief Calculate compression ratio
     */
    double getCompressionRatio() const {
        uint64_t cached = total_bytes_cached.load();
        uint64_t compressed = total_bytes_compressed.load();
        return cached > 0 ? static_cast<double>(cached) / compressed : 1.0;
    }
    
    /**
     * @brief Export metrics as JSON
     */
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
        
        return j;
    }
    
    /**
     * @brief Reset all metrics
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
    }
};

/**
 * @brief Circuit breaker for fault isolation
 * 
 * Implements the circuit breaker pattern to prevent cascading failures.
 * States: CLOSED (normal) -> OPEN (failing) -> HALF_OPEN (testing)
 */
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
    };
    
    explicit CircuitBreaker(const Config& config = Config())
        : config_(config)
        , state_(State::CLOSED)
        , failure_count_(0)
        , success_count_(0)
        , last_failure_time_(std::chrono::steady_clock::time_point::min())
        , half_open_calls_(0) {}
    
    /**
     * @brief Check if operation should be allowed
     */
    bool allowRequest() {
        auto now = std::chrono::steady_clock::now();
        
        if (state_ == State::OPEN) {
            // Check if timeout expired
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_failure_time_
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
     * @brief Record successful operation
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
     * @brief Record failed operation
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
    
    /**
     * @brief Get current state
     */
    State getState() const {
        return state_;
    }
    
    /**
     * @brief Check if circuit breaker is open
     */
    bool isOpen() const {
        return state_ == State::OPEN;
    }
    
    /**
     * @brief Reset circuit breaker to closed state
     */
    void reset() {
        state_ = State::CLOSED;
        failure_count_ = 0;
        success_count_ = 0;
        half_open_calls_ = 0;
    }

private:
    Config config_;
    std::atomic<State> state_;
    std::atomic<uint32_t> failure_count_;
    std::atomic<uint32_t> success_count_;
    std::atomic<std::chrono::steady_clock::time_point> last_failure_time_;
    std::atomic<uint32_t> half_open_calls_;
};

} // namespace cache
} // namespace themis
