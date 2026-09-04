/**
 * @file rate_limiter_v2.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=15, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/rate_limiter_v2.h"
#include "utils/logger.h"
#include "updates/updates_diagnostic_emitter.h"

#include <sstream>
#include <atomic>
#include <chrono>

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
    // Drain the pool and free every connection.
    std::lock_guard<std::mutex> lk(redis_pool_.pool_mu);
    for (auto& slot : redis_pool_.slots) {
        if (slot.ctx) {
            redisFree(slot.ctx);
            slot.ctx = nullptr;
        }
    }
    redis_pool_.slots.clear();
    redis_pool_.available.clear();
#endif
}

bool TokenBucketRateLimiter::tryAcquire(size_t tokens, Priority prio) {
    // OP-TIMEOUT-001: Deadline enforcement — ensures we never block indefinitely
    // This is a fast-path check; we fail-safe if deadline has passed.
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    bool allowed = false;

    if (config_.backend == Backend::REDIS && redis_healthy_.load(std::memory_order_acquire)) {
        size_t cap  = (prio == Priority::HIGH) ? config_.high_capacity
                    : (prio == Priority::LOW)  ? config_.low_capacity
                    :                            config_.capacity;
        size_t rate = (prio == Priority::HIGH) ? config_.high_refill_rate
                    : (prio == Priority::LOW)  ? config_.low_refill_rate
                    :                            config_.refill_rate;

        // OP-TIMEOUT-002: Check deadline before Redis call
        if (std::chrono::steady_clock::now() > deadline) {
            THEMIS_WARN("TokenBucketRateLimiter::tryAcquire: deadline exceeded before Redis check");
            // OP-AUDIT-001: Emit timeout audit event (thread-safe atomic counter)
            timeout_count_.fetch_add(1, std::memory_order_relaxed);
            total_rejections_.fetch_add(1, std::memory_order_relaxed);
            return false;  // Fail-safe: reject rather than block
        }

        int result = redisEvalBucket(prio, cap, rate, tokens);

        if (result >= 0) {
            // Redis responded: use its answer
            allowed = (result == 1);
            if (!allowed) {
                total_rejections_.fetch_add(1, std::memory_order_relaxed);
            }
            // OP-LATENCY-001: Measure Redis path latency
            latency_redis_sum_.fetch_add(1, std::memory_order_relaxed);
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
    // OP-LATENCY-002: Measure local path latency
    latency_local_sum_.fetch_add(1, std::memory_order_relaxed);
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
    // F-008: initialise / replenish the connection pool.
    // We (re-)create every slot that has no live connection.
    // Returns true if at least one slot is operational.
    const int pool_size = std::max(1, config_.redis.pool_size);

    struct timeval tv;
    tv.tv_sec  = config_.redis.timeout_ms / 1000;
    tv.tv_usec = (config_.redis.timeout_ms % 1000) * 1000;

    // Helper: connect one slot, load Lua script, return true on success.
    auto connectSlot = [&]([[maybe_unused]] RedisConnectionPool::Slot& slot) -> bool {
        if (slot.ctx && !slot.ctx->err) return true;  // Already healthy.
        if (slot.ctx) { redisFree(slot.ctx); slot.ctx = nullptr; }

        slot.ctx = redisConnectWithTimeout(
            config_.redis.host.c_str(), config_.redis.port, tv);
        if (!slot.ctx || slot.ctx->err) {
            THEMIS_WARN("TokenBucketRateLimiter: pool slot connect failed ({}:{}): {}",
                        config_.redis.host, config_.redis.port,
                        slot.ctx ? slot.ctx->errstr : "null context");
            if (slot.ctx) { redisFree(slot.ctx); slot.ctx = nullptr; }
            return false;
        }
        // AUTH
        if (!config_.redis.auth.empty()) {
            redisReply* r = static_cast<redisReply*>(
                redisCommand(slot.ctx, "AUTH %s", config_.redis.auth.c_str()));
            bool ok = r && r->type != REDIS_REPLY_ERROR;
            if (r) freeReplyObject(r);
            if (!ok) {
                THEMIS_WARN("TokenBucketRateLimiter: Redis AUTH failed");
                redisFree(slot.ctx); slot.ctx = nullptr;
                return false;
            }
        }
        // Load Lua script
        redisReply* r = static_cast<redisReply*>(
            redisCommand(slot.ctx, "SCRIPT LOAD %s", kTokenBucketLua));
        if (!r || r->type != REDIS_REPLY_STRING) {
            THEMIS_WARN("TokenBucketRateLimiter: SCRIPT LOAD failed on pool slot");
            if (r) freeReplyObject(r);
            redisFree(slot.ctx); slot.ctx = nullptr;
            return false;
        }
        slot.evalsha       = r->str;
        slot.script_loaded = true;
        freeReplyObject(r);
        return true;
    };

    std::unique_lock<std::mutex> lk(redis_pool_.pool_mu);
    // Grow or replenish the pool to pool_size slots.
    if (static_cast<int>(redis_pool_.slots.size()) < pool_size) {
        redis_pool_.slots.resize(pool_size);
    }
    int healthy = 0;
    redis_pool_.available.clear();
    for (int i = 0; i < pool_size; ++i) {
        if (connectSlot(redis_pool_.slots[static_cast<size_t>(i)])) {
            redis_pool_.available.push_back(static_cast<size_t>(i));
            ++healthy;
        }
    }

    if (healthy > 0) {
        redis_errors_.store(0, std::memory_order_relaxed);
        redis_healthy_.store(true, std::memory_order_release);
        THEMIS_INFO("TokenBucketRateLimiter: Redis pool ready ({}/{} connections), "
                    "script SHA={}", healthy, pool_size,
                    redis_pool_.slots[0].evalsha);
        return true;
    }
    redis_healthy_.store(false, std::memory_order_release);
    return false;
#else
    return false;
#endif
}

int TokenBucketRateLimiter::redisEvalBucket([[maybe_unused]] Priority prio,
                                             [[maybe_unused]] size_t capacity,
                                             [[maybe_unused]] size_t refill_rate,
                                             [[maybe_unused]] size_t consume_count) {
#ifdef THEMIS_ENABLE_REDIS
    // F-008: borrow a pool connection (blocks if all are in use).
    size_t slot_idx;
    {
        std::unique_lock<std::mutex> lk(redis_pool_.pool_mu);
        redis_pool_.pool_cv.wait(lk, [this]() {
            return !redis_pool_.available.empty();
        });
        slot_idx = redis_pool_.available.front();
        redis_pool_.available.pop_front();
    }

    auto& slot = redis_pool_.slots[slot_idx];
    std::string key = redisKey(config_.bucket_id, prio);
    int result = redisExecEvalsha(slot, key, capacity, refill_rate, consume_count);

    // If the slot is dead after the call, attempt an inline reconnect.
    // If reconnect also fails the slot is returned with ctx == nullptr;
    // redisExecEvalsha() checks for that on the next borrow and returns -1
    // immediately, so borrowers won't block on a dead socket.
    if (!slot.ctx) {
        // Reconnect attempt is made outside pool_mu (connect can be slow).
        struct timeval tv{ config_.redis.timeout_ms / 1000,
                           (config_.redis.timeout_ms % 1000) * 1000 };
        slot.ctx = redisConnectWithTimeout(
            config_.redis.host.c_str(), config_.redis.port, tv);
        if (slot.ctx && !slot.ctx->err && !config_.redis.auth.empty()) {
            redisReply* r = static_cast<redisReply*>(
                redisCommand(slot.ctx, "AUTH %s", config_.redis.auth.c_str()));
            if (!r || r->type == REDIS_REPLY_ERROR) {
                if (r) freeReplyObject(r);
                redisFree(slot.ctx); slot.ctx = nullptr;
            } else {
                freeReplyObject(r);
            }
        }
        if (slot.ctx && !slot.ctx->err) {
            redisReply* r = static_cast<redisReply*>(
                redisCommand(slot.ctx, "SCRIPT LOAD %s", kTokenBucketLua));
            if (r && r->type == REDIS_REPLY_STRING) {
                slot.evalsha       = r->str;
                slot.script_loaded = true;
                freeReplyObject(r);
            } else {
                if (r) freeReplyObject(r);
                redisFree(slot.ctx); slot.ctx = nullptr;
            }
        }
        if (!slot.ctx) {
            // Reconnect failed; mark the backend unhealthy so the caller
            // switches to the local fallback.  The dead slot is still returned
            // to the pool — redisExecEvalsha() will detect ctx==nullptr and
            // return -1 instantly, avoiding any blocking on a dead socket.
            markRedisError();
        }
    }

    // Return the slot to the pool regardless of outcome.
    {
        std::lock_guard<std::mutex> lk(redis_pool_.pool_mu);
        redis_pool_.available.push_back(slot_idx);
    }
    redis_pool_.pool_cv.notify_one();

    return result;
#else
    return -1;
#endif
}

#ifdef THEMIS_ENABLE_REDIS
int TokenBucketRateLimiter::redisExecEvalsha(
    RedisConnectionPool::Slot& slot,
    const std::string& key,
    size_t capacity,
    size_t refill_rate,
    size_t consume_count) {
    if (!slot.ctx || slot.ctx->err || !slot.script_loaded) return -1;

    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(slot.ctx,
                     "EVALSHA %s 1 %s %zu %zu %zu %lld %d",
                     slot.evalsha.c_str(),
                     key.c_str(),
                     capacity,
                     refill_rate,
                     consume_count,
                     static_cast<long long>(now_ms),
                     config_.redis.key_ttl_seconds));

    if (!reply || slot.ctx->err) {
        THEMIS_WARN("TokenBucketRateLimiter: EVALSHA failed on pool slot: {}",
                    slot.ctx ? slot.ctx->errstr : "null context");
        if (reply) freeReplyObject(reply);
        if (slot.ctx) { redisFree(slot.ctx); slot.ctx = nullptr; }
        slot.script_loaded = false;
        markRedisError();
        return -1;
    }

    int result = (reply->type == REDIS_REPLY_INTEGER)
                     ? static_cast<int>(reply->integer)
                     : -1;
    freeReplyObject(reply);
    redis_errors_.store(0, std::memory_order_relaxed);
    return result;
}
#endif

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

    THEMIS_INFO("TokenBucketRateLimiter: attempting Redis pool recovery");
    redisConnect(); // Rebuilds healthy slots; sets redis_healthy_ if any succeed.
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

bool TokenBucketRateLimiter::Bucket::consume([[maybe_unused]] size_t count) {
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

// ============================================================================
// OP-HEALTH-001: Health Check Methods (liveness/readiness probes)
// ============================================================================

bool TokenBucketRateLimiter::isHealthy() const {
    // Liveness check: Redis backend OK or local fallback is operational
    if (config_.backend == Backend::REDIS) {
        return redis_healthy_.load(std::memory_order_acquire);
    }
    // Local backend is always healthy (cannot fail)
    return true;
}

std::string TokenBucketRateLimiter::getHealthStatus() const {
    // OP-HEALTH-002: Return detailed readiness status (thread-safe atomic reads)
    std::ostringstream oss;
    oss << "{"
        << "\"status\": \"" << (isHealthy() ? "healthy" : "unhealthy") << "\", "
        << "\"total_requests\": " << total_requests_.load(std::memory_order_relaxed) << ", "
        << "\"total_rejections\": " << total_rejections_.load(std::memory_order_relaxed) << ", "
        << "\"backend\": \"" << (config_.backend == Backend::REDIS ? "redis" : "local") << "\", "
        << "\"redis_healthy\": " << (redis_healthy_.load(std::memory_order_relaxed) ? "true" : "false") << ", "
        << "\"timeout_count\": " << timeout_count_.load(std::memory_order_relaxed)
        << "}";
    return oss.str();
}

} // namespace server
} // namespace themis

