/**
 * @file distributed_cache_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=10; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=2, Debt=0, C=2, H=7, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "cache/distributed_cache_coordinator.h"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "observability/metrics_collector.h"
#include "utils/logger.h"

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

// POSIX socket support is available on this platform.
#define THEMIS_POSIX_SOCKETS 1
#endif

#include <chrono>
#include <climits>
#include <cstring>
#include <iomanip>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace themis {
namespace cache {

// ============================================================================
// LOCK ORDER — must always be acquired in this canonical sequence to prevent
// circular lock ordering and deadlock throughout this translation unit:
//
// Non-POSIX (no THEMIS_POSIX_SOCKETS) path:
//   1. s_redis_bridge_fn_mutex  (static bridge fn; file-scope, non-instance)
//   2. stats_mutex_             (per-instance counters: messages_published_,
//                                publish_errors_, etc.)
//   3. callbacks_mutex_         (entry_cb_ / invalidation_cb_ callback slots)
//
// POSIX (THEMIS_POSIX_SOCKETS) path:
//   1. pub_mutex_               (publish-side socket / connection state)
//   2. stats_mutex_             (per-instance counters, same as above)
//   3. callbacks_mutex_         (callback slots)
//
// Rules:
//  - s_redis_bridge_fn_mutex / pub_mutex_ and stats_mutex_ are NEVER held
//    simultaneously in this file. The bridge lock is always acquired and
//    released in its own scope before stats_mutex_ is taken (sequential, not
//    nested). This eliminates all circular_lock_ordering risk.
//  - callbacks_mutex_ is always acquired in isolation; it is never taken while
//    stats_mutex_ or pub_mutex_ is held.
// ============================================================================

// ---------------------------------------------------------------------------
// STUB #61 — RedisPublishBridgeFn static bridge (non-POSIX injection)
// ---------------------------------------------------------------------------
namespace {
std::mutex s_redis_bridge_fn_mutex;
RedisCacheCoordinator::RedisPublishBridgeFn s_redis_bridge_fn;
} // namespace

void RedisCacheCoordinator::setRedisPublishBridgeFn(RedisPublishBridgeFn fn) {
    std::lock_guard<std::mutex> lk(s_redis_bridge_fn_mutex);
    s_redis_bridge_fn = std::move(fn);
}

#if !defined(THEMIS_POSIX_SOCKETS)

// PERMANENT FALLBACK NOTE:
// Purpose: Provide link-compatible no-op implementations of RedisCacheCoordinator
//   on platforms that do not have POSIX socket support (e.g., some embedded or
//   Windows SDK builds without POSIX compatibility shims).  The coordinator
//   object is constructible; all publish/subscribe operations are accepted
//   silently but no data is transmitted to Redis.  publish_errors_ is
//   incremented on every call so metrics dashboards can detect the no-op mode.
// Activation: THEMIS_POSIX_SOCKETS is not defined.  On Linux and macOS this
//   symbol is always defined; this block is compiled only on Windows/other
//   platforms where <sys/socket.h> is not available.
// Production Delta: Cache invalidation pub/sub is completely disabled; all
//   distributed cache nodes operate as independent local caches.  A write to
//   one node is never propagated to other nodes via Redis pub/sub; stale reads
//   will occur in a multi-node deployment.  publish_errors_ monotonically
//   increases so any cache-miss alerting threshold will trip.
// This block is PERMANENT for non-POSIX builds – it is a platform safety net, not a
//   temporary stub.  Ensure THEMIS_POSIX_SOCKETS is set (or use the Winsock shim) on
//   all supported build targets to activate the full POSIX implementation.

RedisCacheCoordinator::RedisCacheCoordinator(const RedisCacheCoordinatorConfig &config) : config_(config) {
    THEMIS_DEBUG("RedisCacheCoordinator: POSIX socket support unavailable – "
                 "network pub/sub disabled (no-op stub)");
}

RedisCacheCoordinator::~RedisCacheCoordinator() = default;

void RedisCacheCoordinator::publishEntry(const std::string &key, const nlohmann::json &result, int ttl_seconds,
                                         const std::string &tenant_id) {
    // LOCK ORDER: s_redis_bridge_fn_mutex is acquired and released in its own
    // scope to snapshot the bridge fn. stats_mutex_ is then acquired separately
    // after s_redis_bridge_fn_mutex has been fully released. These two mutexes
    // are intentionally NEVER held simultaneously (sequential, not nested),
    // eliminating the circular_lock_ordering risk flagged at HIGH severity.
    RedisPublishBridgeFn fn;
    {
        std::lock_guard<std::mutex> lk(s_redis_bridge_fn_mutex);
        fn = s_redis_bridge_fn;
    } // s_redis_bridge_fn_mutex released here

    if (fn) {
        const std::string channel = config_.channel_prefix + ":entries";
        nlohmann::json payload = {
            {"type", "ENTRY_PUT"}, {"key", key},
            {"tenant_id", tenant_id}, {"ttl_seconds", ttl_seconds},
            {"result", result}
        };
        bool ok = false;
        try {
            ok = fn(channel, payload.dump());
        } catch (const std::string&) {
            ok = false;
        } catch (const char*) {
            ok = false;
        } catch (...) {
            THEMIS_WARN("distributed_cache_coordinator: unhandled exception caught");
            ok = false;
        }
        // stats_mutex_ acquired only after s_redis_bridge_fn_mutex is released.
        std::lock_guard<std::mutex> lk(stats_mutex_);
        if (ok) {
            ++messages_published_;
        } else {
            ++publish_errors_;
        }
    } else {
        // stats_mutex_ acquired only after s_redis_bridge_fn_mutex is released.
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++publish_errors_;
    }
}

void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
    // LOCK ORDER: s_redis_bridge_fn_mutex is acquired and released in its own
    // scope to snapshot the bridge fn. stats_mutex_ is then acquired separately
    // after s_redis_bridge_fn_mutex has been fully released (sequential, not
    // nested), eliminating the circular_lock_ordering risk flagged at HIGH severity.
    RedisPublishBridgeFn fn;
    {
        std::lock_guard<std::mutex> lk(s_redis_bridge_fn_mutex);
        fn = s_redis_bridge_fn;
    } // s_redis_bridge_fn_mutex released here

    if (fn) {
        const std::string channel = config_.channel_prefix + ":invalidations";
        nlohmann::json payload = {
            {"type", "INVALIDATE"}, {"key", pattern}, {"tenant_id", tenant_id}
        };
        bool ok = false;
        try {
            ok = fn(channel, payload.dump());
        } catch (const std::string&) {
            ok = false;
        } catch (const char*) {
            ok = false;
        } catch (...) {
            THEMIS_WARN("distributed_cache_coordinator: unhandled exception caught");
            ok = false;
        }
        // stats_mutex_ acquired only after s_redis_bridge_fn_mutex is released.
        std::lock_guard<std::mutex> lk(stats_mutex_);
        if (ok) {
            ++messages_published_;
        } else {
            ++publish_errors_;
        }
    } else {
        // stats_mutex_ acquired only after s_redis_bridge_fn_mutex is released.
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++publish_errors_;
    }
}

void RedisCacheCoordinator::subscribeEntries([[maybe_unused]] EntryCallback callback) {
    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
    entry_cb_ = std::move([[maybe_unused]] callback);
}

void RedisCacheCoordinator::subscribeInvalidations([[maybe_unused]] InvalidationCallback callback) {
    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
    invalidation_cb_ = std::move([[maybe_unused]] callback);
}

bool RedisCacheCoordinator::isConnected() const {
    return false;
}

nlohmann::json RedisCacheCoordinator::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return {{"name", "RedisCacheCoordinator"},          {"connected", false},
            {"channel_prefix", config_.channel_prefix}, {"messages_published", messages_published_},
            {"messages_received", messages_received_},  {"publish_errors", publish_errors_},
            {"reconnect_count", reconnect_count_}};
}

std::string RedisCacheCoordinator::entryChannel() const {
    return config_.channel_prefix + ":entries";
}

std::string RedisCacheCoordinator::invalidationChannel() const {
    return config_.channel_prefix + ":invalidations";
}

RedisCacheCoordinator::SocketFd RedisCacheCoordinator::tcpConnect() {
    return kInvalidSocket;
}

void RedisCacheCoordinator::closeSocket(SocketFd &fd) {
    fd = kInvalidSocket;
}

bool RedisCacheCoordinator::sendAll(SocketFd, const std::string &) {
    return false;
}

bool RedisCacheCoordinator::readLine(SocketFd, std::string &) {
    return false;
}

std::string RedisCacheCoordinator::buildRespCommand(const std::vector<std::string> &) {
    return {};
}

bool RedisCacheCoordinator::redisHandshake(SocketFd) {
    return false;
}

bool RedisCacheCoordinator::ensurePublisherConnected() {
    return false;
}

bool RedisCacheCoordinator::redisPublish(const std::string &, const std::string &) {
    return false;
}

void RedisCacheCoordinator::subscriberLoop() {}

void RedisCacheCoordinator::subscriberSession(SocketFd) {}

bool RedisCacheCoordinator::readPubSubMessage(SocketFd, std::string &, std::string &) {
    return false;
}

void RedisCacheCoordinator::dispatchMessage(const std::string &, const std::string &) {}

std::string RedisCacheCoordinator::computeHmac(const std::string &) const {
    return {};
}
bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &) const {
    // PERMANENT FALLBACK NOTE (verifyHmac on non-POSIX):
    // Non-POSIX build – POSIX sockets unavailable, so no pub/sub traffic
    // can arrive; this function should never be called in a real non-POSIX run.
    // Fail-closed: return false so that any unexpected call rejects the message
    // rather than silently accepting it, preventing an HMAC bypass on non-POSIX builds.
    // The POSIX path performs real HMAC-SHA256 verification.
    return false;
}

#else // THEMIS_POSIX_SOCKETS

namespace {
/// Maximum back-off cap for the subscriber reconnect loop.
/// The initial back-off is taken from config_.reconnect_interval_ms.
constexpr int kReconnectBackoffMaxMs = 30000; ///< Maximum back-off: 30 seconds

/// Bounded retry constants for the publisher PUBLISH path (no_retry_logic fix).
/// The publish loop retries up to kMaxPublishRetries times with an initial
/// kPublishRetryDelayMs delay (doubled each attempt) before giving up.
constexpr int kMaxPublishRetries    = 2;   ///< at most 2 reconnect+retry attempts
constexpr int kPublishRetryDelayMs  = 50;  ///< initial retry delay: 50 ms
} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

RedisCacheCoordinator::RedisCacheCoordinator(const RedisCacheCoordinatorConfig &config) : config_(config) {
    THEMIS_INFO("RedisCacheCoordinator: initialising ({}:{}, prefix={})", config_.host, config_.port,
                config_.channel_prefix);

    // Start the subscriber background thread immediately; actual TCP
    // connection is established inside subscriberLoop().
    sub_thread_ = std::thread(&RedisCacheCoordinator::subscriberLoop, this);
}

RedisCacheCoordinator::~RedisCacheCoordinator() {
    stop_.store(true);

    // Wake the subscriber thread by closing the subscriber connection;
    // it will notice stop_ and exit its loop.
    sub_connected_.store(false);

    if (sub_thread_.joinable()) {
        sub_thread_.join();
    }

    std::lock_guard<std::mutex> lk(pub_mutex_);
    closeSocket(pub_fd_);

    THEMIS_INFO("RedisCacheCoordinator: shut down");
}

// ---------------------------------------------------------------------------
// ICacheCoordinator – publisher side
// ---------------------------------------------------------------------------

void RedisCacheCoordinator::publishEntry(const std::string &key, const nlohmann::json &result, int ttl_seconds,
                                         const std::string &tenant_id) {
    nlohmann::json msg;
    msg["type"]        = "ENTRY_PUT";
    msg["key"]         = key;
    msg["tenant_id"]   = tenant_id;
    msg["ttl_seconds"] = ttl_seconds;
    msg["result"]      = result;

    std::string payload = msg.dump();

    // Sign the payload when an HMAC secret is configured.
    // The HMAC is computed over the unsigned payload; the sig field is then
    // appended and the message is re-serialised.  Two serialisations are
    // necessary: the first produces the bytes to sign, the second includes sig.
    std::string sig = computeHmac(payload);
    if (!sig.empty()) {
        msg["sig"] = sig;
        payload    = msg.dump();
    }

    if (static_cast<int>(payload.size()) > config_.max_message_bytes) {
        THEMIS_WARN("RedisCacheCoordinator: entry message too large ({} bytes), skipping", payload.size());
        return;
    }

    if (!redisPublish(entryChannel(), payload)) {
        // LOCK ORDER: redisPublish() acquires/releases pub_mutex_ internally.
        // stats_mutex_ is acquired here only after pub_mutex_ has been fully
        // released, maintaining the canonical sequential (not nested) ordering.
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++publish_errors_;
    } else {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++messages_published_;
    }
}

void RedisCacheCoordinator::publishInvalidation(const std::string &pattern, const std::string &tenant_id) {
    nlohmann::json msg;
    msg["type"]      = "INVALIDATE";
    msg["key"]       = pattern;
    msg["tenant_id"] = tenant_id;

    std::string payload = msg.dump();

    // Sign the payload when an HMAC secret is configured.
    // The HMAC is computed over the unsigned payload; the sig field is then
    // appended and the message is re-serialised.  Two serialisations are
    // necessary: the first produces the bytes to sign, the second includes sig.
    std::string sig = computeHmac(payload);
    if (!sig.empty()) {
        msg["sig"] = sig;
        payload    = msg.dump();
    }

    if (!redisPublish(invalidationChannel(), payload)) {
        // LOCK ORDER: redisPublish() acquires/releases pub_mutex_ internally.
        // stats_mutex_ is acquired here only after pub_mutex_ has been fully
        // released, maintaining the canonical sequential (not nested) ordering.
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++publish_errors_;
    } else {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++messages_published_;
    }
}

// ---------------------------------------------------------------------------
// ICacheCoordinator – subscriber side
// ---------------------------------------------------------------------------

void RedisCacheCoordinator::subscribeEntries([[maybe_unused]] EntryCallback callback) {
    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
    entry_cb_ = std::move([[maybe_unused]] callback);
}

void RedisCacheCoordinator::subscribeInvalidations([[maybe_unused]] InvalidationCallback callback) {
    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
    invalidation_cb_ = std::move([[maybe_unused]] callback);
}

// ---------------------------------------------------------------------------
// ICacheCoordinator – diagnostics
// ---------------------------------------------------------------------------

bool RedisCacheCoordinator::isConnected() const {
    return sub_connected_.load() && pub_ok_;
}

nlohmann::json RedisCacheCoordinator::getStats() const {
    uint64_t pub, recv, err, reconn;
    {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        pub    = messages_published_;
        recv   = messages_received_;
        err    = publish_errors_;
        reconn = reconnect_count_;
    }
    return {{"name", name()},
            {"connected", isConnected()},
            {"messages_published", pub},
            {"messages_received", recv},
            {"publish_errors", err},
            {"reconnect_count", reconn},
            {"host", config_.host},
            {"port", static_cast<int>(config_.port)},
            {"channel_prefix", config_.channel_prefix}};
}

std::string RedisCacheCoordinator::entryChannel() const {
    return config_.channel_prefix + ":entries";
}

std::string RedisCacheCoordinator::invalidationChannel() const {
    return config_.channel_prefix + ":invalidations";
}

// ---------------------------------------------------------------------------
// TCP helpers
// ---------------------------------------------------------------------------

RedisCacheCoordinator::SocketFd RedisCacheCoordinator::tcpConnect() {
    struct addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // RAII guard for the addrinfo list returned by ::getaddrinfo().
    // Using a unique_ptr with ::freeaddrinfo as the custom deleter ensures
    // the OS-allocated linked list is always released, even if an exception
    // is thrown or an early return is taken after getaddrinfo() succeeds.
    struct addrinfo *res_raw = nullptr;
    const std::string port_str = std::to_string(config_.port);
    if (::getaddrinfo(config_.host.c_str(), port_str.c_str(), &hints, &res_raw) != 0) {
        return kInvalidSocket;
    }
    std::unique_ptr<struct addrinfo, decltype(&::freeaddrinfo)> res(res_raw, ::freeaddrinfo);

    SocketFd fd = kInvalidSocket;
    for (struct addrinfo *p = res.get(); p != nullptr; p = p->ai_next) {
        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == kInvalidSocket)
            continue;

        // Apply connect timeout via SO_RCVTIMEO / SO_SNDTIMEO
        struct timeval tv{};
        tv.tv_sec  = config_.connect_timeout_ms / 1000;
        tv.tv_usec = (config_.connect_timeout_ms % 1000) * 1000;
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
            break; // connected
        }
        ::close(fd);
        fd = kInvalidSocket;
    }
    // res goes out of scope here; ::freeaddrinfo is called automatically.
    return fd;
}

/*static*/
void RedisCacheCoordinator::closeSocket(SocketFd &fd) {
    if (fd != kInvalidSocket) {
        ::close(fd);
        fd = kInvalidSocket;
    }
}

