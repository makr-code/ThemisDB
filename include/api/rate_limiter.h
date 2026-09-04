/**
 * @file rate_limiter.h
 * @brief Token bucket rate limiting for GraphQL and API endpoints.
 *
 * @details Implements the token bucket algorithm to enforce per-key quotas
 * for API requests, preventing resource exhaustion and managing traffic burst patterns.
 *
 * Core components:
 *  - `RateLimiter::Config`: Configurable capacity, refill rate, and time window
 *  - `RateLimiter::Bucket`: Per-key token state with automatic expiration
 *  - `RateLimiter::allow()`: Check and consume quota for a given key
 *
 * Algorithm:
 *  - Each key maintains a floating-point token count (initially = capacity)
 *  - On each request, tokens are refilled based on elapsed time and refill_rate
 *  - If available tokens ≥ requested cost, the request is allowed and tokens are deducted
 *  - Idle buckets (fully recharged, no activity for 2× time window) are evicted automatically
 *
 * Quota semantics:
 *  - Capacity = maximum burst size (e.g., 100 requests)
 *  - Refill rate = tokens/second (e.g., 10 tokens/sec = 10 req/s average)
 *  - Variable cost = number of tokens consumed per request (default: 1)
 *
 * ### Thread safety
 * - All public methods are thread-safe via internal mutex
 * - Statistics (allowed_requests, rejected_requests) are updated atomically
 * - Safe for concurrent calls from multiple threads
 *
 * ### Usage
 * ```cpp
 * RateLimiter limiter(RateLimiter::Config::defaults());
 * if (limiter.allow(user_id, 1)) {
 *     // Process request
 * } else {
 *     // Return ERR_API_RATE_LIMIT (429 Too Many Requests)
 * }
 * ```
 *
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 */


#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <atomic>

namespace themis {
namespace graphql {

/**
 * @brief Token Bucket Rate Limiter
 * 
 * Implements the token bucket algorithm for rate limiting.
 * Allows burst traffic while maintaining average rate limits.
 */
class RateLimiter {
public:
    struct Config {
        size_t capacity = 100;              // Maximum tokens (burst size)
        size_t refill_rate = 10;            // Tokens per second
        std::chrono::seconds window = std::chrono::seconds(1);
        
        static Config defaults() {
            return Config{};
        }
        
        static Config strict() {
            return Config{
                .capacity = 10,
                .refill_rate = 1,
                .window = std::chrono::seconds(1)
            };
        }
        
        static Config permissive() {
            return Config{
                .capacity = 1000,
                .refill_rate = 100,
                .window = std::chrono::seconds(1)
            };
        }
    };
    
    struct Bucket {
        double tokens = 0;
        std::chrono::steady_clock::time_point last_refill;
        size_t capacity;
        size_t refill_rate;
        
        Bucket(size_t cap, size_t rate)
            : tokens(static_cast<double>(cap))
            , last_refill(std::chrono::steady_clock::now())
            , capacity(cap)
            , refill_rate(rate)
        {}
        
        void refill(std::chrono::steady_clock::time_point now) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_refill
            ).count();
            
            if (elapsed > 0) {
                double tokens_to_add = (elapsed / 1000.0) * refill_rate;
                tokens = std::min(static_cast<double>(capacity), tokens + tokens_to_add);
                last_refill = now;
            }
        }
        
        bool consume(std::chrono::steady_clock::time_point now, size_t count = 1) {
            refill(now);
            
            if (tokens >= count) {
                tokens -= count;
                return true;
            }
            return false;
        }
        
        size_t available() const {
            return static_cast<size_t>(tokens);
        }
    };
    
