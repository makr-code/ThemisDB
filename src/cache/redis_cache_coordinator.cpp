/**
 * @file redis_cache_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

/*
 * HiredisRedisCacheCoordinator – Redis pub/sub backed ICacheCoordinator implementation.
 *
 * When THEMIS_ENABLE_REDIS is defined (hiredis available), full pub/sub
 * connectivity is established.  When undefined the coordinator compiles to a
 * no-op stub that logs a warning on construction, allowing the rest of the
 * cache stack to function without a Redis dependency.
 */

#include "cache/redis_cache_coordinator.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <thread>
#include <climits>
#include <sstream>
#include <stdexcept>

#include "observability/metrics_collector.h"
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_REDIS
#include <hiredis/hiredis.h>
#endif

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace themis {
namespace cache {


// ============================================================================
// LOCKING:
//  - pub_mutex_ protects the publisher context and is released before
//    stats_mutex_ is acquired for counter updates.
//  - publish_status_mutex_ protects ordered publish outcomes and is always
//    acquired in isolation, never nested with the connection or stats locks.
//  - cb_mutex_ is acquired in isolation for callback registration/snapshots.
//  - s_redis_pub_fn_mutex is used only by the static test-hook setter.
//  - The subscriber loop exclusively owns sub_ctx_; it does not share the
//    connection with the publisher path.
// ============================================================================

// ---------------------------------------------------------------------------
// BRIDGE_REF #42 — RedisPublishFn static bridge (non-hiredis injection)
// ---------------------------------------------------------------------------
namespace {
std::mutex s_redis_pub_fn_mutex;
std::function<bool(const std::string &, const std::string &)> s_redis_pub_fn;

void recordRedisMetric(const char* name) noexcept {
    try {
        observability::MetricsCollector::getInstance().addCounter(name, 1);
    } catch (...) {
    }
}

#ifdef THEMIS_ENABLE_REDIS
constexpr int kMaxPublishRetries   = 2;   ///< at most 2 reconnect+retry attempts
constexpr int kPublishRetryDelayMs = 50;  ///< initial retry delay: 50 ms
#endif
} // namespace

/**
 * @brief Set Redis Publish Fn.
 * @param[in] fn Input parameter.
 * @details Calls: lk(), std::move().
 */
void HiredisRedisCacheCoordinator::setRedisPublishFn(RedisPublishFn fn) {
    std::lock_guard<std::mutex> lk(s_redis_pub_fn_mutex);
    s_redis_pub_fn = std::move(fn);
}

uint64_t HiredisRedisCacheCoordinator::beginPublishStatusUpdate() {
    std::lock_guard<std::mutex> lk(publish_status_mutex_);
    return ++publish_sequence_;
}

void HiredisRedisCacheCoordinator::recordPublishStatus(uint64_t sequence, bool success, bool invalidation) {
    std::lock_guard<std::mutex> lk(publish_status_mutex_);
    if (sequence >= latest_publish_status_sequence_) {
        latest_publish_status_sequence_ = sequence;
        publish_degraded_ = !success;
    }
    if (invalidation && sequence >= latest_invalidation_status_sequence_) {
        latest_invalidation_status_sequence_ = sequence;
        invalidation_degraded_ = !success;
    }
}

// ============================================================================
// Constructor / destructor
// ============================================================================

HiredisRedisCacheCoordinator::HiredisRedisCacheCoordinator(const Config &config) : config_(config) {
    // Derive channel name and effective node_id
    channel_ = config_.channel_prefix + ":replication";
    node_id_ = config_.node_id.empty() ? (config_.host + ":" + std::to_string(config_.port)) : config_.node_id;

#ifdef THEMIS_ENABLE_REDIS
    THEMIS_INFO("RedisCacheCoordinator: channel={} node_id={}", channel_, node_id_);

    // Start background subscribe thread
    running_.store(true);
    sub_thread_ = std::thread([this] { subscribeLoop(); });
#else
    THEMIS_WARN("RedisCacheCoordinator: built without hiredis (THEMIS_ENABLE_REDIS not defined); "
                "Redis transport unavailable; coordinator readiness remains false.");
    pub_connected_.store(false);
    sub_connected_.store(false);
    recordRedisMetric("cache.redis.dependency_unavailable");
#endif
}

HiredisRedisCacheCoordinator::~HiredisRedisCacheCoordinator() {
#ifdef THEMIS_ENABLE_REDIS
    // Signal the subscribe loop to exit, then wait for it to do so before
    // freeing any connections.
    running_.store(false);

    if (sub_thread_.joinable()) {
        sub_thread_.join();
    }
    sub_connected_.store(false);

    // sub_ctx_ is now only owned by this thread (subscribe loop has exited)
    if (sub_ctx_) {
        redisFree(sub_ctx_);
        sub_ctx_ = nullptr;
    }

    {
        /**
         * @brief Lk.
         * @param[in] pub_mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(pub_mutex_);
        if (pub_ctx_) {
            redisFree(pub_ctx_);
            pub_ctx_ = nullptr;
        }
        pub_connected_.store(false);
    }

    THEMIS_DEBUG("RedisCacheCoordinator destroyed");
#endif
}

// ============================================================================
// ICacheCoordinator – publish side
// ============================================================================

/**
 * @brief Publish Entry.
 * @param[in] key Input parameter.
 * @param[in] result Input parameter.
 * @param[in] ttl_seconds Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: serializeMessage(), THEMIS_WARN(), std::this_thread::sleep_for(), std::chrono::milliseconds(), lk(), connectPublish(), redisCommand(), c_str().
 */
void HiredisRedisCacheCoordinator::publishEntry(const std::string &key, const nlohmann::json &result, int ttl_seconds,
                                         const std::string &tenant_id) {
#ifdef THEMIS_ENABLE_REDIS
    ReplicationMessage msg;
    msg.type        = ReplicationMessage::Type::ENTRY_PUT;
    msg.key         = key;
    msg.result      = result;
    msg.ttl_seconds = ttl_seconds;
    msg.tenant_id   = tenant_id;

    std::string payload = serializeMessage(msg);
    const uint64_t publish_sequence = beginPublishStatusUpdate();

    // no_retry_logic fix: retry the publish+reconnect cycle up to kMaxPublishRetries
    // times before giving up, using kPublishRetryDelayMs initial back-off.
    // LOCK ORDER: pub_mutex_ acquired first; stats_mutex_ updated separately
    // after pub_mutex_ is released to respect the canonical lock hierarchy.
    bool publish_ok = false;
    int retry_delay_ms = kPublishRetryDelayMs;
    for (int attempt = 0; attempt <= kMaxPublishRetries && !publish_ok; ++attempt) {
        if (attempt > 0) {
            THEMIS_WARN("RedisCacheCoordinator::publishEntry: retry {}/{} after {}ms",
                        attempt, kMaxPublishRetries, retry_delay_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
            retry_delay_ms *= 2;
        }
        std::lock_guard<std::mutex> lk(pub_mutex_);
        if (!connectPublish()) {
            continue; // will retry on next attempt
        }
        redisReply *reply = static_cast<redisReply *>(
            redisCommand(pub_ctx_, "PUBLISH %s %b", channel_.c_str(), payload.data(),payload.size()));

        if (reply == nullptr || pub_ctx_->err) {
            THEMIS_WARN("RedisCacheCoordinator::publishEntry: PUBLISH failed: {}",
                        pub_ctx_ ? pub_ctx_->errstr : "null context");
            if (reply)
                freeReplyObject(reply);
            // Close broken connection; will reconnect on next attempt
            redisFree(pub_ctx_);
            pub_ctx_ = nullptr;
            pub_connected_.store(false);
            recordRedisMetric("cache.redis.connection_failure");
        } else if (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0) {
            freeReplyObject(reply);
            publish_ok = true;
        } else {
            if (reply) {
                freeReplyObject(reply);
            }
        }
    } // pub_mutex_ released before stats_mutex_ is acquired

    {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        if (publish_ok) {
            ++messages_published_;
        } else {
            ++publish_errors_;
        }
    }
    recordPublishStatus(publish_sequence, publish_ok, false);
    if (!publish_ok) {
        recordRedisMetric("cache.redis.publish_error");
    }
#else
    (void)key;
    (void)result;
    (void)ttl_seconds;
    (void)tenant_id;
    {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++publish_errors_;
    }
    recordRedisMetric("cache.redis.publish_error");
#endif
}

/**
 * @brief Publish Invalidation.
 * @param[in] pattern Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: serializeMessage(), THEMIS_WARN(), std::this_thread::sleep_for(), std::chrono::milliseconds(), lk(), connectPublish(), redisCommand(), c_str().
 */
void HiredisRedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
#ifdef THEMIS_ENABLE_REDIS
    ReplicationMessage msg;
    msg.type      = ReplicationMessage::Type::INVALIDATE;
    msg.key       = pattern;
    msg.tenant_id = tenant_id;

    std::string payload = serializeMessage(msg);
    const uint64_t publish_sequence = beginPublishStatusUpdate();

    // no_retry_logic fix: retry the publish+reconnect cycle up to kMaxPublishRetries
    // times before giving up. LOCK ORDER: pub_mutex_ acquired first; stats_mutex_
    // updated separately after pub_mutex_ is released (canonical lock hierarchy).
    bool publish_ok = false;
    int retry_delay_ms = kPublishRetryDelayMs;
    for (int attempt = 0; attempt <= kMaxPublishRetries && !publish_ok; ++attempt) {
        if (attempt > 0) {
            THEMIS_WARN("RedisCacheCoordinator::publishInvalidation: retry {}/{} after {}ms",
                        attempt, kMaxPublishRetries, retry_delay_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
            retry_delay_ms *= 2;
        }
        std::lock_guard<std::mutex> lk(pub_mutex_);
        if (!connectPublish()) {
            continue; // will retry on next attempt
        }
        redisReply *reply = static_cast<redisReply *>(
            redisCommand(pub_ctx_, "PUBLISH %s %b", channel_.c_str(), payload.data(),payload.size()));

        if (reply == nullptr || pub_ctx_->err) {
            THEMIS_WARN("RedisCacheCoordinator::publishInvalidation: PUBLISH failed: {}",
                        pub_ctx_ ? pub_ctx_->errstr : "null context");
            if (reply)
                freeReplyObject(reply);
            redisFree(pub_ctx_);
            pub_ctx_ = nullptr;
            pub_connected_.store(false);
            recordRedisMetric("cache.redis.connection_failure");
        } else if (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0) {
            freeReplyObject(reply);
            publish_ok = true;
        } else {
            if (reply) {
                freeReplyObject(reply);
            }
        }
    } // pub_mutex_ released before stats_mutex_ is acquired

    {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        if (publish_ok) {
            ++messages_published_;
        } else {
            ++publish_errors_;
            ++invalidation_errors_;
        }
    }
    recordPublishStatus(publish_sequence, publish_ok, true);
    if (!publish_ok) {
        recordRedisMetric("cache.redis.publish_error");
        recordRedisMetric("cache.redis.invalidation_error");
    }
#else
    (void)pattern;
    (void)tenant_id;
    {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++publish_errors_;
        ++invalidation_errors_;
    }
    recordRedisMetric("cache.redis.publish_error");
    recordRedisMetric("cache.redis.invalidation_error");
#endif
}

/**
 * @brief Subscribe Entries.
 * @param[in] callback Input parameter.
 * @details Calls: lk(), std::move().
 */
void HiredisRedisCacheCoordinator::subscribeEntries(EntryCallback callback) {
    std::lock_guard<std::mutex> lk(cb_mutex_);
    entry_cb_ = std::move(callback);
}

/**
 * @brief Subscribe Invalidations.
 * @param[in] callback Input parameter.
 * @details Calls: lk(), std::move().
 */
void HiredisRedisCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
    std::lock_guard<std::mutex> lk(cb_mutex_);
    invalidation_cb_ = std::move(callback);
}

// ============================================================================
// Health / diagnostics
// ============================================================================

bool HiredisRedisCacheCoordinator::isConnected() const {
    return pub_connected_.load() && sub_connected_.load();
}

std::string HiredisRedisCacheCoordinator::name() const {
    return "RedisCacheCoordinator(" + config_.host + ":" + std::to_string(config_.port) + ")";
}

nlohmann::json HiredisRedisCacheCoordinator::getStats() const {
    uint64_t pub, recv, err, invalidation_err, reconn;
    {
        /**
         * @brief Lk.
         * @param[in] stats_mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(stats_mutex_);
        pub    = messages_published_;
        recv   = messages_received_;
        err    = publish_errors_;
        invalidation_err = invalidation_errors_;
        reconn = reconnect_count_;
    }
    const bool connected = isConnected();
    bool degraded = false;
    bool invalidation_degraded = false;
    {
        std::lock_guard<std::mutex> status_lock(publish_status_mutex_);
        degraded = publish_degraded_ || invalidation_degraded_;
        invalidation_degraded = invalidation_degraded_;
    }
    const bool ready = connected && !degraded;
#ifdef THEMIS_ENABLE_REDIS
    constexpr bool dependency_available = true;
#else
    constexpr bool dependency_available = false;
#endif
    return {{"messages_published", pub},
            {"messages_received", recv},
            {"publish_errors", err},
            {"invalidation_errors", invalidation_err},
            {"reconnect_count", reconn},
            {"connected", connected},
            {"ready", ready},
            {"dependency_available", dependency_available},
            {"status", dependency_available
                           ? (!connected ? "disconnected" : degraded ? "degraded" : "ready")
                           : "dependency_unavailable"},
            {"failure_reason", !dependency_available
                                   ? "hiredis_unavailable"
                                   : !connected ? "redis_connection_unavailable"
                                                : degraded ? (invalidation_degraded
                                                                  ? "invalidation_publish_failed"
                                                                  : "publish_failed")
                                                           : ""},
            {"channel", channel_},
            {"node_id", node_id_}};
}

// ============================================================================
// Private helpers
// ============================================================================

#ifdef THEMIS_ENABLE_REDIS

namespace {
constexpr int kReconnectBackoffMaxMs = 30000; ///< Maximum back-off: 30 seconds
} // anonymous namespace

/**
 * @brief Connect Publish.
 * @return True when the operation succeeds.
 * @details Calls: redisFree(), redisConnectWithTimeout(), c_str(), THEMIS_WARN(), store(), std::to_string(), empty(), redisCommand().
 */
bool HiredisRedisCacheCoordinator::connectPublish() {
    // Caller must hold pub_mutex_
    if (pub_ctx_ != nullptr && !pub_ctx_->err) {
        return true; // Already connected
    }

    // Clean up any broken context
    if (pub_ctx_) {
        redisFree(pub_ctx_);
        pub_ctx_ = nullptr;
    }

    struct timeval timeout;
    timeout.tv_sec  = config_.connect_timeout_ms / 1000;
    timeout.tv_usec = (config_.connect_timeout_ms % 1000) * 1000;

    pub_ctx_ = redisConnectWithTimeout(config_.host.c_str(), config_.port, timeout);
    if (pub_ctx_ == nullptr) {
        THEMIS_WARN("RedisCacheCoordinator: publish connect failed (null context) {}:{}", config_.host, config_.port);
        pub_connected_.store(false);
        recordRedisMetric("cache.redis.connection_failure");
        return false;
    }
    if (pub_ctx_->err) {
        THEMIS_WARN("RedisCacheCoordinator: publish connect error {}: {}",
                    config_.host + ":" + std::to_string(config_.port), pub_ctx_->errstr);
        redisFree(pub_ctx_);
        pub_ctx_ = nullptr;
        pub_connected_.store(false);
        recordRedisMetric("cache.redis.connection_failure");
        return false;
    }

    // Authenticate if password provided
    if (!config_.auth.empty()) {
        redisReply *r = static_cast<redisReply *>(redisCommand(pub_ctx_, "AUTH %s", config_.auth.c_str()));
        if (r == nullptr || pub_ctx_->err || (r->type == REDIS_REPLY_ERROR)) {
            THEMIS_WARN("RedisCacheCoordinator: AUTH failed on publish connection");
            if (r)
                freeReplyObject(r);
            redisFree(pub_ctx_);
            pub_ctx_ = nullptr;
            pub_connected_.store(false);
            recordRedisMetric("cache.redis.connection_failure");
            return false;
        }
        freeReplyObject(r);
    }

    pub_connected_.store(true);
    THEMIS_INFO("RedisCacheCoordinator: publish connection established to {}:{}", config_.host, config_.port);
    return true;
}

/**
 * @brief Connect Subscribe.
 * @return True when the operation succeeds.
 * @details Calls: redisFree(), redisConnectWithTimeout(), c_str(), THEMIS_WARN(), empty(), redisCommand(), freeReplyObject(), redisSetTimeout().
 */
bool HiredisRedisCacheCoordinator::connectSubscribe() {
    sub_connected_.store(false);
    if (sub_ctx_) {
        redisFree(sub_ctx_);
        sub_ctx_ = nullptr;
    }

    struct timeval timeout;
    timeout.tv_sec  = config_.connect_timeout_ms / 1000;
    timeout.tv_usec = (config_.connect_timeout_ms % 1000) * 1000;

    sub_ctx_ = redisConnectWithTimeout(config_.host.c_str(), config_.port, timeout);
    if (sub_ctx_ == nullptr || sub_ctx_->err) {
        THEMIS_WARN("RedisCacheCoordinator: subscribe connect failed {}:{}", config_.host, config_.port);
        if (sub_ctx_) {
            redisFree(sub_ctx_);
            sub_ctx_ = nullptr;
        }
        return false;
    }

    // Authenticate
    if (!config_.auth.empty()) {
        redisReply *r = static_cast<redisReply *>(redisCommand(sub_ctx_, "AUTH %s", config_.auth.c_str()));
        if (r == nullptr || sub_ctx_->err || (r->type == REDIS_REPLY_ERROR)) {
            THEMIS_WARN("RedisCacheCoordinator: AUTH failed on subscribe connection");
            if (r)
                freeReplyObject(r);
            redisFree(sub_ctx_);
            sub_ctx_ = nullptr;
            return false;
        }
        freeReplyObject(r);
    }

    // Issue SUBSCRIBE command
    redisReply *r = static_cast<redisReply *>(redisCommand(sub_ctx_, "SUBSCRIBE %s", channel_.c_str()));
    const bool subscription_acknowledged =
        r != nullptr && !sub_ctx_->err && r->type == REDIS_REPLY_ARRAY && r->elements == 3
        && r->element != nullptr && r->element[0] != nullptr && r->element[0]->type == REDIS_REPLY_STRING
        && r->element[0]->str != nullptr
        && std::string(r->element[0]->str, static_cast<size_t>(r->element[0]->len)) == "subscribe"
        && r->element[1] != nullptr && r->element[1]->type == REDIS_REPLY_STRING
        && r->element[1]->str != nullptr
        && std::string(r->element[1]->str, static_cast<size_t>(r->element[1]->len)) == channel_
        && r->element[2] != nullptr && r->element[2]->type == REDIS_REPLY_INTEGER && r->element[2]->integer > 0;
    if (!subscription_acknowledged) {
        THEMIS_WARN("RedisCacheCoordinator: SUBSCRIBE was not acknowledged on channel {}", channel_);
        if (r)
            freeReplyObject(r);
        redisFree(sub_ctx_);
        sub_ctx_ = nullptr;
        return false;
    }
    freeReplyObject(r);
    sub_connected_.store(true);

    // Set a short read timeout so the subscribe loop can periodically check
    // the running_ flag and exit cleanly on destruction.
    struct timeval read_timeout;
    read_timeout.tv_sec  = 0;
    read_timeout.tv_usec = 200 * 1000; // 200 ms
    redisSetTimeout(sub_ctx_, read_timeout);

    THEMIS_INFO("RedisCacheCoordinator: subscribe connection established, channel={}", channel_);
    return true;
}

/**
 * @brief Subscribe Loop.
 * @details Calls: std::max(), load(), connectSubscribe(), slk(), observability::MetricsCollector::getInstance(), addCounter(), THEMIS_DEBUG(), what().
 */
void HiredisRedisCacheCoordinator::subscribeLoop() {
    // Exponential back-off: starts at config_.reconnect_interval_ms, doubles
    // each failed attempt, capped at kReconnectBackoffMaxMs.
    // Resets to the base interval on a successful connection.
    const int backoff_base_ms = std::max(1, config_.reconnect_interval_ms);
    int backoff_ms            = backoff_base_ms;

    while (running_.load()) {
        // Connect the subscribe connection
        if (!connectSubscribe()) {
            if (running_.load()) {
                {
                    std::lock_guard<std::mutex> slk(stats_mutex_);
                    ++reconnect_count_;
                }
                // Emit reconnect metric
                try {
                    auto &mc = observability::MetricsCollector::getInstance();
                    mc.addCounter("cache.redis.reconnect", 1);
                    mc.addCounter("cache.redis.connection_failure", 1);
                } catch (const std::exception &ex) {
                    THEMIS_DEBUG("RedisCacheCoordinator: metric emit failed: {}", ex.what());
                } catch (const std::string &ex) {
                    THEMIS_DEBUG("RedisCacheCoordinator: metric emit failed: {}", ex);
                } catch (const char *ex) {
                    THEMIS_DEBUG("RedisCacheCoordinator: metric emit failed: {}", (ex ? ex : "<null>"));
                }

                THEMIS_WARN("RedisCacheCoordinator: subscribe connect failed; "
                            "retrying in {} ms (back-off)",
                            backoff_ms);
            }
            // Interruptible sleep in 50 ms increments so shutdown is fast
            for (int elapsed = 0; elapsed < backoff_ms && running_.load(); elapsed += 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            // Double the back-off interval, capped at max
            backoff_ms = std::min(backoff_ms * 2, kReconnectBackoffMaxMs);
            continue;
        }

        // Successful connection – reset back-off
        backoff_ms = backoff_base_ms;

        // Read messages in a loop; redisGetReply times out every 200ms
        // (set via redisSetTimeout in connectSubscribe) so we can check running_.
        redisReply *reply = nullptr;
        while (running_.load() && sub_ctx_ != nullptr) {
            int rc = redisGetReply(sub_ctx_, reinterpret_cast<void **>(&reply));

            if (rc != REDIS_OK) {
                // EAGAIN / EWOULDBLOCK = read timeout; check running_ and retry
                if ((sub_ctx_ && sub_ctx_->err == REDIS_ERR_IO && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    reply = nullptr;
                    continue;
                }
                // Real connection error – break and reconnect
                THEMIS_WARN("RedisCacheCoordinator: subscribe read error – reconnecting");
                break;
            }

            if (reply == nullptr) {
                break;
            }

            // A pub/sub message is an array: ["message", channel, payload]
            if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3
                && reply->element[0]->type == REDIS_REPLY_STRING && std::string(reply->element[0]->str) == "message"
                && reply->element[2]->type == REDIS_REPLY_STRING) {
                std::string payload(reply->element[2]->str, static_cast<size_t>(reply->element[2]->len));
                handleMessage(payload);

                {
                    std::lock_guard<std::mutex> slk(stats_mutex_);
                    ++messages_received_;
                }
            }

            freeReplyObject(reply);
            reply = nullptr;
        }

        if (reply) {
            freeReplyObject(reply);
        }

        sub_connected_.store(false);
        if (sub_ctx_) {
            redisFree(sub_ctx_);
            sub_ctx_ = nullptr;
        }
        {
            std::lock_guard<std::mutex> publisher_lock(pub_mutex_);
            if (pub_ctx_) {
                redisFree(pub_ctx_);
                pub_ctx_ = nullptr;
            }
            pub_connected_.store(false);
        }

        if (running_.load()) {
            {
                std::lock_guard<std::mutex> slk(stats_mutex_);
                ++reconnect_count_;
            }
            // Emit reconnect metric
            try {
                auto &mc = observability::MetricsCollector::getInstance();
                mc.addCounter("cache.redis.reconnect", 1);
                mc.addCounter("cache.redis.connection_failure", 1);
            } catch (const std::exception &ex) {
                THEMIS_DEBUG("RedisCacheCoordinator: metric emit failed: {}", ex.what());
            } catch (const std::string &ex) {
                THEMIS_DEBUG("RedisCacheCoordinator: metric emit failed: {}", ex);
            } catch (const char *ex) {
                THEMIS_DEBUG("RedisCacheCoordinator: metric emit failed: {}", (ex ? ex : "<null>"));
            }

            THEMIS_WARN("RedisCacheCoordinator: subscriber connection lost; "
                        "reconnecting in {} ms (back-off)",
                        backoff_ms);
            for (int elapsed = 0; elapsed < backoff_ms && running_.load(); elapsed += 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            backoff_ms = std::min(backoff_ms * 2, kReconnectBackoffMaxMs);
        }
    }
    THEMIS_DEBUG("RedisCacheCoordinator: subscribe loop exited");
}

#else // !THEMIS_ENABLE_REDIS

// PERMANENT FALLBACK NOTE:
// Purpose: Provide link-compatible no-op bodies for the three private methods
//   that depend on hiredis so that RedisCacheCoordinator compiles and links on
//   systems without hiredis.  The constructor (above) logs a WARN and sets
//   pub_connected_ = false so any caller can detect the degraded state.
// Activation: THEMIS_ENABLE_REDIS is not defined (default in minimal builds).
//   Enable via vcpkg feature 'redis' or -DTHEMIS_ENABLE_REDIS=ON.
// Production Delta: Cross-node cache invalidation is disabled; each node
//   maintains an independent in-process cache.  Invalidations issued on one
//   node are NOT propagated to peers, potentially causing stale reads across
//   a distributed deployment for the duration of the cache TTL.
// This is the PERMANENT FALLBACK for no-hiredis builds; it is not a temporary stub.
// Roadmap ref: src/cache/FUTURE_ENHANCEMENTS.md (Redis pub/sub invalidation — planned)
/**
 * @brief Connect Publish.
 * @return True when the operation succeeds.
 * @details Implements connectPublish without additional internal calls.
 */
bool HiredisRedisCacheCoordinator::connectPublish() {
    return false;
}
/**
 * @brief Connect Subscribe.
 * @return True when the operation succeeds.
 * @details Implements connectSubscribe without additional internal calls.
 */
bool HiredisRedisCacheCoordinator::connectSubscribe() {
    return false;
}
/**
 * @brief Subscribe Loop.
 * @details Implements subscribeLoop without additional internal calls.
 */
void HiredisRedisCacheCoordinator::subscribeLoop() {}

#endif // THEMIS_ENABLE_REDIS

/**
 * @brief Handle Message.
 * @param[in] payload Input parameter.
 * @details Calls: nlohmann::json::parse(), verifyHmac(), value(), THEMIS_WARN(), lk(), entry_cb(), inv_cb(), what().
 */
void HiredisRedisCacheCoordinator::handleMessage(const std::string &payload) {
    // Parse once; check self-echo before dispatching to avoid spurious evictions.
    try {
        auto j = nlohmann::json::parse(payload);

        // Verify HMAC signature when a shared secret is configured.
        if (!verifyHmac(j)) {
            return;
        }

        // Discard messages originating from this node (self-echo prevention)
        if (j.value("node_id", "") == node_id_) {
            return;
        }

        std::string type_str = j.value("type", "");
        ReplicationMessage msg = {};
        if (type_str == "ENTRY_PUT") {
            msg.type = ReplicationMessage::Type::ENTRY_PUT;
        } else if (type_str == "INVALIDATE") {
            msg.type = ReplicationMessage::Type::INVALIDATE;
        } else {
            THEMIS_WARN("RedisCacheCoordinator: unknown message type '{}' (ignored)", type_str);
            return;
        }

        msg.key         = j.value("key", "");
        msg.tenant_id   = j.value("tenant_id", "");
        msg.ttl_seconds = j.value("ttl_seconds", 0);
        msg.result      = j.value("result", nlohmann::json{});

        EntryCallback entry_cb;
        InvalidationCallback inv_cb;
        {
            std::lock_guard<std::mutex> lk(cb_mutex_);
            entry_cb = entry_cb_;
            inv_cb   = invalidation_cb_;
        }

        if (msg.type == ReplicationMessage::Type::ENTRY_PUT && entry_cb) {
            entry_cb(msg);
        } else if (msg.type == ReplicationMessage::Type::INVALIDATE && inv_cb) {
            inv_cb(msg);
        }
    } catch (const std::exception &ex) {
        THEMIS_WARN("RedisCacheCoordinator: message handling error: {}", ex.what());
    }
}

std::string HiredisRedisCacheCoordinator::serializeMessage(const ReplicationMessage &msg) const {
    nlohmann::json j;
    j["type"]        = (msg.type == ReplicationMessage::Type::ENTRY_PUT) ? "ENTRY_PUT" : "INVALIDATE";
    j["key"]         = msg.key;
    j["tenant_id"]   = msg.tenant_id;
    j["ttl_seconds"] = msg.ttl_seconds;
    j["result"]      = msg.result;
    j["node_id"]     = node_id_; // Used by peers for self-echo detection

    std::string payload = j.dump();

    std::string sig = computeHmac(payload);
    if (!sig.empty()) {
        // Re-serialise with the signature field appended.  We produce the
        // signature over the payload WITHOUT the sig field to avoid a
        // chicken-and-egg dependency.
        j["sig"] = sig;
        payload  = j.dump();
    }

    return payload;
}

std::string HiredisRedisCacheCoordinator::computeHmac(const std::string &payload) const {
    if (config_.hmac_secret.empty()) {
        return {};
    }

    // Guard against pathological sizes that would truncate in the cast to int.
    if (config_.hmac_secret.size() > static_cast<size_t>(INT_MAX) || payload.size() > static_cast<size_t>(INT_MAX)) {
        THEMIS_WARN("RedisCacheCoordinator: HMAC input exceeds INT_MAX – aborting");
        return {};
    }

    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;

    if (!HMAC(EVP_sha256(), config_.hmac_secret.data(), config_.hmac_secret.size(),
              reinterpret_cast<const unsigned char *>(payload.data()), payload.size(), md, &md_len)) {
        THEMIS_WARN("RedisCacheCoordinator: HMAC computation failed");
        return {};
    }

    std::ostringstream oss = {};
    for (unsigned int i = 0; i < md_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md[i]);
    }
    return oss.str();
}

bool HiredisRedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {
    // Signing disabled – all messages are accepted.
    if (config_.hmac_secret.empty()) {
        return true;
    }

    // The signature must be present.
    if (!j.contains("sig") || !j["sig"].is_string()) {
        THEMIS_WARN("RedisCacheCoordinator: received unsigned message – rejected "
                    "(hmac_secret is configured)");
        return false;
    }

    std::string received_sig = j["sig"].get<std::string>();

    // Re-compute the HMAC over the payload WITHOUT the "sig" field.
    // We reconstruct the unsigned JSON by removing the field, then dumping.
    try {
        nlohmann::json j_unsigned = j;
        j_unsigned.erase("sig");
        std::string unsigned_payload = j_unsigned.dump();

        std::string expected_sig = computeHmac(unsigned_payload);
        if (expected_sig.empty()) {
            return false;
        }

        // Constant-time comparison via CRYPTO_memcmp to prevent timing side-channels.
        if (received_sig.size() != expected_sig.size()) {
            THEMIS_WARN("RedisCacheCoordinator: HMAC verification failed (size mismatch)");
            return false;
        }
        if (CRYPTO_memcmp(received_sig.data(), expected_sig.data(),expected_sig.size()) != 0) {
            THEMIS_WARN("RedisCacheCoordinator: HMAC verification failed");
            return false;
        }
        return true;
    } catch (const std::exception &ex) {
        THEMIS_WARN("RedisCacheCoordinator: HMAC verification error: {}", ex.what());
        return false;
    }
}

std::optional<ReplicationMessage> HiredisRedisCacheCoordinator::deserializeMessage(const std::string &data) const {
    // Note: handleMessage() already parses and dispatches; this helper is
    // provided for callers that need a standalone deserialise step.
    try {
        auto j = nlohmann::json::parse(data);

        ReplicationMessage msg;
        std::string type_str = j.value("type", "");
        if (type_str == "ENTRY_PUT") {
            msg.type = ReplicationMessage::Type::ENTRY_PUT;
        } else if (type_str == "INVALIDATE") {
            msg.type = ReplicationMessage::Type::INVALIDATE;
        } else {
            return std::nullopt;
        }

        msg.key         = j.value("key", "");
        msg.tenant_id   = j.value("tenant_id", "");
        msg.ttl_seconds = j.value("ttl_seconds", 0);
        msg.result      = j.value("result", nlohmann::json{});

        return msg;
    } catch (const std::exception &ex) {
        THEMIS_WARN("RedisCacheCoordinator: JSON parse error: {}", ex.what());
        return std::nullopt;
    }
}

// ── Wave-2: Direct hiredis cache operations (THEMIS_HAS_HIREDIS) ─────────────
//
// When -DTHEMIS_HAS_HIREDIS=ON is set, the RedisDirectClient class provides a
// RAII-safe synchronous Redis client supporting SET/GET/DEL/EXPIRE.  These
// operations complement the pub/sub path above and allow callers that need
// direct key-value access to Redis (e.g. distributed TTL management) to do so
// without setting up a full coordinator.
//
// Usage (example):
//   RedisDirectClient client{"127.0.0.1", 6379, "auth_pw", 5000};
//   if (client.connected()) {
//       client.set("key", "value", 300);   // SET key value EX 300
//       auto v = client.get("key");        // GET key  → std::optional<std::string>
//       client.expire("key", 60);          // EXPIRE key 60
//       client.del("key");                 // DEL key
//   }
#ifdef THEMIS_HAS_HIREDIS

class RedisDirectClient {
public:
    RedisDirectClient(const std::string& host,
                      int                port,
                      const std::string& password   = {},
                      int                timeout_ms = 5000)
    {
        struct timeval tv;
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        ctx_ = redisConnectWithTimeout(host.c_str(), port, tv);
        if (!ctx_ || ctx_->err) {
            THEMIS_WARN("[RedisDirectClient] connect to {}:{} failed: {}",
                        host, port, ctx_ ? ctx_->errstr : "null context");
            if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
            return;
        }

        if (!password.empty()) {
            redisReply* r = static_cast<redisReply*>(
                redisCommand(ctx_, "AUTH %s", password.c_str()));
            if (r) {
                if (r->type == REDIS_REPLY_ERROR)
                    THEMIS_WARN("[RedisDirectClient] AUTH failed: {}", r->str);
                freeReplyObject(r);
            }
        }
        THEMIS_DEBUG("[RedisDirectClient] connected to {}:{}", host, port);
    }

    ~RedisDirectClient() {
        if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
    }

    // Non-copyable, movable.
    RedisDirectClient(const RedisDirectClient&) = delete;
    RedisDirectClient& operator=(const RedisDirectClient&) = delete;

    [[nodiscard]] bool connected() const noexcept { return ctx_ != nullptr; }

    bool set(const std::string& key,
             const std::string& value,
             int                ttl_secs = 0) noexcept {
        if (!ctx_) {
          return false;
        }
        redisReply* r = nullptr;
        if (ttl_secs > 0) {
            r = static_cast<redisReply*>(
                redisCommand(ctx_, "SET %b %b EX %d",
                             key.data(),key.size(),
                             value.data(),value.size(),
                             ttl_secs));
        } else {
            r = static_cast<redisReply*>(
                redisCommand(ctx_, "SET %b %b",
                             key.data(),key.size(),
                             value.data(),value.size()));
        }
        const bool ok = r && r->type != REDIS_REPLY_ERROR;
        if (r) {
          freeReplyObject(r);
        }
        return ok;
    }

    [[nodiscard]] std::optional<std::string> get(const std::string& key) noexcept {
        if (!ctx_) {
          return std::nullopt;
        }
        redisReply* r = static_cast<redisReply*>(
            redisCommand(ctx_, "GET %b", key.data(),key.size()));
        if (!r) {
          return std::nullopt;
        }
        std::optional<std::string> result = {};

        if (r->type == REDIS_REPLY_STRING)
            result = std::string(r->str, r->len);
        freeReplyObject(r);
        return result;
    }

    int del(const std::string& key) noexcept {
        if (!ctx_) {
          return -1;
        }
        redisReply* r = static_cast<redisReply*>(
            redisCommand(ctx_, "DEL %b", key.data(),key.size()));
        const int n = (r && r->type == REDIS_REPLY_INTEGER)
                          ? static_cast<int>(r->integer) : -1;
        if (r) {
          freeReplyObject(r);
        }
        return n;
    }

    int expire(const std::string& key, int seconds) noexcept {
        if (!ctx_) {
          return -1;
        }
        redisReply* r = static_cast<redisReply*>(
            redisCommand(ctx_, "EXPIRE %b %d", key.data(),key.size(), seconds));
        const int n = (r && r->type == REDIS_REPLY_INTEGER)
                          ? static_cast<int>(r->integer) : -1;
        if (r) {
          freeReplyObject(r);
        }
        return n;
    }

private:
    redisContext* ctx_ = nullptr;
};

#endif // THEMIS_HAS_HIREDIS

} // namespace cache
} // namespace themis
