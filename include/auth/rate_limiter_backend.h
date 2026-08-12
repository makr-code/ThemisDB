/**
 * @file rate_limiter_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <cstdint>
#include <atomic>

// Forward-declare to avoid pulling in hiredis from this header.
struct redisContext;

namespace themis {
namespace auth {

// ============================================================================
// IRateLimiterBackend — abstract counter-storage interface
// ============================================================================

/**
 * @brief Abstract backend interface for distributed rate-limiter counter storage.
 *
 * Provides the storage layer for sliding-window request counters used by
 * AuthRateLimiter.  Two built-in implementations are provided:
 *
 *   - InMemoryRateLimiterBackend : thread-safe in-process counters
 *                                   (default; single-node deployments)
 *   - RedisRateLimiterBackend    : Redis-backed shared counters
 *                                   (multi-node deployments; avoids horizontal bypass)
 *
 * Custom implementations (e.g. Memcached, etcd) can be injected via
 * AuthRateLimiter::setBackend().
 *
 * All methods must be thread-safe.
 */
class IRateLimiterBackend {
public:
    virtual ~IRateLimiterBackend() = default;

    /**
     * @brief Sentinel value returned by increment() and getCount() when the
     *        backend is unavailable (e.g. Redis not reachable).
     *
     * Callers MUST check for this value to distinguish "backend unavailable"
     * (fail-closed: treat every call as over-limit) from a legitimately high
     * but finite count.  All concrete implementations that operate in a
     * degraded mode MUST return this constant rather than an ad-hoc value.
     */
    static constexpr int64_t kBackendUnavailable = INT64_MAX;

    /**
     * @brief Atomically record a new request for the given key and return
     *        the total request count within the sliding window.
     *
     * @param key            Opaque counter key (e.g. "ip:1.2.3.4", "user:alice").
     * @param window_seconds Sliding-window duration in seconds.
     * @return               New request count (including this request) within the window.
     */
    [[nodiscard]] virtual int64_t increment(const std::string& key, uint32_t window_seconds) = 0;

    /**
     * @brief Return the current request count for the key without modifying state.
     *
     * @param key            Counter key.
     * @param window_seconds Sliding-window duration in seconds.
     * @return               Current count within the window (0 if key unknown).
     */
    [[nodiscard]] virtual int64_t getCount(const std::string& key, uint32_t window_seconds) const = 0;

    /**
     * @brief Clear all recorded requests for the given key.
     *
     * @param key Counter key to reset.
     */
    virtual void reset(const std::string& key) = 0;
};

// ============================================================================
// InMemoryRateLimiterBackend — in-process sliding-window counter
// ============================================================================

/**
 * @brief Thread-safe in-process sliding-window counter backend.
 *
 * Stores per-key request timestamps in an unordered_map protected by a mutex.
 * Suitable for single-node deployments.
 *
 * Two AuthRateLimiter instances that share the same InMemoryRateLimiterBackend
 * instance will observe each other's request counts, making this also useful
 * for testing distributed behaviour without a real Redis server.
 */
class InMemoryRateLimiterBackend final : public IRateLimiterBackend {
public:
    InMemoryRateLimiterBackend() = default;
    ~InMemoryRateLimiterBackend() override = default;

    InMemoryRateLimiterBackend(const InMemoryRateLimiterBackend&) = delete;
    InMemoryRateLimiterBackend& operator=(const InMemoryRateLimiterBackend&) = delete;

    int64_t increment(const std::string& key, uint32_t window_seconds) override;
    int64_t getCount(const std::string& key, uint32_t window_seconds) const override;
    void    reset(const std::string& key) override;

private:
    using TimePoint = std::chrono::steady_clock::time_point;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<TimePoint>> counters_;
};

// ============================================================================
// RedisRateLimiterBackend — Redis-backed sliding-window counter
// ============================================================================

/**
 * @brief Redis-backed sliding-window counter backend for distributed deployments.
 *
 * Uses a single Lua script executed atomically on the Redis server to maintain
 * a sorted-set sliding window per key, avoiding any TOCTOU race:
 *
 *   1. ZREMRANGEBYSCORE <key> -inf <window_start_us>   -- prune expired entries
 *   2. ZADD <key> <now_us> <unique_member>             -- record this request
 *   3. EXPIRE <key> <window_seconds + 1>               -- bound storage lifetime
 *   4. return ZCARD <key>                              -- count in window
 *
 * Because Redis executes Lua scripts atomically (single-threaded), step 1-4 form
 * an indivisible unit — no concurrent request can observe a partial state.
 *
 * When built without hiredis (THEMIS_ENABLE_REDIS not defined), all methods
 * compile and link and transparently use a process-local in-memory fallback.
 * This preserves single-process rate limiting but does not synchronize counters
 * across replicas.
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Performance target: increment() ≤ 2 ms P99 on a local-network Redis instance.
 */
