/**
 * @file rate_limiter_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class IRateLimiterBackend {
public:
    /**
     * @brief IRate Limiter Backend.
     * @return Return value.
     */
    virtual ~IRateLimiterBackend() = default;

    static constexpr int64_t kBackendUnavailable = INT64_MAX;

    [[nodiscard]] virtual int64_t increment(const std::string& key, uint32_t window_seconds) = 0;

    [[nodiscard]] virtual int64_t getCount(const std::string& key, uint32_t window_seconds) const = 0;

    /**
     * @brief Reset the modification detection flag.
     * @param[in] key Input parameter.
     */
    virtual void reset(const std::string& key) = 0;
};

// ============================================================================
// InMemoryRateLimiterBackend — in-process sliding-window counter
// ============================================================================

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

class RedisRateLimiterBackend final : public IRateLimiterBackend {
public:
    using IncrementFn = std::function<int64_t(const std::string&, uint32_t)>;
    using GetCountFn = std::function<int64_t(const std::string&, uint32_t)>;
    using ResetFn = std::function<void(const std::string&)>;
    using IsConnectedFn = std::function<bool()>;
    using ReconnectFn = std::function<bool()>;

    struct Config {
        std::string host = "127.0.0.1";
        int port = 6379;
        std::string auth;
        std::string key_prefix = "themis:rl:";
        int connect_timeout_ms = 200;

        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };

    explicit RedisRateLimiterBackend(const Config& config = Config::defaults());
    ~RedisRateLimiterBackend() override;

    RedisRateLimiterBackend(const RedisRateLimiterBackend&) = delete;
    RedisRateLimiterBackend& operator=(const RedisRateLimiterBackend&) = delete;

    // -----------------------------------------------------------------------
    // IRateLimiterBackend interface
    // -----------------------------------------------------------------------

    int64_t increment(const std::string& key, uint32_t window_seconds) override;

    int64_t getCount(const std::string& key, uint32_t window_seconds) const override;

    void reset(const std::string& key) override;

    // -----------------------------------------------------------------------
    // Connectivity
    // -----------------------------------------------------------------------

    /**
     * @brief Is Connected.
     * @return True when the operation succeeds.
     */
    bool isConnected() const;

    /**
     * @brief Reconnect.
     * @return True when the operation succeeds.
     */
    bool reconnect();

    /**
     * @brief Set Increment Fn.
     * @param[in] fn Input parameter.
     */
    static void setIncrementFn(IncrementFn fn);
    /**
     * @brief Set Get Count Fn.
     * @param[in] fn Input parameter.
     */
    static void setGetCountFn(GetCountFn fn);
    /**
     * @brief Set Reset Fn.
     * @param[in] fn Input parameter.
     */
    static void setResetFn(ResetFn fn);
    /**
     * @brief Set Is Connected Fn.
     * @param[in] fn Input parameter.
     */
    static void setIsConnectedFn(IsConnectedFn fn);
    /**
     * @brief Set Reconnect Fn.
     * @param[in] fn Input parameter.
     */
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

    /**
     * @brief Connect.
     * @return True when the operation succeeds.
     */
    bool        connect();
    /**
     * @brief Disconnect.
     */
    void        disconnect();
    /**
     * @brief Make Key.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::string makeKey(const std::string& key) const;

    // Global counter appended to each sorted-set member to guarantee uniqueness
    // even when two requests arrive within the same microsecond on the same node.
    static std::atomic<uint64_t> member_counter_;
#endif
};

} // namespace auth
} // namespace themis

