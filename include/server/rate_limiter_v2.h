/**
 * @file rate_limiter_v2.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>
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

    /// F-008: Number of pooled Redis connections.  Each connection can
    /// execute one EVALSHA call concurrently, so pool_size determines the
    /// maximum concurrency of distributed rate-limit checks.
    /// Minimum: 1.  Recommended: number of worker threads / 4.
    int pool_size = 4;
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
     * @brief Try to acquire tokens from the token bucket.
     * 
     * Attempts to consume the requested number of tokens from the appropriate priority lane.
     * If not enough tokens are available, the request is rejected immediately (fail-fast).
     * This method is called for every incoming request as part of rate-limit enforcement.
     * 
     * ### Priority Lanes
     * - HIGH: VIP clients with higher burst and sustained rate limits
     * - NORMAL: Standard clients with default limits
     * - LOW: Batch/background jobs with lower limits
     * 
     * ### Token Bucket Algorithm
     * 1. Calculate refill amount based on time elapsed since last refill
     * 2. Add refilled tokens to current bucket (capped at capacity)
     * 3. If bucket has enough tokens, consume and return true
     * 4. Otherwise, return false (rate limit exceeded)
     * 
     * ### Distributed Mode (Redis backend)
     * When Backend::REDIS is configured, the token bucket state is stored in Redis
     * and shared across all server instances. This ensures consistent rate limiting
     * across the entire cluster. If Redis becomes unhealthy, the local bucket is used
     * as fallback.
     * 
     * @param tokens Number of tokens to consume (default: 1).
     *               A value of 0 is treated as a no-op acquisition and succeeds.
     * @param prio Priority lane to use (HIGH, NORMAL, LOW)
     * 
     * @return true if tokens were successfully acquired and consumed; false if rate limit exceeded
     *         (bucket did not have enough tokens)
     * 
     * @note Thread-safe; concurrent calls allowed on the same limiter
     * @note Time-based refills are processed only when tryAcquire is called
     * @note In distributed mode (Redis), Redis errors trigger local fallback; see isRedisHealthy()
     * @note Does NOT block; returns immediately with success/failure status
     * 
     * @see getAvailableTokens() to query current token count without consuming
     * @see getTotalRequests() and getTotalRejections() for metrics
     * @see isRedisHealthy() to check distributed backend status
     */
    bool tryAcquire(size_t tokens = 1, Priority prio = Priority::NORMAL);

    /**
     * @brief Query the current number of available tokens without consuming any.
     * 
     * Useful for monitoring and debugging rate-limiter state.
     * 
     * @param prio Priority lane to query (HIGH, NORMAL, LOW)
     * 
     * @return Current number of tokens in the specified priority lane bucket
     * 
     * @note Thread-safe; returns snapshot of current state
     * @note Does NOT consume tokens; subsequent tryAcquire() calls are unaffected
     * @note Returns local in-process bucket counters only; this call does not query Redis
     * @note Does not trigger refill; values may lag until tryAcquire() or explicit refill paths run
     * 
     * @see tryAcquire() to consume tokens and enforce the limit
     */
    size_t getAvailableTokens(Priority prio = Priority::NORMAL) const;

    /**
     * @brief Get total number of requests that attempted token acquisition.
     * 
     * Includes both successful acquisitions and rejections.
     * Useful for calculating rejection rate and monitoring traffic volume.
     * 
     * @return Cumulative count of all tryAcquire() calls
     * 
     * @note Thread-safe; returns snapshot
     * @note Covers both successful and failed acquisitions; see getTotalRejections() for failures only
     * 
     * @see getTotalRejections() for count of rate-limited requests
     */
    uint64_t getTotalRequests() const { return total_requests_.load(); }

    /**
     * @brief Get total number of rate-limited requests (failed token acquisitions).
     * 
     * Useful for calculating rejection rate and monitoring rate-limit effectiveness.
     * Rejection rate = getTotalRejections() / getTotalRequests()
     * 
     * @return Cumulative count of all tryAcquire() calls that returned false
     * 
     * @note Thread-safe; returns snapshot
     * @note Only counts failed attempts; successful acquisitions are not counted here
     * 
     * @see getTotalRequests() for total attempt count (successful + rejected)
     */
    uint64_t getTotalRejections() const { return total_rejections_.load(); }

    /**
     * @brief Check if the distributed Redis backend is currently healthy and reachable.
     * 
     * Returns true only when:
     * - Backend::REDIS is configured
     * - Redis connection is established
     * - Recent EVALSHA calls have succeeded
     * 
     * Returns false when:
     * - Backend::LOCAL is configured (no Redis)
     * - Redis connection failed or timed out
     * - Too many consecutive Redis errors (max_errors threshold exceeded)
     * 
     * When Redis becomes unhealthy, the rate limiter automatically falls back to
     * local token bucket mode. When Redis recovers, health checks resume periodically.
     * 
     * @return true if Redis backend is healthy; false if unavailable or not configured
     * 
     * @note Thread-safe; returns snapshot
     * @note Even when isRedisHealthy() returns false, rate limiting continues using local fallback
     * @note This is informational; rate limiter behavior is unaffected by return value
     * 
     * @see Backend enum for configuration options
     */
    bool isRedisHealthy() const;

    /**
     * @brief Reset all counters and token buckets to initial state.
     * 
     * Useful for testing and performance profiling. Resets:
     * - Total requests counter
     * - Total rejections counter
     * - Token bucket levels to capacity for each priority lane
     * 
     * @note Thread-safe; safe to call while other threads are using tryAcquire()
     * @note Does NOT change configuration; only affects counters and token state
     * @note Does NOT reset Redis backend (if configured); only affects local state
     * @note Intended for testing only; avoid in production
     * 
     * @see Config for initial configuration
     */
    void reset();

    /**
     * @brief OP-HEALTH-001: Check if rate limiter is healthy and operational.
     * 
     * Returns true if the rate limiter can process requests without degradation:
     * - Local backend: always true (cannot fail)
     * - Redis backend: true if Redis is reachable and responding
     * 
     * @return true if healthy; false if degraded
     * @note Thread-safe atomic read
     * @note Operational observability; informs /health probes
     */
    bool isHealthy() const;

    /**
     * @brief OP-HEALTH-002: Get detailed health status as JSON (readiness probe).
     * 
     * Returns operational metrics suitable for /health/{module} endpoint:
     * - status: "healthy" or "unhealthy"
     * - total_requests: cumulative request count
     * - total_rejections: cumulative rejection count
     * - backend: "redis" or "local"
     * - redis_healthy: Redis backend status
     * - timeout_count: deadlines exceeded
     * 
     * @return JSON string with health metrics
     * @note Thread-safe; uses atomic reads
     * @note Suitable for Prometheus scraping or health endpoint
     */
    std::string getHealthStatus() const;