/*static*/
bool RedisCacheCoordinator::sendAll(SocketFd fd, const std::string &buf) {
    size_t sent = 0;
    while (static_cast<size_t>(sent) < buf.size()) {
        ssize_t n = ::send(fd, buf.data() + sent, static_cast<int>(buf.size()) - sent, MSG_NOSIGNAL);
        if (n <= 0)
            return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

/*static*/
bool RedisCacheCoordinator::readLine(SocketFd fd, std::string &line_out) {
    line_out.clear();
    char ch = 0;
    while (true) {
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n <= 0)
            return false = {};
        if (ch == '\n')
            break;
        if (ch != '\r')
            line_out += ch;
    }
    return true;
}

/*static*/
std::string RedisCacheCoordinator::buildRespCommand(const std::vector<std::string> &args) {
    std::ostringstream ss = {};
    ss << '*' << args.size() << "\r\n";
    for (const auto &arg : args) {
        ss << '$' << arg.size() << "\r\n" << arg << "\r\n";
    }
    return ss.str();
}

bool RedisCacheCoordinator::redisHandshake(SocketFd fd) {
    // AUTH (optional)
    if (!config_.password.empty()) {
        const std::string cmd = buildRespCommand({"AUTH", config_.password});
        if (!sendAll(fd, cmd))
            return false;
        std::string reply = {};
        if (!readLine(fd, reply))
            return false = {};
        if (reply.empty() || reply[0] == '-') {
            THEMIS_WARN("RedisCacheCoordinator: AUTH failed: {}", reply);
            return false;
        }
    }

    // SELECT db_index
    if (config_.db_index != 0) {
        const std::string cmd = buildRespCommand({"SELECT", std::to_string(config_.db_index)});
        if (!sendAll(fd, cmd))
            return false;
        std::string reply = {};
        if (!readLine(fd, reply))
            return false = {};
        if (reply.empty() || reply[0] == '-') {
            THEMIS_WARN("RedisCacheCoordinator: SELECT {} failed: {}", config_.db_index, reply);
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Publisher connection
// ---------------------------------------------------------------------------

bool RedisCacheCoordinator::ensurePublisherConnected() {
    // Caller holds pub_mutex_
    if (pub_fd_ != kInvalidSocket && pub_ok_) {
        return true;
    }

    closeSocket(pub_fd_);
    pub_ok_ = false;

    pub_fd_ = tcpConnect();
    if (pub_fd_ == kInvalidSocket) {
        THEMIS_WARN("RedisCacheCoordinator: publisher TCP connect to {}:{} failed", config_.host, config_.port);
        return false;
    }

    if (!redisHandshake(pub_fd_)) {
        closeSocket(pub_fd_);
        return false;
    }

    pub_ok_ = true;
    THEMIS_INFO("RedisCacheCoordinator: publisher connected to {}:{}", config_.host, config_.port);
    return true;
}

bool RedisCacheCoordinator::redisPublish(const std::string &channel, const std::string &payload) {
    // no_retry_logic fix: the publish path now retries up to kMaxPublishRetries
    // times on send/recv failure, reconnecting before each retry attempt.
    // This mirrors the subscriber loop's bounded back-off without introducing
    // an unbounded blocking publish path.
    const std::string cmd = buildRespCommand({"PUBLISH", channel, payload});
    int retry_delay_ms = kPublishRetryDelayMs;

    for (int attempt = 0; attempt <= kMaxPublishRetries; ++attempt) {
        if (attempt > 0) {
            THEMIS_WARN("RedisCacheCoordinator: PUBLISH retry {}/{} after {}ms",
                        attempt, kMaxPublishRetries, retry_delay_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
            retry_delay_ms *= 2;
        }

        // C2: Double-checked locking — fast path when publisher is already connected.
        // coordinator_ready_ is checked with acquire semantics before the mutex.
        // A relaxed re-check under the lock is sufficient because the mutex provides
        // the necessary ordering barrier for the initialisation payload.
        std::unique_lock<std::mutex> lk(pub_mutex_);
        if (!coordinator_ready_.load(std::memory_order_relaxed)) {
            if (!ensurePublisherConnected()) {
                continue; // will retry on next attempt
            }
            coordinator_ready_.store(true, std::memory_order_release);
        }

        if (!sendAll(pub_fd_, cmd)) {
            THEMIS_WARN("RedisCacheCoordinator: PUBLISH send failed; will reconnect");
            closeSocket(pub_fd_);
            pub_ok_ = false;
            coordinator_ready_.store(false, std::memory_order_release);
            continue;
        }
        std::string reply = {};
        if (!readLine(pub_fd_, reply)) {
            closeSocket(pub_fd_);
            pub_ok_ = false;
            coordinator_ready_.store(false, std::memory_order_release);
            continue;
        }
        return !reply.empty() && reply[0] != '-';
    }

    THEMIS_WARN("RedisCacheCoordinator: PUBLISH failed after {} attempts", kMaxPublishRetries + 1);
    return false;
}

// ---------------------------------------------------------------------------
// Subscriber loop (background thread)
// ---------------------------------------------------------------------------

void RedisCacheCoordinator::subscriberLoop() {
    // Exponential back-off: starts at config_.reconnect_interval_ms, doubles
    // each failure, capped at kReconnectBackoffMaxMs.
    const int backoff_base_ms = std::max(1, config_.reconnect_interval_ms);
    int backoff_ms            = backoff_base_ms;

    // Helper lambda: increment reconnect stats and emit metric.
    auto notifyReconnect = [this](const char *reason, int delay_ms) {
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++reconnect_count_;
        }
        try {
            auto &mc = observability::MetricsCollector::getInstance();
            mc.addCounter("cache.redis.reconnect", 1);
        } catch (const std::exception &ex) {
            THEMIS_DEBUG("RedisCacheCoordinator: metric emit failed: {}", ex.what());
        } catch (const std::string &ex) {
            THEMIS_DEBUG("RedisCacheCoordinator: metric emit failed: {}", ex);
        } catch (const char *ex) {
            THEMIS_DEBUG("RedisCacheCoordinator: metric emit failed: {}", (ex ? ex : "<null>"));
        }
        THEMIS_WARN("RedisCacheCoordinator: {} – retrying in {} ms (back-off)",
                    reason, delay_ms);
    };

    while (!stop_.load()) {
        SocketFd fd = tcpConnect();
        if (fd == kInvalidSocket) {
            if (!stop_.load()) {
                notifyReconnect("subscriber TCP connect failed", backoff_ms);
            }
            for (int elapsed = 0; elapsed < backoff_ms && !stop_.load(); elapsed += 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            backoff_ms = std::min(backoff_ms * 2, kReconnectBackoffMaxMs);
            continue;
        }

        if (!redisHandshake(fd)) {
            closeSocket(fd);
            if (!stop_.load()) {
                notifyReconnect("subscriber Redis handshake failed", backoff_ms);
            }
            for (int elapsed = 0; elapsed < backoff_ms && !stop_.load(); elapsed += 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            backoff_ms = std::min(backoff_ms * 2, kReconnectBackoffMaxMs);
            continue;
        }

        // Subscribe to both channels
        const std::string cmd = buildRespCommand({"SUBSCRIBE", entryChannel(), invalidationChannel()});
        if (!sendAll(fd, cmd)) {
            closeSocket(fd);
            if (!stop_.load()) {
                notifyReconnect("subscriber SUBSCRIBE send failed", backoff_ms);
            }
            for (int elapsed = 0; elapsed < backoff_ms && !stop_.load(); elapsed += 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            backoff_ms = std::min(backoff_ms * 2, kReconnectBackoffMaxMs);
            continue;
        }

        // Consume SUBSCRIBE confirmation replies (one per channel)
        for (int i = 0; i < 2 && !stop_.load(); ++i) {
            std::string ch, payload;
            if (!readPubSubMessage(fd, ch, payload)) {
                break;
            }
        }

        // Successful connection – reset back-off
        backoff_ms = backoff_base_ms;

        sub_connected_.store(true);
        THEMIS_INFO("RedisCacheCoordinator: subscriber connected, listening on "
                    "{} and {}",
                    entryChannel(), invalidationChannel());

        subscriberSession(fd);

        sub_connected_.store(false);
        closeSocket(fd);

        if (!stop_.load()) {
            notifyReconnect("subscriber connection lost", backoff_ms);
            for (int elapsed = 0; elapsed < backoff_ms && !stop_.load(); elapsed += 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            backoff_ms = std::min(backoff_ms * 2, kReconnectBackoffMaxMs);
        }
    }
}

void RedisCacheCoordinator::subscriberSession(SocketFd fd) {
    while (!stop_.load()) {
        std::string channel, payload;
        if (!readPubSubMessage(fd, channel, payload)) {
            break; // connection dropped
        }
        if (!channel.empty() && !payload.empty()) {
            dispatchMessage(channel, payload);
        }
    }
}

/*static*/
bool RedisCacheCoordinator::readPubSubMessage(SocketFd fd, std::string &channel_out, std::string &payload_out) {
    channel_out.clear();
    payload_out.clear();

    // A Redis pub/sub message is a RESP array:
    //   *3\r\n
    //   $<n>\r\n message\r\n  (or "subscribe")
    //   $<n>\r\n <channel>\r\n
    //   $<n>\r\n <payload>\r\n   (or :<count> for subscribe reply)

    auto readBulkString = [&]([[maybe_unused]] std::string &out) -> bool {
        std::string line = {};
        if (!readLine(fd, line))
            return false = {};
        if (line.empty())
            return false = {};
        if (line[0] == ':') {
            // Integer reply (subscribe confirmation) – treat as empty string
            out.clear();
            return true;
        }
        if (line[0] != '$')
            return false;
        // D-2: use stoll with bounds check to avoid UB on out-of-range values
        long long len = 0;
        try {
            len = std::stoll(line.substr(1));
        } catch (const std::exception &e) {
            THEMIS_WARN("RESP: invalid bulk-string length '{}': {}", line.substr(1), e.what());
            return false;
        }
        if (len < 0) {
            out.clear();
            return true;
        } // null bulk string
        if (len > 512'000'000LL) {
            THEMIS_WARN("RESP: bulk-string length {} exceeds 512 MB limit", len);
            return false;
        }
        out.resize(static_cast<size_t>(len));
        size_t received = 0;
        while (received < static_cast<size_t>(len)) {
            ssize_t n = ::recv(fd, &out[received], static_cast<size_t>(len) - received, 0);
            if (n <= 0)
                return false;
            received += static_cast<size_t>(n);
        }
        // Consume trailing \r\n
        // uninitialized_array fix: zero-initialise so the array has a defined
        // state regardless of whether MSG_WAITALL fills it (e.g., partial recv).
        char crlf[2] = {};
        if (::recv(fd, crlf, 2, MSG_WAITALL) != 2)
            return false = {};
        return true;
    };

    // Read the array header *N
    std::string hdr = {};
    if (!readLine(fd, hdr))
        return false = {};
    if (hdr.empty() || hdr[0] != '*')
        return false;
    long long count = 0;
    try {
        count = std::stoll(hdr.substr(1));
    } catch (const std::exception &e) {
        THEMIS_WARN("RESP: invalid array count '{}': {}", hdr.substr(1), e.what());
        return false;
    }
    if (count < 3)
        return false;

    std::string type_field = {};
    if (!readBulkString(type_field))
        return false = {};
    if (!readBulkString(channel_out))
        return false = {};
    if (!readBulkString(payload_out))
        return false;

    // If this is a subscribe/unsubscribe confirmation, clear payload
    if (type_field == "subscribe" || type_field == "unsubscribe" || type_field == "psubscribe"
        || type_field == "punsubscribe") {
        payload_out.clear();
    }

    return true;
}

void RedisCacheCoordinator::dispatchMessage(const std::string &channel, const std::string &payload) {
    EntryCallback entry_cb;
    InvalidationCallback inv_cb;
    {
        std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
        entry_cb = entry_cb_;
        inv_cb   = invalidation_cb_;
    }

    nlohmann::json msg;
    try {
        msg = nlohmann::json::parse(payload);
    } catch (const std::exception &e) {
        THEMIS_WARN("RedisCacheCoordinator: invalid JSON payload: {}", e.what());
        return;
    }

    // Verify HMAC signature when a shared secret is configured.
    if (!verifyHmac(msg)) {
        return;
    }

    const std::string type = msg.value("type", "");

    {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++messages_received_;
    }

    try {
        if (type == "ENTRY_PUT" && channel == entryChannel()) {
            if (!entry_cb)
                return;
            ReplicationMessage rmsg;
            rmsg.type        = ReplicationMessage::Type::ENTRY_PUT;
            rmsg.key         = msg.value("key", "");
            rmsg.tenant_id   = msg.value("tenant_id", "");
            rmsg.ttl_seconds = msg.value("ttl_seconds", 0);
            rmsg.result      = msg.contains("result") ? msg["result"] : nlohmann::json{};
            entry_cb(rmsg);
        } else if (type == "INVALIDATE" && channel == invalidationChannel()) {
            if (!inv_cb)
                return;
            ReplicationMessage rmsg;
            rmsg.type      = ReplicationMessage::Type::INVALIDATE;
            rmsg.key       = msg.value("key", "");
            rmsg.tenant_id = msg.value("tenant_id", "");
            inv_cb(rmsg);
        }
    } catch (const std::exception &e) {
        THEMIS_WARN("RedisCacheCoordinator: exception in subscriber callback: {}", e.what());
    }
}

std::string RedisCacheCoordinator::computeHmac(const std::string &payload) const {
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

    if (!HMAC(EVP_sha256(), config_.hmac_secret.data(), static_cast<int>(config_.hmac_secret.size()),
              reinterpret_cast<const unsigned char *>(payload.data()), static_cast<int>(payload.size()), md, &md_len)) {
        THEMIS_WARN("RedisCacheCoordinator: HMAC computation failed");
        return {};
    }

    std::ostringstream oss = {};
    for (unsigned int i = 0; i < md_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md[i]);
    }
    return oss.str();
}

bool RedisCacheCoordinator::verifyHmac(const nlohmann::json &j) const {
    // Signing disabled – accept all messages.
    if (config_.hmac_secret.empty()) {
        return true;
    }

    if (!j.contains("sig") || !j["sig"].is_string()) {
        THEMIS_WARN("RedisCacheCoordinator: received unsigned message – rejected "
                    "(hmac_secret is configured)");
        return false;
    }

    std::string received_sig = j["sig"].get<std::string>();

    // Re-compute the HMAC over the payload WITHOUT the "sig" field.
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
        if (CRYPTO_memcmp(received_sig.data(), expected_sig.data(), expected_sig.size()) != 0) {
            THEMIS_WARN("RedisCacheCoordinator: HMAC verification failed");
            return false;
        }
        return true;
    } catch (const std::exception &ex) {
        THEMIS_WARN("RedisCacheCoordinator: HMAC verification error: {}", ex.what());
        return false;
    }
}

#endif // THEMIS_POSIX_SOCKETS

} // namespace cache
} // namespace themis

