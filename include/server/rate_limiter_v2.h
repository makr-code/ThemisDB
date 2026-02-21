/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rate_limiter_v2.h                                  ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:43:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     218                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <string>
#include <memory>

namespace themis {
namespace server {

/**
 * @brief Token-Bucket Rate Limiter with priority lanes
 * 
 * Enterprise-grade rate limiting using the Token Bucket algorithm:
 * - Burst traffic support (capacity > refill_rate)
 * - Priority lanes (HIGH/NORMAL/LOW)
 * - Per-client rate limiting
 * - Thread-safe
 * 
 * Algorithm:
 * 1. Bucket holds 'capacity' tokens
 * 2. Refills at 'refill_rate' tokens/second
 * 3. Request consumes 1 token (customizable)
 * 4. If bucket empty → reject (429)
 * 
 * Example:
 *   capacity=1000, refill_rate=100
 *   → Can handle 1000 req/s burst, 100 req/s sustained
 */
class TokenBucketRateLimiter {
public:
    enum class Priority {
        HIGH = 0,   // VIP clients (e.g., via JWT claim "premium": true)
        NORMAL = 1, // Standard clients
        LOW = 2     // Batch/background jobs
    };

    struct Config {
        size_t capacity = 1000;              // Max tokens (burst capacity)
        size_t refill_rate = 100;            // Tokens per second
        bool enable_priority_lanes = true;  // Separate buckets per priority
        
        // Priority-specific configs (optional)
        size_t high_capacity = 2000;         // VIP burst
        size_t high_refill_rate = 200;       // VIP sustained
        size_t low_capacity = 500;           // Low-priority burst
        size_t low_refill_rate = 50;         // Low-priority sustained
    };

    explicit TokenBucketRateLimiter(const Config& config);
    ~TokenBucketRateLimiter() = default;

    // Disable copy, allow move
    TokenBucketRateLimiter(const TokenBucketRateLimiter&) = delete;
    TokenBucketRateLimiter& operator=(const TokenBucketRateLimiter&) = delete;
    TokenBucketRateLimiter(TokenBucketRateLimiter&&) = default;
    TokenBucketRateLimiter& operator=(TokenBucketRateLimiter&&) = default;

    /**
     * @brief Try to acquire tokens from the bucket
     * 
     * @param tokens Number of tokens to consume (default: 1)
     * @param prio Priority lane to use
     * @return true if tokens acquired, false if bucket empty (rate limit exceeded)
     */
    bool tryAcquire(size_t tokens = 1, Priority prio = Priority::NORMAL);

    /**
     * @brief Get current token count (for monitoring)
     */
    size_t getAvailableTokens(Priority prio = Priority::NORMAL) const;

    /**
     * @brief Get total requests handled (metrics)
     */
    uint64_t getTotalRequests() const { return total_requests_.load(); }

    /**
     * @brief Get total rejections (metrics)
     */
    uint64_t getTotalRejections() const { return total_rejections_.load(); }

    /**
     * @brief Reset counters (for testing)
     */
    void reset();

private:
    struct Bucket {
        std::atomic<size_t> tokens;
        size_t capacity;
        size_t refill_rate;
        std::chrono::steady_clock::time_point last_refill;
        mutable std::mutex mutex;

        Bucket(size_t cap, size_t rate)
            : tokens(cap)
            , capacity(cap)
            , refill_rate(rate)
            , last_refill(std::chrono::steady_clock::now())
        {}

        void refill();
        bool consume(size_t count);
    };

    Config config_;
    std::unordered_map<Priority, std::unique_ptr<Bucket>> buckets_;
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> total_rejections_{0};
};

/**
 * @brief Per-client rate limiter (uses client_id as key)
 * 
 * Usage:
 *   auto limiter = std::make_shared<PerClientRateLimiter>();
 *   if (!limiter->allowRequest(client_id)) {
 *     return HTTP 429;
 *   }
 */
class PerClientRateLimiter {
public:
    struct Config {
        size_t capacity_per_client;
        size_t refill_rate_per_client;
        size_t max_clients;
        std::chrono::minutes cleanup_interval;
        
        // Default constructor with values
        Config() 
            : capacity_per_client(100)
            , refill_rate_per_client(10)
            , max_clients(10000)
            , cleanup_interval(5) {}
    };

    PerClientRateLimiter();
    explicit PerClientRateLimiter(const Config& config);

    /**
     * @brief Check if request from client is allowed
     * 
     * @param client_id Client identifier (e.g., API key, IP, user_id)
     * @param tokens Number of tokens to consume
     * @param prio Priority for this client
     * @return true if allowed, false if rate limited
     */
    bool allowRequest(
        const std::string& client_id,
        size_t tokens = 1,
        TokenBucketRateLimiter::Priority prio = TokenBucketRateLimiter::Priority::NORMAL
    );

    /**
     * @brief Get metrics for a specific client
     */
    struct ClientMetrics {
        uint64_t total_requests = 0;
        uint64_t total_rejections = 0;
        size_t available_tokens = 0;
    };
    ClientMetrics getClientMetrics(const std::string& client_id) const;

    /**
     * @brief Get total number of tracked clients
     */
    size_t getActiveClients() const;

    /**
     * @brief Manually cleanup idle clients (automatic via background thread)
     */
    void cleanupIdleClients();

private:
    struct ClientBucket {
        std::unique_ptr<TokenBucketRateLimiter> limiter;
        std::chrono::steady_clock::time_point last_access;
        std::atomic<uint64_t> total_requests{0};
        std::atomic<uint64_t> total_rejections{0};
    };

    Config config_;
    mutable std::mutex clients_mutex_;
    std::unordered_map<std::string, std::unique_ptr<ClientBucket>> client_buckets_;
    std::chrono::steady_clock::time_point last_cleanup_;
};

} // namespace server
} // namespace themis
