/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rate_limiter_v2.cpp                                ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:44:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     532                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ea7db4d78f  2026-03-11  feat(server): Redis backend for RateLimiterV2 (distribute... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/rate_limiter_v2.h"
#include "utils/logger.h"

#include <sstream>

#ifdef THEMIS_ENABLE_REDIS
#include <hiredis/hiredis.h>
#endif

namespace themis {
namespace server {

// ============================================================================
// Lua script for atomic token-bucket check-and-consume in Redis.
//
// KEYS[1]  – rate-limit key (e.g. "themis:rl:default:1")
// ARGV[1]  – bucket capacity     (size_t as string)
// ARGV[2]  – refill rate         (tokens per second, size_t as string)
// ARGV[3]  – tokens to consume   (size_t as string)
// ARGV[4]  – current Unix time   (milliseconds, as string)
// ARGV[5]  – key TTL in seconds  (size_t as string)
//
// Returns: 1  → allowed (tokens consumed)
//          0  → rejected (bucket empty)
// ============================================================================
static const char* kTokenBucketLua = R"LUA(
local key       = KEYS[1]
local capacity  = tonumber(ARGV[1])
local rate      = tonumber(ARGV[2])
local consume   = tonumber(ARGV[3])
local now_ms    = tonumber(ARGV[4])
local ttl_sec   = tonumber(ARGV[5])

local data = redis.call("HMGET", key, "tokens", "last_ms")
local tokens  = tonumber(data[1]) or capacity
local last_ms = tonumber(data[2]) or now_ms

-- refill
local elapsed_sec = math.max(0, (now_ms - last_ms) / 1000.0)
local new_tokens  = math.min(capacity, tokens + elapsed_sec * rate)

-- try to consume
if new_tokens < consume then
    -- Update the refilled amount even on rejection so subsequent calls see the correct state
    redis.call("HMSET", key, "tokens", new_tokens, "last_ms", now_ms)
    redis.call("EXPIRE", key, ttl_sec)
    return 0
end

new_tokens = new_tokens - consume
redis.call("HMSET", key, "tokens", new_tokens, "last_ms", now_ms)
redis.call("EXPIRE", key, ttl_sec)
return 1
)LUA";

// ===== TokenBucketRateLimiter =====

TokenBucketRateLimiter::TokenBucketRateLimiter(const Config& config)
    : config_(config)
{
    // Always initialise local buckets as fallback
    if (config_.enable_priority_lanes) {
        buckets_[Priority::HIGH] = std::make_unique<Bucket>(
            config_.high_capacity, config_.high_refill_rate);
        buckets_[Priority::NORMAL] = std::make_unique<Bucket>(
            config_.capacity, config_.refill_rate);
        buckets_[Priority::LOW] = std::make_unique<Bucket>(
            config_.low_capacity, config_.low_refill_rate);
    } else {
        // Single bucket for all priorities
        buckets_[Priority::NORMAL] = std::make_unique<Bucket>(
            config_.capacity, config_.refill_rate);
    }

    if (config_.backend == Backend::REDIS) {
#ifdef THEMIS_ENABLE_REDIS
        if (redisConnect()) {
            THEMIS_INFO("TokenBucketRateLimiter: Redis backend active ({}:{})",
                        config_.redis.host, config_.redis.port);
        } else {
            THEMIS_WARN("TokenBucketRateLimiter: Redis backend requested but connection failed; "
                        "falling back to local bucket");
        }
#else
        THEMIS_WARN("TokenBucketRateLimiter: Redis backend requested but built without "
                    "THEMIS_ENABLE_REDIS; falling back to local bucket");
#endif
    }
}

TokenBucketRateLimiter::~TokenBucketRateLimiter() {
#ifdef THEMIS_ENABLE_REDIS
    std::lock_guard<std::mutex> lk(redis_mutex_);
    if (redis_ctx_) {
        redisFree(redis_ctx_);
        redis_ctx_ = nullptr;
    }
#endif
}

bool TokenBucketRateLimiter::tryAcquire(size_t tokens, Priority prio) {
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    bool allowed = false;

    if (config_.backend == Backend::REDIS && redis_healthy_.load(std::memory_order_acquire)) {
        size_t cap  = (prio == Priority::HIGH) ? config_.high_capacity
                    : (prio == Priority::LOW)  ? config_.low_capacity
                    :                            config_.capacity;
        size_t rate = (prio == Priority::HIGH) ? config_.high_refill_rate
                    : (prio == Priority::LOW)  ? config_.low_refill_rate
                    :                            config_.refill_rate;

        int result = redisEvalBucket(prio, cap, rate, tokens);

        if (result >= 0) {
            // Redis responded: use its answer
            allowed = (result == 1);
            if (!allowed) {
                total_rejections_.fetch_add(1, std::memory_order_relaxed);
            }
            return allowed;
        }
        // result == -1 → Redis error; fall through to local bucket
        THEMIS_WARN("TokenBucketRateLimiter: Redis error; using local fallback for this request");
    }

    // Local bucket path
    if (!localTryAcquire(tokens, prio)) {
        total_rejections_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    return true;
}

bool TokenBucketRateLimiter::localTryAcquire(size_t tokens, Priority prio) {
    // If priority lanes disabled, use NORMAL bucket for all
    auto bucket_it = buckets_.find(prio);
    if (bucket_it == buckets_.end()) {
        bucket_it = buckets_.find(Priority::NORMAL);
    }

    if (bucket_it == buckets_.end()) {
        THEMIS_ERROR("RateLimiter: No bucket configured for priority {}", static_cast<int>(prio));
        return false; // Safe fallback: reject
    }

    auto& bucket = bucket_it->second;
    bucket->refill();
    return bucket->consume(tokens);
}

size_t TokenBucketRateLimiter::getAvailableTokens(Priority prio) const {
    return localAvailableTokens(prio);
}

size_t TokenBucketRateLimiter::localAvailableTokens(Priority prio) const {
    auto bucket_it = buckets_.find(prio);
    if (bucket_it == buckets_.end()) {
        bucket_it = buckets_.find(Priority::NORMAL);
    }

    if (bucket_it == buckets_.end()) {
        return 0;
    }

    return bucket_it->second->tokens.load(std::memory_order_relaxed);
}

bool TokenBucketRateLimiter::isRedisHealthy() const {
    return config_.backend == Backend::REDIS &&
           redis_healthy_.load(std::memory_order_acquire);
}

void TokenBucketRateLimiter::reset() {
    total_requests_.store(0, std::memory_order_relaxed);
    total_rejections_.store(0, std::memory_order_relaxed);

    for (auto& [prio, bucket] : buckets_) {
        std::lock_guard<std::mutex> lock(bucket->mutex);
        bucket->tokens.store(bucket->capacity, std::memory_order_relaxed);
        bucket->last_refill = std::chrono::steady_clock::now();
    }
}

// ===== Redis helpers =====

std::string TokenBucketRateLimiter::redisKey(const std::string& bucket_id,
                                              Priority prio) const {
    std::ostringstream oss;
    oss << config_.redis.key_prefix << ":" << bucket_id
        << ":" << static_cast<int>(prio);
    return oss.str();
}

bool TokenBucketRateLimiter::redisConnect() {
#ifdef THEMIS_ENABLE_REDIS
    std::lock_guard<std::mutex> lk(redis_mutex_);

    if (redis_ctx_ && !redis_ctx_->err) {
        return true; // Already connected
    }

    if (redis_ctx_) {
        redisFree(redis_ctx_);
        redis_ctx_ = nullptr;
    }

    struct timeval tv;
    tv.tv_sec  = config_.redis.timeout_ms / 1000;
    tv.tv_usec = (config_.redis.timeout_ms % 1000) * 1000;

    redis_ctx_ = redisConnectWithTimeout(
        config_.redis.host.c_str(), config_.redis.port, tv);

    if (!redis_ctx_ || redis_ctx_->err) {
        THEMIS_WARN("TokenBucketRateLimiter: Redis connect failed ({}:{}): {}",
                    config_.redis.host, config_.redis.port,
                    redis_ctx_ ? redis_ctx_->errstr : "null context");
        if (redis_ctx_) {
            redisFree(redis_ctx_);
            redis_ctx_ = nullptr;
        }
        redis_healthy_.store(false, std::memory_order_release);
        return false;
    }

    // AUTH
    if (!config_.redis.auth.empty()) {
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(redis_ctx_, "AUTH %s", config_.redis.auth.c_str()));
        bool ok = reply && reply->type != REDIS_REPLY_ERROR;
        if (reply) freeReplyObject(reply);
        if (!ok) {
            THEMIS_WARN("TokenBucketRateLimiter: Redis AUTH failed");
            redisFree(redis_ctx_);
            redis_ctx_ = nullptr;
            redis_healthy_.store(false, std::memory_order_release);
            return false;
        }
    }

    // Load Lua script via SCRIPT LOAD
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(redis_ctx_, "SCRIPT LOAD %s", kTokenBucketLua));

    if (!reply || reply->type != REDIS_REPLY_STRING) {
        THEMIS_WARN("TokenBucketRateLimiter: SCRIPT LOAD failed");
        if (reply) freeReplyObject(reply);
        redisFree(redis_ctx_);
        redis_ctx_ = nullptr;
        redis_healthy_.store(false, std::memory_order_release);
        return false;
    }

    evalsha_       = reply->str;
    script_loaded_ = true;
    freeReplyObject(reply);

    redis_errors_.store(0, std::memory_order_relaxed);
    redis_healthy_.store(true, std::memory_order_release);
    THEMIS_INFO("TokenBucketRateLimiter: Redis connected, script SHA={}", evalsha_);
    return true;
#else
    return false;
#endif
}

int TokenBucketRateLimiter::redisEvalBucket(Priority prio,
                                             size_t capacity,
                                             size_t refill_rate,
                                             size_t consume_count) {
#ifdef THEMIS_ENABLE_REDIS
    std::string key = redisKey(config_.bucket_id, prio);

    // Fast path: existing healthy connection
    {
        std::lock_guard<std::mutex> lk(redis_mutex_);
        if (redis_ctx_ && !redis_ctx_->err && script_loaded_) {
            return redisExecEvalsha(key, capacity, refill_rate, consume_count);
        }
    }

    // Reconnect path (redisConnect() acquires redis_mutex_ internally)
    if (!redisConnect()) {
        return -1;
    }

    // Retry after successful reconnect
    std::lock_guard<std::mutex> lk(redis_mutex_);
    if (!redis_ctx_ || !script_loaded_) {
        return -1;
    }
    return redisExecEvalsha(key, capacity, refill_rate, consume_count);
#else
    return -1;
#endif
}

int TokenBucketRateLimiter::redisExecEvalsha(const std::string& key,
                                              size_t capacity,
                                              size_t refill_rate,
                                              size_t consume_count) {
#ifdef THEMIS_ENABLE_REDIS
    // Caller must hold redis_mutex_ and have a valid redis_ctx_ with script loaded.
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(redis_ctx_,
                     "EVALSHA %s 1 %s %zu %zu %zu %lld %d",
                     evalsha_.c_str(),
                     key.c_str(),
                     capacity,
                     refill_rate,
                     consume_count,
                     static_cast<long long>(now_ms),
                     config_.redis.key_ttl_seconds));

    if (!reply || redis_ctx_->err) {
        THEMIS_WARN("TokenBucketRateLimiter: EVALSHA failed: {}",
                    redis_ctx_ ? redis_ctx_->errstr : "null context");
        if (reply) freeReplyObject(reply);
        redisFree(redis_ctx_);
        redis_ctx_     = nullptr;
        script_loaded_ = false;
        markRedisError();
        return -1;
    }

    int result = (reply->type == REDIS_REPLY_INTEGER)
                     ? static_cast<int>(reply->integer)
                     : -1;
    freeReplyObject(reply);
    redis_errors_.store(0, std::memory_order_relaxed);
    return result;
#else
    return -1;
#endif
}