private:
    struct Bucket {
        std::atomic<size_t> tokens;
        size_t capacity = {};
        size_t refill_rate = {};
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

    /// Initialise all pool slots (connect + load Lua script).  Returns false
    /// if no slot could be connected; redis_healthy_ is set accordingly.
    bool redisConnect();

    /// Execute the EVALSHA token-bucket Lua script on Redis.
    /// Borrows a connection from the pool, executes, and returns it.
    /// Returns -1 on Redis error (triggers local fallback), 1 if allowed, 0 if rejected.
    int redisEvalBucket(Priority prio, size_t capacity, size_t refill_rate,
                        size_t consume_count);

#ifdef THEMIS_ENABLE_REDIS
    /// Execute EVALSHA on a borrowed slot (caller holds the slot exclusively).
    /// Returns -1 on error, 1 if allowed, 0 if rejected.
    int redisExecEvalsha(RedisConnectionPool::Slot& slot,
                         const std::string& key, size_t capacity,
                         size_t refill_rate, size_t consume_count);
#endif

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
    
    // OP-TIMEOUT-001: Track deadlines exceeded (fail-safe timeouts)
    std::atomic<uint64_t> timeout_count_{0};
    
    // OP-LATENCY-001/002: Track latency metrics (thread-safe atomic counters)
    std::atomic<uint64_t> latency_redis_sum_{0};    // Redis path latency count
    std::atomic<uint64_t> latency_local_sum_{0};    // Local path latency count

#ifdef THEMIS_ENABLE_REDIS
    // F-008: Connection pool — one mutex/cv guards the pool of redisContext* slots.
    // Each slot can be borrowed by one thread at a time; concurrent EVALSHA calls
    // are dispatched from separate connections instead of serialising on a single one.
    struct RedisConnectionPool {
        struct Slot {
            redisContext* ctx{nullptr};
            std::string   evalsha;       ///< SHA1 loaded on this connection.
            bool          script_loaded{false};
        };
        std::vector<Slot>               slots;
        std::deque<size_t>              available; ///< Indices of idle slots.
        mutable std::mutex              pool_mu;
        std::condition_variable         pool_cv;
    };
    mutable RedisConnectionPool redis_pool_;
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
        size_t capacity_per_client = 0;
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
