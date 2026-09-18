/**
 * @file rate_limiter_v2.h
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

struct RedisRateLimiterConfig {
    std::string host = "127.0.0.1";

    int port = 6379;

    std::string auth;

    std::string key_prefix = "themis:rl";

    int timeout_ms = 5000;

    int max_errors = 3;

    int key_ttl_seconds = 3600;

    int pool_size = 4;
};

class TokenBucketRateLimiter {
public:
    enum class Priority {
        HIGH = 0,   // VIP clients (e.g., via JWT claim "premium": true)
        NORMAL = 1, // Standard clients
        LOW = 2     // Batch/background jobs
    };

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

        Backend backend = Backend::LOCAL;

        RedisRateLimiterConfig redis;

        std::string bucket_id = "default";
    };

    /**
     * @brief Token Bucket Rate Limiter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit TokenBucketRateLimiter(const Config& config);
    ~TokenBucketRateLimiter();

    // Non-copyable; move is disabled to prevent double-free of redis_ctx_.
    TokenBucketRateLimiter(const TokenBucketRateLimiter&) = delete;
    TokenBucketRateLimiter& operator=(const TokenBucketRateLimiter&) = delete;
    TokenBucketRateLimiter(TokenBucketRateLimiter&&) = delete;
    TokenBucketRateLimiter& operator=(TokenBucketRateLimiter&&) = delete;

    bool tryAcquire(size_t tokens = 1, Priority prio = Priority::NORMAL);

    size_t getAvailableTokens(Priority prio = Priority::NORMAL) const;

    uint64_t getTotalRequests() const { return total_requests_.load(); }

    uint64_t getTotalRejections() const { return total_rejections_.load(); }

    /**
     * @brief Is Redis Healthy.
     * @return True when the operation succeeds.
     */
    bool isRedisHealthy() const;

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

    /**
     * @brief Is Healthy.
     * @return True when the operation succeeds.
     */
    bool isHealthy() const;

    /**
     * @brief Get Health Status.
     * @return Return value.
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

        /**
         * @brief Refill.
         */
        void refill();
        /**
         * @brief Consume.
         * @param[in] count Input parameter.
         * @return True when the operation succeeds.
         */
        bool consume(size_t count);
    };


    /**
     * @brief Redis Key.
     * @param[in] bucket_id Identifier of the bucket.
     * @param[in] prio Input parameter.
     * @return Return value.
     */
    std::string redisKey(const std::string& bucket_id, Priority prio) const;

    /**
     * @brief Redis Connect.
     * @return True when the operation succeeds.
     */
    bool redisConnect();

    /**
     * @brief Redis Eval Bucket.
     * @param[in] prio Input parameter.
     * @param[in] capacity Input parameter.
     * @param[in] refill_rate Input parameter.
     * @param[in] consume_count Input parameter.
     * @return Return value.
     */
    int redisEvalBucket(Priority prio, size_t capacity, size_t refill_rate,
                        size_t consume_count);

#ifdef THEMIS_ENABLE_REDIS
    /**
     * @brief Redis Exec Evalsha.
     * @param[in,out] slot Input/output parameter.
     * @param[in] key Input parameter.
     * @param[in] capacity Input parameter.
     * @param[in] refill_rate Input parameter.
     * @param[in] consume_count Input parameter.
     * @return Return value.
     */
    int redisExecEvalsha(RedisConnectionPool::Slot& slot,
                         const std::string& key, size_t capacity,
                         size_t refill_rate, size_t consume_count);
#endif

    /**
     * @brief Mark Redis Error.
     */
    void markRedisError();

    /**
     * @brief Try Redis Recover.
     */
    void tryRedisRecover();

    /**
     * @brief Local Try Acquire.
     * @param[in] tokens Input parameter.
     * @param[in] prio Input parameter.
     * @return True when the operation succeeds.
     */
    bool localTryAcquire(size_t tokens, Priority prio);
    /**
     * @brief Local Available Tokens.
     * @param[in] prio Input parameter.
     * @return Return value.
     */
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

class PerClientRateLimiter {
public:
    struct Config {
        size_t capacity_per_client = 0;
        size_t refill_rate_per_client;
        size_t max_clients;
        std::chrono::minutes cleanup_interval;

        TokenBucketRateLimiter::Backend backend = TokenBucketRateLimiter::Backend::LOCAL;

        RedisRateLimiterConfig redis;
        
        // Default constructor with values
        Config() 
            : capacity_per_client(100)
            , refill_rate_per_client(10)
            , max_clients(10000)
            , cleanup_interval(5) {}
    };

    PerClientRateLimiter();
    /**
     * @brief Per Client Rate Limiter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit PerClientRateLimiter(const Config& config);

    bool allowRequest(
        const std::string& client_id,
        size_t tokens = 1,
        TokenBucketRateLimiter::Priority prio = TokenBucketRateLimiter::Priority::NORMAL
    );

    struct ClientMetrics {
        uint64_t total_requests = 0;
        uint64_t total_rejections = 0;
        size_t available_tokens = 0;
    };
    /**
     * @brief Get Client Metrics.
     * @param[in] client_id Identifier of the client.
     * @return Return value.
     */
    ClientMetrics getClientMetrics(const std::string& client_id) const;

    /**
     * @brief Get Active Clients.
     * @return Return value.
     */
    size_t getActiveClients() const;

    /**
     * @brief Cleanup Idle Clients.
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