    /**
     * @brief Check if request is allowed for a key
     * @param key Rate limit key (e.g., user ID, IP address)
     * @param cost Number of tokens to consume (default: 1)
     * @return true if request is allowed, false if rate limited
     */
    bool allow(const std::string& key, size_t cost = 1) {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        
        // TTL-based eviction: sweep every 64 calls to avoid O(n) on every request.
        // Remove fully-recharged buckets that have been idle for > 2×window.
        if ((++evict_counter_ & 63u) == 0) {
            auto eviction_threshold = config_.window * 2;
            for (auto it = buckets_.begin(); it != buckets_.end(); ) {
                auto idle = std::chrono::duration_cast<std::chrono::seconds>(
                    now - it->second.last_refill
                );
                if (idle >= eviction_threshold &&
                    it->second.tokens >= static_cast<double>(it->second.capacity)) {
                    it = buckets_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        auto it = buckets_.find(key);
        if (it == buckets_.end()) {
            // Create new bucket
            buckets_.emplace(key, Bucket(config_.capacity, config_.refill_rate));
            it = buckets_.find(key);
        }
        
        bool allowed = it->second.consume(now, cost);
        
        if (allowed) {
            stats_.allowed_requests.fetch_add(1, std::memory_order_relaxed);
        } else {
            stats_.rejected_requests.fetch_add(1, std::memory_order_relaxed);
        }
        
        return allowed;
    }
    
    /**
     * @brief Get remaining tokens for a key
     */
    size_t remaining(const std::string& key) {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = buckets_.find(key);
        if (it != buckets_.end()) {
            it->second.refill(now);
            return it->second.available();
        }
        return config_.capacity;
    }
    
    /**
     * @brief Reset rate limit for a key
     */
    void reset(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        buckets_.erase(key);
    }
    
    /**
     * @brief Clear all rate limit state
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        buckets_.clear();
        stats_ = Stats{};
    }
    
    /**
     * @brief Get rate limiter statistics
     */
    struct Stats {
        std::atomic<uint64_t> allowed_requests{0};
        std::atomic<uint64_t> rejected_requests{0};

        Stats() = default;

        Stats(const Stats& other) {
            allowed_requests.store(other.allowed_requests.load());
            rejected_requests.store(other.rejected_requests.load());
        }

        Stats& operator=(const Stats& other) {
            if (this != &other) {
                allowed_requests.store(other.allowed_requests.load());
                rejected_requests.store(other.rejected_requests.load());
            }
            return *this;
        }
        
        uint64_t total() const {
            return allowed_requests.load() + rejected_requests.load();
        }
        
        double rejectRate() const {
            uint64_t t = total();
            return t > 0 ? static_cast<double>(rejected_requests.load()) / t : 0.0;
        }
    };
    
    Stats getStats() const {
        return stats_;
    }
    
    /**
     * @brief Configure rate limiter
     */
    void setConfig(const Config& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }
    
    Config getConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }
    
    /**
     * @brief Create a rate limiter with configuration
     */
    explicit RateLimiter(const Config& config = Config::defaults())
        : config_(config)
    {}
    
private:
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Bucket> buckets_;
    Stats stats_;
    uint64_t evict_counter_{0};
};

/**
 * @brief Rate limit headers helper
 * 
 * Generates standard rate limit headers for HTTP responses.
 */
struct RateLimitHeaders {
    size_t limit = 0;       // X-RateLimit-Limit
    size_t remaining = 0;   // X-RateLimit-Remaining
    std::chrono::seconds reset{0};  // X-RateLimit-Reset
    
    /**
     * @brief Convert to header map
     */
    std::unordered_map<std::string, std::string> toHeaders() const {
        return {
            {"X-RateLimit-Limit", std::to_string(limit)},
            {"X-RateLimit-Remaining", std::to_string(remaining)},
            {"X-RateLimit-Reset", std::to_string(reset.count())}
        };
    }
};

/**
 * @brief Per-operation rate limiting
 * 
 * Different rate limits for different GraphQL operations.
 */
class OperationRateLimiter {
public:
    /**
     * @brief Set rate limit for an operation type
     */
    void setLimit(const std::string& operation_type, const RateLimiter::Config& config) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        
        auto it = limiters_.find(operation_type);
        if (it != limiters_.end()) {
            it->second->setConfig(config);
        } else {
            limiters_[operation_type] = std::make_unique<RateLimiter>(config);
        }
    }
    
    /**
     * @brief Check if request is allowed
     * @param operation_type Query, Mutation, or Subscription
     * @param key Rate limit key (user ID, IP, etc.)
     * @param cost Number of tokens to consume
     * @return true if allowed, false if rate limited
     */
    bool allow(const std::string& operation_type, const std::string& key, size_t cost = 1) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        auto it = limiters_.find(operation_type);
        if (it != limiters_.end()) {
            return it->second->allow(key, cost);
        }
        
        // No rate limit configured for this operation type
        return true;
    }
    
    /**
     * @brief Get remaining tokens for operation and key
     */
    size_t remaining(const std::string& operation_type, const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        auto it = limiters_.find(operation_type);
        if (it != limiters_.end()) {
            return it->second->remaining(key);
        }
        
        return 0;  // Unknown
    }
    
    /**
     * @brief Get rate limit headers for operation
     */
    RateLimitHeaders getHeaders(const std::string& operation_type, const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        RateLimitHeaders headers;
        
        auto it = limiters_.find(operation_type);
        if (it != limiters_.end()) {
            auto config = it->second->getConfig();
            headers.limit = config.capacity;
            headers.remaining = it->second->remaining(key);
            headers.reset = config.window;
        }
        
        return headers;
    }
    
    /**
     * @brief Clear all rate limit state
     */
    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        limiters_.clear();
    }
    
    /**
     * @brief Singleton instance
     */
    static OperationRateLimiter& instance() {
        static OperationRateLimiter instance;
        return instance;
    }
    
private:
    OperationRateLimiter() = default;
    
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<RateLimiter>> limiters_;
};

} // namespace graphql
} // namespace themis
