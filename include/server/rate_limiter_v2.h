/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rate_limiter_v2.h                                  ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:26:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     336                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea7db4d78f  2026-03-11  feat(server): Redis backend for RateLimiterV2 (distribute... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
#include <functional>

#ifdef THEMIS_ENABLE_REDIS
#include <hiredis/hiredis.h>
#endif

namespace themis {
namespace server {

/**
 * @brief Redis connection configuration for distributed rate limiting.
 *
 * Used by TokenBucketRateLimiter and PerClientRateLimiter when
 * Backend::REDIS is selected.  When THEMIS_ENABLE_REDIS is not defined,
 * the struct is still present but the Redis code path is compiled out;
 * the limiter silently falls back to the local token bucket and emits a
 * one-time WARN log.
 */
struct RedisRateLimiterConfig {
    /// Redis server hostname or IP address.
    std::string host = "127.0.0.1";

    /// Redis server TCP port.
    int port = 6379;

    /// Optional AUTH password.  Empty = no authentication.
    std::string auth;

    /// Key namespace prefix, e.g. "themis:rl".
    /// The full key becomes "{key_prefix}:{client_key}:{priority}".
    std::string key_prefix = "themis:rl";

    /// Connect / command timeout in milliseconds.
    int timeout_ms = 5000;

    /// Maximum number of consecutive Redis errors before declaring the
    /// backend unhealthy and triggering the local fallback.
    int max_errors = 3;

    /// TTL (in seconds) for rate-limit keys stored in Redis.
    /// Should be at least capacity / refill_rate seconds.
    int key_ttl_seconds = 3600;
};

/**
 * @brief Token-Bucket Rate Limiter with priority lanes
 * 
 * Enterprise-grade rate limiting using the Token Bucket algorithm:
 * - Burst traffic support (capacity > refill_rate)
 * - Priority lanes (HIGH/NORMAL/LOW)
 * - Per-client rate limiting
 * - Thread-safe
 * - Optional Redis backend for cluster-wide distributed rate limiting
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
 *
 * Redis backend example:
 *   TokenBucketRateLimiter::Config cfg;
 *   cfg.backend = TokenBucketRateLimiter::Backend::REDIS;
 *   cfg.redis.host = "redis.internal";
 *   TokenBucketRateLimiter limiter(cfg);
 */
class TokenBucketRateLimiter {
public:
    enum class Priority {
        HIGH = 0,   // VIP clients (e.g., via JWT claim "premium": true)
        NORMAL = 1, // Standard clients
        LOW = 2     // Batch/background jobs
    };

    /// Selects where the bucket state is stored.
    enum class Backend {
        LOCAL, ///< In-process token bucket (default, original behaviour).
        REDIS  ///< Distributed token bucket backed by Redis EVALSHA.
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

        /// Storage backend.  Defaults to LOCAL for backwards compatibility.
        Backend backend = Backend::LOCAL;

        /// Redis configuration (only used when backend == Backend::REDIS).
        RedisRateLimiterConfig redis;

        /// Identifies this bucket in Redis keys (e.g., "global" or a route name).
        /// Must be unique across all TokenBucketRateLimiter instances sharing
        /// the same Redis keyspace.
        std::string bucket_id = "default";
    };

    explicit TokenBucketRateLimiter(const Config& config);
    ~TokenBucketRateLimiter();

    // Non-copyable; move is disabled to prevent double-free of redis_ctx_.
    TokenBucketRateLimiter(const TokenBucketRateLimiter&) = delete;
    TokenBucketRateLimiter& operator=(const TokenBucketRateLimiter&) = delete;
    TokenBucketRateLimiter(TokenBucketRateLimiter&&) = delete;
    TokenBucketRateLimiter& operator=(TokenBucketRateLimiter&&) = delete;

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
     * @brief Returns true when the Redis backend is configured and currently
     *        reachable; false when running in local-fallback mode.
     */
    bool isRedisHealthy() const;

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

    // ---- Redis helpers (compiled out when THEMIS_ENABLE_REDIS is not set) ----

    /// Build the Redis key for a given priority lane.
    std::string redisKey(const std::string& bucket_id, Priority prio) const;

    /// Ensure the publish/command connection is open; returns false on failure.
    bool redisConnect();

    /// Execute the EVALSHA token-bucket Lua script on Redis.
    /// Returns -1 on Redis error (triggers local fallback), 1 if allowed, 0 if rejected.
    int redisEvalBucket(Priority prio, size_t capacity, size_t refill_rate,
                        size_t consume_count);

    /// Execute the EVALSHA command with the current redis_ctx_ (must hold redis_mutex_).
    /// Returns -1 on error, 1 if allowed, 0 if rejected.
    int redisExecEvalsha(const std::string& key, size_t capacity, size_t refill_rate,
                         size_t consume_count);

    /// Mark Redis as unhealthy; increments error counter and, if max_errors
    /// reached, sets redis_healthy_ = false and emits a WARN log.
    void markRedisError();

    /// Perform a probe to see whether Redis has recovered; if so, resets the
    /// error counter and sets redis_healthy_ = true.
    void tryRedisRecover();

    // ---- Local (in-process) helpers ----
    bool localTryAcquire(size_t tokens, Priority prio);
    size_t localAvailableTokens(Priority prio) const;

    Config config_;
    std::unordered_map<Priority, std::unique_ptr<Bucket>> buckets_;
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> total_rejections_{0};

#ifdef THEMIS_ENABLE_REDIS
    mutable std::mutex redis_mutex_;
    redisContext* redis_ctx_{nullptr};
    std::string evalsha_;          ///< SHA1 of the loaded Lua script.
    bool script_loaded_{false};
#endif
    std::atomic<bool> redis_healthy_{false};
    std::atomic<int>  redis_errors_{0};
};

/**
 * @brief Per-client rate limiter (uses client_id as key)
 * 
 * Usage:
 *   auto limiter = std::make_shared<PerClientRateLimiter>();
 *   if (!limiter->allowRequest(client_id)) {
 *     return HTTP 429;
 *   }
 *
 * Redis backend usage:
 *   PerClientRateLimiter::Config cfg;
 *   cfg.backend = TokenBucketRateLimiter::Backend::REDIS;
 *   cfg.redis.host = "redis.internal";
 *   auto limiter = std::make_shared<PerClientRateLimiter>(cfg);
 */
class PerClientRateLimiter {
public:
    struct Config {
        size_t capacity_per_client;
        size_t refill_rate_per_client;
        size_t max_clients;
        std::chrono::minutes cleanup_interval;

        /// Storage backend forwarded to each per-client TokenBucketRateLimiter.
        TokenBucketRateLimiter::Backend backend = TokenBucketRateLimiter::Backend::LOCAL;

        /// Redis configuration (only used when backend == Backend::REDIS).
        RedisRateLimiterConfig redis;
        
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