void TokenBucketRateLimiter::markRedisError() {
    int errors = redis_errors_.fetch_add(1, std::memory_order_relaxed) + 1;
    if (errors >= config_.redis.max_errors) {
        redis_healthy_.store(false, std::memory_order_release);
        THEMIS_WARN("TokenBucketRateLimiter: Redis backend unhealthy after {} errors; "
                    "switched to local fallback", errors);
    }
}

void TokenBucketRateLimiter::tryRedisRecover() {
#ifdef THEMIS_ENABLE_REDIS
    if (config_.backend != Backend::REDIS) return;
    if (redis_healthy_.load(std::memory_order_acquire)) return;

    THEMIS_INFO("TokenBucketRateLimiter: attempting Redis recovery");
    redisConnect(); // Sets redis_healthy_ if successful
#endif
}

// ===== Bucket Implementation =====

void TokenBucketRateLimiter::Bucket::refill() {
    std::lock_guard<std::mutex> lock(mutex);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill);

    if (elapsed.count() <= 0) {
        return; // No time elapsed
    }

    // Calculate tokens to add: (refill_rate * elapsed_seconds)
    double elapsed_seconds = elapsed.count() / 1000.0;
    size_t tokens_to_add = static_cast<size_t>(refill_rate * elapsed_seconds);

    if (tokens_to_add > 0) {
        size_t current = tokens.load(std::memory_order_relaxed);
        size_t new_tokens = std::min(current + tokens_to_add, capacity);
        tokens.store(new_tokens, std::memory_order_relaxed);
        last_refill = now;
    }
}