class RedisRateLimiterBackend final : public IRateLimiterBackend {
public:
    using IncrementFn = std::function<int64_t(const std::string&, uint32_t)>;
    using GetCountFn = std::function<int64_t(const std::string&, uint32_t)>;
    using ResetFn = std::function<void(const std::string&)>;
    using IsConnectedFn = std::function<bool()>;
    using ReconnectFn = std::function<bool()>;

    struct Config {
        /// Redis server hostname or IP address.
        std::string host = "127.0.0.1";
        /// Redis server port.
        int port = 6379;
        /// Optional AUTH password (empty = no authentication).
        std::string auth;
        /// Key prefix applied to every counter key stored in Redis.
        std::string key_prefix = "themis:rl:";
        /// Connection timeout in milliseconds.
        int connect_timeout_ms = 200;

        static Config defaults() { return {}; }
    };

    /**
     * @brief Construct and eagerly connect to Redis.
     *
     * If the connection fails a warning is logged and the instance operates as
     * a no-op (fail-open) until reconnect() succeeds.
     */
    explicit RedisRateLimiterBackend(const Config& config = Config::defaults());
    ~RedisRateLimiterBackend() override;

    RedisRateLimiterBackend(const RedisRateLimiterBackend&) = delete;
    RedisRateLimiterBackend& operator=(const RedisRateLimiterBackend&) = delete;

    // -----------------------------------------------------------------------
    // IRateLimiterBackend interface
    // -----------------------------------------------------------------------

    /**
     * @brief Atomically increment the sliding-window counter via a Lua script.
     *
     * Returns a process-local fallback count when Redis support is not compiled
     * in, so that non-Redis builds still enforce local rate limits.
     */
    int64_t increment(const std::string& key, uint32_t window_seconds) override;

    /**
     * @brief Return current count via a read-only Lua script (no side-effects).
     *
     * Returns a process-local fallback count when Redis support is not compiled in.
     */
    int64_t getCount(const std::string& key, uint32_t window_seconds) const override;

    /**
     * @brief Delete the sorted-set key from Redis.
     */
    void reset(const std::string& key) override;

    // -----------------------------------------------------------------------
    // Connectivity
    // -----------------------------------------------------------------------

    /** @return true if the Redis connection is currently alive. */
    bool isConnected() const;

    /**
     * @brief Attempt to (re)connect to Redis.
     * @return true on success.
     */
    bool reconnect();

    static void setIncrementFn(IncrementFn fn);
    static void setGetCountFn(GetCountFn fn);
    static void setResetFn(ResetFn fn);
    static void setIsConnectedFn(IsConnectedFn fn);
    static void setReconnectFn(ReconnectFn fn);

private:
    Config config_;

    // Lua script: atomic sliding-window increment using a sorted set.
    // KEYS[1] = key, ARGV[1] = now_us, ARGV[2] = window_us,
    // ARGV[3] = window_seconds, ARGV[4] = unique_member
    static constexpr const char* kIncrScript =
        "local key = KEYS[1]\n"
        "local now_us = tonumber(ARGV[1])\n"
        "local window_us = tonumber(ARGV[2])\n"
        "local window_seconds = tonumber(ARGV[3])\n"
        "local member = ARGV[4]\n"
        "redis.call('ZREMRANGEBYSCORE', key, '-inf', now_us - window_us)\n"
        "redis.call('ZADD', key, now_us, member)\n"
        "redis.call('EXPIRE', key, window_seconds + 1)\n"
        "return redis.call('ZCARD', key)\n";

    // Lua script: read-only count (prunes stale entries, no new record added).
    // KEYS[1] = key, ARGV[1] = now_us, ARGV[2] = window_us
    static constexpr const char* kCountScript =
        "local key = KEYS[1]\n"
        "local now_us = tonumber(ARGV[1])\n"
        "local window_us = tonumber(ARGV[2])\n"
        "redis.call('ZREMRANGEBYSCORE', key, '-inf', now_us - window_us)\n"
        "return redis.call('ZCARD', key)\n";

#ifdef THEMIS_ENABLE_REDIS
    mutable std::mutex mutex_;
    redisContext*      ctx_{nullptr};

    bool        connect();
    void        disconnect();
    std::string makeKey(const std::string& key) const;

    // Global counter appended to each sorted-set member to guarantee uniqueness
    // even when two requests arrive within the same microsecond on the same node.
    static std::atomic<uint64_t> member_counter_;
#endif
};

} // namespace auth
} // namespace themis