bool TokenBucketRateLimiter::Bucket::consume(size_t count) {
    // Atomic decrement if sufficient tokens available
    size_t current = tokens.load(std::memory_order_acquire);

    while (current >= count) {
        if (tokens.compare_exchange_weak(current, current - count,
                                          std::memory_order_release,
                                          std::memory_order_acquire)) {
            return true; // Success
        }
        // CAS failed, retry with updated 'current'
    }

    return false; // Insufficient tokens
}

// ===== PerClientRateLimiter =====

PerClientRateLimiter::PerClientRateLimiter()
    : PerClientRateLimiter(Config{}) {
}

PerClientRateLimiter::PerClientRateLimiter(const Config& config)
    : config_(config)
    , last_cleanup_(std::chrono::steady_clock::now())
{
}

bool PerClientRateLimiter::allowRequest(
    const std::string& client_id,
    size_t tokens,
    TokenBucketRateLimiter::Priority prio
) {
    // Periodic cleanup of idle clients
    auto now = std::chrono::steady_clock::now();
    if (now - last_cleanup_ > config_.cleanup_interval) {
        cleanupIdleClients();
        last_cleanup_ = now;
    }

    std::unique_lock<std::mutex> lock(clients_mutex_);

    // Get or create client bucket
    auto it = client_buckets_.find(client_id);
    if (it == client_buckets_.end()) {
        // Enforce max clients limit
        if (client_buckets_.size() >= config_.max_clients) {
            THEMIS_WARN("PerClientRateLimiter: Max clients ({}) reached, rejecting new client: {}",
                        config_.max_clients, client_id);
            return false;
        }

        // Create new bucket for client
        auto client_bucket = std::make_unique<ClientBucket>();
        TokenBucketRateLimiter::Config limiter_cfg;
        limiter_cfg.capacity             = config_.capacity_per_client;
        limiter_cfg.refill_rate          = config_.refill_rate_per_client;
        limiter_cfg.enable_priority_lanes = false; // Per-client uses single bucket
        limiter_cfg.backend              = config_.backend;
        limiter_cfg.redis                = config_.redis;
        limiter_cfg.bucket_id            = client_id; // Use client_id as Redis key namespace
        client_bucket->limiter = std::make_unique<TokenBucketRateLimiter>(limiter_cfg);
        client_bucket->last_access = now;

        it = client_buckets_.emplace(client_id, std::move(client_bucket)).first;
    }

    auto& client_bucket = it->second;
    client_bucket->last_access = now;
    client_bucket->total_requests.fetch_add(1, std::memory_order_relaxed);

    lock.unlock(); // Unlock before trying to acquire tokens

    bool allowed = client_bucket->limiter->tryAcquire(tokens, prio);

    if (!allowed) {
        client_bucket->total_rejections.fetch_add(1, std::memory_order_relaxed);
    }

    return allowed;
}

PerClientRateLimiter::ClientMetrics
PerClientRateLimiter::getClientMetrics(const std::string& client_id) const {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = client_buckets_.find(client_id);
    if (it == client_buckets_.end()) {
        return ClientMetrics{};
    }

    const auto& bucket = it->second;
    return ClientMetrics{
        .total_requests = bucket->total_requests.load(std::memory_order_relaxed),
        .total_rejections = bucket->total_rejections.load(std::memory_order_relaxed),
        .available_tokens = bucket->limiter->getAvailableTokens()
    };
}

size_t PerClientRateLimiter::getActiveClients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return client_buckets_.size();
}

void PerClientRateLimiter::cleanupIdleClients() {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto now = std::chrono::steady_clock::now();
    auto idle_threshold = std::chrono::minutes(10); // Remove after 10min idle

    for (auto it = client_buckets_.begin(); it != client_buckets_.end(); ) {
        if (now - it->second->last_access > idle_threshold) {
            THEMIS_DEBUG("PerClientRateLimiter: Removing idle client: {}", it->first);
            it = client_buckets_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace server
} // namespace themis
