/**
 * @file qos_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Network QoS Manager – Implementation
// Token Bucket rate limiting, Priority Queues, and Backpressure control

#include "network/qos_manager.h"
#include "utils/logger.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <thread>

#if defined(__linux__)
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <spawn.h>
extern char** environ;  // POSIX environ for posix_spawn
#endif

namespace {

/// @brief Validate a network interface name for use in shell-adjacent tc calls.
/// Allows only: ASCII alphanumeric, hyphen, underscore, dot — max 15 chars.
/// Rejects leading '-' to prevent argument injection (e.g. "--help").
/// This is a defence-in-depth guard; the primary protection is posix_spawn().
static bool isValidInterfaceName(std::string_view iface) noexcept {
    if (iface.empty() || iface.size() > 15) return false;
    if (iface.front() == '-') return false;
    return std::all_of(iface.begin(), iface.end(), [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) ||
               c == '-' || c == '_' || c == '.';
    });
}

#if defined(__linux__)
/// @brief Execute a tc(8) command via posix_spawn() — no shell involved.
/// @param tc_bin  Absolute path to the tc binary (already validated by access()).
/// @param argv    Null-terminated argument array; argv[0] must equal tc_bin.
/// @return true on success (exit status 0), false otherwise.
static bool runTcCommand(const char* tc_bin, char* const argv[]) noexcept {
    pid_t pid = 0;
    if (::posix_spawn(&pid, tc_bin, nullptr, nullptr, argv, environ) != 0) {
        return false;
    }
    int status = 0;
    // Retry EINTR from waitpid (signal safety).
    while (::waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif

}  // anonymous namespace

namespace themis {
namespace network {

// =============================================================================
// TokenBucket
// =============================================================================

TokenBucket::TokenBucket(uint64_t rate_bps, uint64_t burst_bytes)
    : tokens_(static_cast<double>(burst_bytes))
    , rate_bps_(rate_bps)
    , burst_bytes_(burst_bytes)
    , last_refill_(std::chrono::steady_clock::now())
{}

void TokenBucket::refill() {
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - last_refill_).count();
    last_refill_ = now;

    if (rate_bps_ > 0) {
        double rate_bytes_per_sec = static_cast<double>(rate_bps_) / 8.0;
        tokens_ += elapsed * rate_bytes_per_sec;
        tokens_  = std::min(tokens_, static_cast<double>(burst_bytes_));
    } else {
        // Unlimited – always full
        tokens_ = static_cast<double>(burst_bytes_);
    }
}

bool TokenBucket::tryConsume(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    refill();

    if (rate_bps_ == 0) {
        // Unlimited bandwidth
        return true;
    }

    if (tokens_ >= static_cast<double>(bytes)) {
        tokens_ -= static_cast<double>(bytes);
        return true;
    }
    return false;
}

bool TokenBucket::consume(uint64_t bytes, std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true) {
        if (tryConsume(bytes)) {
            return true;
        }

        auto now = std::chrono::steady_clock::now();
        if (timeout.count() == 0 || now >= deadline) {
            return false;
        }

        // Sleep for a short interval proportional to the deficit
        double rate_bytes_per_sec;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            rate_bytes_per_sec = static_cast<double>(rate_bps_) / 8.0;
        }

        double deficit = static_cast<double>(bytes) - availableBytes();
        if (rate_bytes_per_sec > 0.0 && deficit > 0.0) {
            auto wait_ms = static_cast<int64_t>((deficit / rate_bytes_per_sec) * 1000.0);
            wait_ms      = std::max<int64_t>(1, std::min(wait_ms, static_cast<int64_t>(10)));
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void TokenBucket::reconfigure(uint64_t rate_bps, uint64_t burst_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    rate_bps_    = rate_bps;
    burst_bytes_ = burst_bytes;
    // Clamp current tokens to new burst
    tokens_ = std::min(tokens_, static_cast<double>(burst_bytes_));
}

double TokenBucket::availableBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Perform a non-destructive refill estimate
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - last_refill_).count();

    if (rate_bps_ == 0) {
        return static_cast<double>(burst_bytes_);
    }

    double rate_bytes_per_sec = static_cast<double>(rate_bps_) / 8.0;
    double estimated          = tokens_ + elapsed * rate_bytes_per_sec;
    return std::min(estimated, static_cast<double>(burst_bytes_));
}

uint64_t TokenBucket::rateBps() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rate_bps_;
}

uint64_t TokenBucket::burstBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return burst_bytes_;
}

// =============================================================================
// LeakyBucket
// =============================================================================

LeakyBucket::LeakyBucket(uint64_t drain_rate_bps, uint64_t capacity_bytes)
    : fill_(0.0)
    , drain_rate_bps_(drain_rate_bps)
    , capacity_bytes_(capacity_bytes)
    , last_drain_(std::chrono::steady_clock::now())
{}

void LeakyBucket::drain() {
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - last_drain_).count();
    last_drain_  = now;

    if (drain_rate_bps_ > 0) {
        double drain_bytes_per_sec = static_cast<double>(drain_rate_bps_) / 8.0;
        fill_ -= elapsed * drain_bytes_per_sec;
        if (fill_ < 0.0) {
            fill_ = 0.0;
        }
    }
}

bool LeakyBucket::add(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    drain();

    double new_fill = fill_ + static_cast<double>(bytes);
    if (new_fill > static_cast<double>(capacity_bytes_)) {
        // Overflow: accept but report non-conformant
        fill_ = static_cast<double>(capacity_bytes_);
        return false;
    }
    fill_ = new_fill;
    return true;
}

bool LeakyBucket::tryConform(uint64_t bytes) const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Estimate fill after draining elapsed time
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - last_drain_).count();

    double estimated_fill = fill_;
    if (drain_rate_bps_ > 0) {
        double drain_bytes_per_sec = static_cast<double>(drain_rate_bps_) / 8.0;
        estimated_fill -= elapsed * drain_bytes_per_sec;
        if (estimated_fill < 0.0) {
            estimated_fill = 0.0;
        }
    }

    return (estimated_fill + static_cast<double>(bytes)) <=
           static_cast<double>(capacity_bytes_);
}

void LeakyBucket::reconfigure(uint64_t drain_rate_bps, uint64_t capacity_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    drain_rate_bps_ = drain_rate_bps;
    capacity_bytes_ = capacity_bytes;
    if (fill_ > static_cast<double>(capacity_bytes_)) {
        fill_ = static_cast<double>(capacity_bytes_);
    }
}

double LeakyBucket::currentFill() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return fill_;
}

uint64_t LeakyBucket::capacityBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return capacity_bytes_;
}

uint64_t LeakyBucket::drainRateBps() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return drain_rate_bps_;
}

// =============================================================================
// CongestionController
// =============================================================================

CongestionController::CongestionController()
    : cwnd_(kDefaultInitialCwnd)
    , ssthresh_(kMaxCwnd)
    , srtt_(std::chrono::microseconds(0))
    , in_slow_start_(true)
{}

void CongestionController::recordAck(uint64_t bytes_acked,
                                      std::chrono::microseconds rtt) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Update smoothed RTT (Jacobson/Karels algorithm, alpha = 0.125)
    if (srtt_.count() == 0) {
        srtt_ = rtt;
    } else {
        // SRTT = (1 - alpha) * SRTT + alpha * RTT  (alpha = 1/8)
        auto new_srtt = srtt_.count() - (srtt_.count() >> 3) + (rtt.count() >> 3);
        srtt_ = std::chrono::microseconds(new_srtt);
    }

    if (in_slow_start_) {
        // Slow start: increase cwnd by bytes_acked (doubles each RTT)
        cwnd_ += bytes_acked;
        if (cwnd_ >= ssthresh_) {
            in_slow_start_ = false;
        }
    } else {
        // Congestion avoidance: increase by MSS^2 / cwnd (approximately 1 MSS/RTT)
        uint64_t increase = (kDefaultMss * kDefaultMss) / std::max(cwnd_, kDefaultMss);
        cwnd_ += std::max(increase, uint64_t{1});
    }

    if (cwnd_ > kMaxCwnd) {
        cwnd_ = kMaxCwnd;
    }
}

void CongestionController::recordLoss() {
    std::lock_guard<std::mutex> lock(mutex_);
    ssthresh_      = std::max(cwnd_ / 2, kDefaultMss * 2);
    cwnd_          = ssthresh_;
    in_slow_start_ = false;
}

uint64_t CongestionController::cwnd() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cwnd_;
}

uint64_t CongestionController::ssthresh() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ssthresh_;
}

std::chrono::microseconds CongestionController::smoothedRtt() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return srtt_;
}

void CongestionController::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    cwnd_          = kDefaultInitialCwnd;
    ssthresh_      = kMaxCwnd;
    srtt_          = std::chrono::microseconds(0);
    in_slow_start_ = true;
}

// =============================================================================
// QoSManager
// =============================================================================

QoSManager::QoSManager(const Config& config)
    : config_(config)
{
    // Initialise per-priority counters
    bytes_per_priority_[Priority::CRITICAL] = 0;
    bytes_per_priority_[Priority::HIGH]     = 0;
    bytes_per_priority_[Priority::MEDIUM]   = 0;
    bytes_per_priority_[Priority::LOW]      = 0;
}

QoSManager::QoSManager()
    : QoSManager(Config{})
{}

QoSManager::~QoSManager() = default;

// -------------------------------------------------------------------------
// Internal helpers
// -------------------------------------------------------------------------

std::shared_ptr<QoSManager::ConnectionState>
QoSManager::findConnection(uint64_t id) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(id);
    if (it == connections_.end()) {
        return nullptr;
    }
    return it->second;
}

// -------------------------------------------------------------------------
// Connection lifecycle
// -------------------------------------------------------------------------

void QoSManager::registerConnection(uint64_t connection_id, Priority priority) {
    auto state            = std::make_shared<ConnectionState>();
    state->connection_id  = connection_id;
    state->priority.store(static_cast<uint8_t>(priority), std::memory_order_relaxed);

    uint64_t effective_rate = effectiveDefaultRateBps();
    if (effective_rate > 0) {
        uint64_t burst = config_.default_burst_bytes > 0
                             ? config_.default_burst_bytes
                             : effective_rate / 8;  // 1 s worth of data
        state->token_bucket = std::make_shared<TokenBucket>(effective_rate, burst);
    }

    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_[connection_id] = std::move(state);
}

void QoSManager::unregisterConnection(uint64_t connection_id) {
    // Clean up tenant assignment and decrement tenant connection count
    std::string old_tenant_id;
    {
        std::lock_guard<std::mutex> lock(tenant_assignments_mutex_);
        auto it = tenant_assignments_.find(connection_id);
        if (it != tenant_assignments_.end()) {
            old_tenant_id = it->second;
            tenant_assignments_.erase(it);
        }
    }
    if (!old_tenant_id.empty()) {
        auto ts = findTenant(old_tenant_id);
        if (ts && ts->active_connections.load(std::memory_order_relaxed) > 0) {
            ts->active_connections.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(connection_id);
}

// -------------------------------------------------------------------------
// Per-connection controls
// -------------------------------------------------------------------------

void QoSManager::setPriority(uint64_t connection_id, Priority priority) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }
    state->priority.store(static_cast<uint8_t>(priority), std::memory_order_relaxed);
}

void QoSManager::setTokenBucket(uint64_t connection_id,
                                 uint64_t rate_bps,
                                 uint64_t burst_bytes) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }

    uint64_t burst = burst_bytes > 0 ? burst_bytes : config_.default_burst_bytes;
    if (burst == 0 && rate_bps > 0) {
        burst = rate_bps / 8;  // Default burst = 1 second of sustained rate
    }

    std::lock_guard<std::mutex> lock(state->token_bucket_mutex);
    if (state->token_bucket) {
        state->token_bucket->reconfigure(rate_bps, burst);
    } else {
        state->token_bucket = std::make_shared<TokenBucket>(rate_bps, burst);
    }
}

void QoSManager::clearTokenBucket(uint64_t connection_id) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }
    std::lock_guard<std::mutex> lock(state->token_bucket_mutex);
    state->token_bucket.reset();
}

// -------------------------------------------------------------------------
// Helper: resolve effective bandwidth values from config
// -------------------------------------------------------------------------

uint64_t QoSManager::effectiveMaxBandwidthBps() const {
    if (config_.max_bandwidth_mbps > 0) {
        return config_.max_bandwidth_mbps * 1'000'000ULL;
    }
    return config_.max_bandwidth_bps;
}

uint64_t QoSManager::effectiveDefaultRateBps() const {
    if (config_.per_connection_limit_mbps > 0) {
        return config_.per_connection_limit_mbps * 1'000'000ULL;
    }
    return config_.default_rate_bps;
}

// -------------------------------------------------------------------------
// Snake-case API
// -------------------------------------------------------------------------

void QoSManager::set_bandwidth_limit(uint64_t connection_id,
                                      uint64_t bytes_per_second) {
    // Convert bytes/sec → bits/sec; burst = 1 second of sustained throughput
    uint64_t rate_bps    = bytes_per_second * 8;
    uint64_t burst_bytes = bytes_per_second;  // 1 second worth
    setTokenBucket(connection_id, rate_bps, burst_bytes);
}

// -------------------------------------------------------------------------
// Leaky bucket shaping
// -------------------------------------------------------------------------

void QoSManager::setLeakyBucket(uint64_t connection_id,
                                  uint64_t drain_rate_bps,
                                  uint64_t capacity_bytes) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }
    std::lock_guard<std::mutex> lock(state->leaky_bucket_mutex);
    if (state->leaky_bucket) {
        state->leaky_bucket->reconfigure(drain_rate_bps, capacity_bytes);
    } else {
        state->leaky_bucket = std::make_shared<LeakyBucket>(drain_rate_bps, capacity_bytes);
    }
}

void QoSManager::clearLeakyBucket(uint64_t connection_id) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }
    std::lock_guard<std::mutex> lock(state->leaky_bucket_mutex);
    state->leaky_bucket.reset();
}

// -------------------------------------------------------------------------
// Priority queue scheduling
// -------------------------------------------------------------------------

bool QoSManager::enqueueSend(uint64_t connection_id, uint64_t bytes) {
    auto state = findConnection(connection_id);
    if (!state) {
        return false;
    }

    Priority prio = static_cast<Priority>(
        state->priority.load(std::memory_order_relaxed));

    PendingSend ps;
    ps.connection_id = connection_id;
    ps.bytes         = bytes;
    ps.priority      = prio;

    std::lock_guard<std::mutex> lock(pq_mutex_);
    switch (prio) {
        case Priority::CRITICAL: pq_critical_.push_back(ps); break;
        case Priority::HIGH:     pq_high_.push_back(ps);     break;
        case Priority::MEDIUM:   pq_medium_.push_back(ps);   break;
        case Priority::LOW:      pq_low_.push_back(ps);      break;
    }
    return true;
}

std::optional<QoSManager::PendingSend> QoSManager::dequeueForSend() {
    std::lock_guard<std::mutex> lock(pq_mutex_);

    bool pq_enabled = config_.enable_priority_queuing ||
                      config_.enable_priority_scheduling;

    if (!pq_enabled) {
        // No priority scheduling: simple round-robin across all queues
        for (auto* q : {&pq_critical_, &pq_high_, &pq_medium_, &pq_low_}) {
            if (!q->empty()) {
                auto item = q->front();
                q->pop_front();
                return item;
            }
        }
        return std::nullopt;
    }

    // Starvation guard: if we've served too many high-priority sends
    // consecutively, force a lower-priority send to prevent starvation.
    bool force_low = config_.enable_fair_queuing &&
                     pq_consecutive_high_serves_ >= config_.starvation_guard_threshold;

    if (force_low) {
        // Serve the lowest non-empty queue
        for (auto* q : {&pq_low_, &pq_medium_, &pq_high_, &pq_critical_}) {
            if (!q->empty()) {
                auto item = q->front();
                q->pop_front();
                pq_consecutive_high_serves_ = 0;
                return item;
            }
        }
    }

    // Strict priority: CRITICAL > HIGH > MEDIUM > LOW
    std::array<std::deque<PendingSend>*, 4> queues = {
        &pq_critical_, &pq_high_, &pq_medium_, &pq_low_
    };
    for (std::deque<PendingSend>* q : queues) {
        if (!q->empty()) {
            auto item = q->front();
            q->pop_front();
            // Only count CRITICAL/HIGH serves for starvation guard
            if (item.priority == Priority::CRITICAL ||
                item.priority == Priority::HIGH) {
                ++pq_consecutive_high_serves_;
            } else {
                pq_consecutive_high_serves_ = 0;
            }
            return item;
        }
    }
    return std::nullopt;
}

size_t QoSManager::getPendingQueueDepth(Priority priority) const {
    std::lock_guard<std::mutex> lock(pq_mutex_);
    switch (priority) {
        case Priority::CRITICAL: return pq_critical_.size();
        case Priority::HIGH:     return pq_high_.size();
        case Priority::MEDIUM:   return pq_medium_.size();
        case Priority::LOW:      return pq_low_.size();
    }
    return 0;
}

// -------------------------------------------------------------------------
// Congestion control
// -------------------------------------------------------------------------

void QoSManager::recordAck(uint64_t connection_id,
                             uint64_t bytes_acked,
                             std::chrono::microseconds rtt) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }
    std::shared_ptr<CongestionController> cc;
    {
        std::lock_guard<std::mutex> lock(state->congestion_mutex);
        if (!state->congestion_ctrl) {
            state->congestion_ctrl = std::make_shared<CongestionController>();
        }
        cc = state->congestion_ctrl;
    }
    cc->recordAck(bytes_acked, rtt);
}

void QoSManager::recordLoss(uint64_t connection_id) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }
    std::shared_ptr<CongestionController> cc;
    {
        std::lock_guard<std::mutex> lock(state->congestion_mutex);
        if (!state->congestion_ctrl) {
            state->congestion_ctrl = std::make_shared<CongestionController>();
        }
        cc = state->congestion_ctrl;
    }
    cc->recordLoss();
}

uint64_t QoSManager::getCongestionWindow(uint64_t connection_id) const {
    auto state = findConnection(connection_id);
    if (!state) {
        return UINT64_MAX;
    }
    std::lock_guard<std::mutex> lock(state->congestion_mutex);
    if (!state->congestion_ctrl) {
        return UINT64_MAX;
    }
    return state->congestion_ctrl->cwnd();
}

// -------------------------------------------------------------------------
// Linux tc integration
// -------------------------------------------------------------------------

bool QoSManager::configureTc(const TcConfig& tc_config) {
    if (!tc_config.enabled || tc_config.interface_name.empty()) {
        return false;
    }

    // Command injection guard — reject any interface name that does not
    // conform to the POSIX interface name character set.  This is checked
    // before we ever reach the posix_spawn() call below.
    if (!isValidInterfaceName(tc_config.interface_name)) {
        THEMIS_ERROR("QosManager: invalid interface name '{}' — command injection guard rejected",
                     tc_config.interface_name);
        return false;
    }

#if defined(__linux__)
    // Locate tc binary
    const char* tc_paths[] = {"/sbin/tc", "/usr/sbin/tc", "/usr/bin/tc"};
    const char* tc_bin     = nullptr;
    for (const char* p : tc_paths) {
        if (::access(p, X_OK) == 0) {
            tc_bin = p;
            break;
        }
    }
    if (tc_bin == nullptr) {
        return false;  // tc not available
    }

    // iface is validated — safe to use as a direct argv element.
    const char* iface = tc_config.interface_name.c_str();
    uint64_t rate_bps = tc_config.total_rate_bps > 0
                            ? tc_config.total_rate_bps
                            : effectiveMaxBandwidthBps();

    // Build rate/ceil strings for HTB (needed only when rate_bps > 0).
    char rate_str[32] = {};
    char ceil_str[32] = {};
    if (rate_bps > 0) {
        uint64_t rate_kbps = std::max(rate_bps / 1000, uint64_t{1});
        std::snprintf(rate_str, sizeof(rate_str), "%" PRIu64 "kbit", rate_kbps);
        std::snprintf(ceil_str, sizeof(ceil_str), "%" PRIu64 "kbit", rate_kbps);
    }

    // --- Remove any existing root qdisc (ignore failure) ---
    // argv is built inline; each token is a separate element — no shell involved.
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        char* argv[] = {
            const_cast<char*>(tc_bin),
            const_cast<char*>("qdisc"),
            const_cast<char*>("del"),
            const_cast<char*>("dev"),
            const_cast<char*>(iface),
            const_cast<char*>("root"),
            nullptr
        };
        runTcCommand(tc_bin, argv);  // intentionally ignore failure
    }

    // --- Add HTB root qdisc ---
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        char* argv[] = {
            const_cast<char*>(tc_bin),
            const_cast<char*>("qdisc"),
            const_cast<char*>("add"),
            const_cast<char*>("dev"),
            const_cast<char*>(iface),
            const_cast<char*>("root"),
            const_cast<char*>("handle"),
            const_cast<char*>("1:"),
            const_cast<char*>("htb"),
            const_cast<char*>("default"),
            const_cast<char*>("10"),
            nullptr
        };
        if (!runTcCommand(tc_bin, argv)) {
            return false;
        }
    }

    // --- Add root class with total rate limit (if configured) ---
    if (rate_bps > 0) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        char* argv[] = {
            const_cast<char*>(tc_bin),
            const_cast<char*>("class"),
            const_cast<char*>("add"),
            const_cast<char*>("dev"),
            const_cast<char*>(iface),
            const_cast<char*>("parent"),
            const_cast<char*>("1:"),
            const_cast<char*>("classid"),
            const_cast<char*>("1:1"),
            const_cast<char*>("htb"),
            const_cast<char*>("rate"),
            rate_str,
            const_cast<char*>("ceil"),
            ceil_str,
            nullptr
        };
        if (!runTcCommand(tc_bin, argv)) {
            return false;
        }
    }

    return true;
#else
    return false;  // Not supported on non-Linux platforms
#endif
}

// -------------------------------------------------------------------------
// Hot-path
// -------------------------------------------------------------------------

bool QoSManager::allowSend(uint64_t connection_id,
                            uint64_t bytes,
                            std::chrono::milliseconds timeout) {
    auto state = findConnection(connection_id);
    if (!state) {
        // Unknown connection – allow by default
        return true;
    }

    // --- Backpressure check ---
    if (config_.max_queue_bytes > 0 &&
        state->queue_depth.load(std::memory_order_relaxed) + bytes >
            config_.max_queue_bytes) {
        state->backpressure_events.fetch_add(1, std::memory_order_relaxed);
        total_backpressure_events_.fetch_add(1, std::memory_order_relaxed);

        // Fire callback if set
        {
            std::lock_guard<std::mutex> cb_lock(callback_mutex_);
            if (backpressure_cb_) {
                backpressure_cb_(connection_id, bytes);
            }
        }
        return false;
    }

    // --- Per-tenant quota check ---
    // Evaluate the shared tenant bucket BEFORE consuming per-connection tokens.
    // If the tenant quota rejects the send, no per-connection tokens should be
    // charged (tenant is the outer "budget owner").
    std::string tenant_id;
    {
        std::lock_guard<std::mutex> lock(tenant_assignments_mutex_);
        auto ta_it = tenant_assignments_.find(connection_id);
        if (ta_it != tenant_assignments_.end()) {
            tenant_id = ta_it->second;
        }
    }
    if (!tenant_id.empty()) {
        auto ts = findTenant(tenant_id);
        if (ts) {
            std::shared_ptr<TokenBucket> tenant_bucket;
            {
                std::lock_guard<std::mutex> tb_lock(ts->token_bucket_mutex);
                tenant_bucket = ts->token_bucket;
            }
            if (tenant_bucket) {
                bool ok = (timeout.count() > 0)
                              ? tenant_bucket->consume(bytes, timeout)
                              : tenant_bucket->tryConsume(bytes);
                if (!ok) {
                    ts->bytes_shaped.fetch_add(bytes, std::memory_order_relaxed);
                    total_bytes_shaped_.fetch_add(bytes, std::memory_order_relaxed);
                    return false;
                }
            }
        }
    }

    // --- Token bucket check (per-connection) ---
    // Snapshot the bucket pointer under the lock, then release before blocking.
    // TokenBucket is internally thread-safe, so calling consume/tryConsume on
    // the snapshot without holding token_bucket_mutex is safe.
    std::shared_ptr<TokenBucket> bucket;
    {
        std::lock_guard<std::mutex> tb_lock(state->token_bucket_mutex);
        bucket = state->token_bucket;
    }
    if (bucket) {
        bool ok = (timeout.count() > 0)
                      ? bucket->consume(bytes, timeout)
                      : bucket->tryConsume(bytes);

        if (!ok) {
            state->bytes_shaped.fetch_add(bytes, std::memory_order_relaxed);
            total_bytes_shaped_.fetch_add(bytes, std::memory_order_relaxed);
            return false;
        }
    }

    // --- Leaky bucket conformance check (per-connection) ---
    std::shared_ptr<LeakyBucket> leaky;
    {
        std::lock_guard<std::mutex> lb_lock(state->leaky_bucket_mutex);
        leaky = state->leaky_bucket;
    }
    if (leaky) {
        bool ok = leaky->add(bytes);
        if (!ok) {
            state->bytes_shaped.fetch_add(bytes, std::memory_order_relaxed);
            total_bytes_shaped_.fetch_add(bytes, std::memory_order_relaxed);
            return false;
        }
    }

    // --- Congestion window check ---
    // If a CongestionController is active, gate sends so that in-flight bytes
    // never exceed the current congestion window.  This provides the
    // "congestion control integration" called for by the issue AC.
    {
        std::shared_ptr<CongestionController> cc;
        {
            std::lock_guard<std::mutex> cc_lock(state->congestion_mutex);
            cc = state->congestion_ctrl;
        }
        if (cc) {
            uint64_t cwnd      = cc->cwnd();
            uint64_t in_flight = state->queue_depth.load(std::memory_order_relaxed);
            if (in_flight + bytes > cwnd) {
                state->bytes_shaped.fetch_add(bytes, std::memory_order_relaxed);
                total_bytes_shaped_.fetch_add(bytes, std::memory_order_relaxed);
                return false;
            }
        }
    }

    // Reserve queue space
    state->queue_depth.fetch_add(bytes, std::memory_order_relaxed);
    return true;
}

void QoSManager::recordBytesSent(uint64_t connection_id, uint64_t bytes) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }

    state->bytes_sent.fetch_add(bytes, std::memory_order_relaxed);

    // Release queue reservation
    uint64_t current = state->queue_depth.load(std::memory_order_relaxed);
    uint64_t release  = std::min(current, bytes);
    state->queue_depth.fetch_sub(release, std::memory_order_relaxed);

    total_bytes_sent_.fetch_add(bytes, std::memory_order_relaxed);

    // Update per-priority counter
    {
        std::lock_guard<std::mutex> lock(priority_stats_mutex_);
        bytes_per_priority_[static_cast<Priority>(state->priority.load(std::memory_order_relaxed))] += bytes;
    }

    // Update per-tenant bytes_sent counter
    std::string tenant_id;
    {
        std::lock_guard<std::mutex> lock(tenant_assignments_mutex_);
        auto it = tenant_assignments_.find(connection_id);
        if (it != tenant_assignments_.end()) {
            tenant_id = it->second;
        }
    }
    if (!tenant_id.empty()) {
        auto ts = findTenant(tenant_id);
        if (ts) {
            ts->bytes_sent.fetch_add(bytes, std::memory_order_relaxed);
        }
    }
}

void QoSManager::recordBytesReceived(uint64_t connection_id, uint64_t bytes) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }

    state->bytes_received.fetch_add(bytes, std::memory_order_relaxed);
    total_bytes_received_.fetch_add(bytes, std::memory_order_relaxed);
}

// -------------------------------------------------------------------------
// Statistics
// -------------------------------------------------------------------------

QoSManager::Stats QoSManager::getStats() const {
    Stats s;
    s.total_bytes_sent     = total_bytes_sent_.load(std::memory_order_relaxed);
    s.total_bytes_received = total_bytes_received_.load(std::memory_order_relaxed);
    s.total_bytes_shaped   = total_bytes_shaped_.load(std::memory_order_relaxed);
    s.backpressure_events  = total_backpressure_events_.load(std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        s.active_connections = connections_.size();
    }

    {
        std::lock_guard<std::mutex> lock(priority_stats_mutex_);
        s.bytes_per_priority = bytes_per_priority_;
    }

    return s;
}

QoSManager::ConnectionStats
QoSManager::getConnectionStats(uint64_t connection_id) const {
    auto state = findConnection(connection_id);
    if (!state) {
        return ConnectionStats{};
    }

    ConnectionStats cs;
    cs.connection_id        = state->connection_id;
    cs.priority             = static_cast<Priority>(state->priority.load(std::memory_order_relaxed));
    cs.bytes_sent           = state->bytes_sent.load(std::memory_order_relaxed);
    cs.bytes_received       = state->bytes_received.load(std::memory_order_relaxed);
    cs.bytes_shaped         = state->bytes_shaped.load(std::memory_order_relaxed);
    cs.queue_depth          = state->queue_depth.load(std::memory_order_relaxed);
    cs.backpressure_events  = state->backpressure_events.load(std::memory_order_relaxed);
    cs.has_token_bucket     = false;
    cs.token_bucket_rate_bps    = 0;
    cs.token_bucket_burst_bytes = 0;
    {
        std::lock_guard<std::mutex> tb_lock(state->token_bucket_mutex);
        cs.has_token_bucket = (state->token_bucket != nullptr);
        if (cs.has_token_bucket) {
            cs.token_bucket_rate_bps    = state->token_bucket->rateBps();
            cs.token_bucket_burst_bytes = state->token_bucket->burstBytes();
        }
    }
    {
        std::lock_guard<std::mutex> cc_lock(state->congestion_mutex);
        if (state->congestion_ctrl) {
            cs.congestion_window        = state->congestion_ctrl->cwnd();
            cs.congestion_ssthresh_bytes = state->congestion_ctrl->ssthresh();
            cs.smoothed_rtt_us          = static_cast<uint64_t>(
                state->congestion_ctrl->smoothedRtt().count());
        }
    }
    return cs;
}

std::vector<QoSManager::ConnectionStats>
QoSManager::getAllConnectionStats() const {
    std::vector<uint64_t> ids;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        ids.reserve(connections_.size());
        for (const auto& [id, _] : connections_) {
            ids.push_back(id);
        }
    }

    std::vector<ConnectionStats> result;
    result.reserve(ids.size());
    for (uint64_t id : ids) {
        result.push_back(getConnectionStats(id));
    }
    return result;
}

// -------------------------------------------------------------------------
// Callbacks
// -------------------------------------------------------------------------

void QoSManager::setBackpressureCallback(
    std::function<void(uint64_t, uint64_t)> cb) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    backpressure_cb_ = std::move(cb);
}

// =============================================================================
// Per-tenant bandwidth quota management
// =============================================================================

std::shared_ptr<QoSManager::TenantState>
QoSManager::findTenant(const std::string& id) const {
    std::lock_guard<std::mutex> lock(tenants_mutex_);
    auto it = tenants_.find(id);
    if (it == tenants_.end()) {
        return nullptr;
    }
    return it->second;
}

void QoSManager::registerTenantQuota(const std::string& tenant_id,
                                      uint64_t rate_bps,
                                      uint64_t burst_bytes) {
    uint64_t burst = burst_bytes > 0 ? burst_bytes
                                      : (rate_bps > 0 ? rate_bps / 8 : 0);

    std::lock_guard<std::mutex> lock(tenants_mutex_);
    auto it = tenants_.find(tenant_id);
    if (it != tenants_.end()) {
        // Update existing entry in-place
        auto& ts = it->second;
        std::lock_guard<std::mutex> tb_lock(ts->token_bucket_mutex);
        if (ts->token_bucket) {
            ts->token_bucket->reconfigure(rate_bps, burst > 0 ? burst : 1);
        } else if (rate_bps > 0) {
            ts->token_bucket = std::make_shared<TokenBucket>(rate_bps, burst > 0 ? burst : 1);
        }
    } else {
        auto ts        = std::make_shared<TenantState>();
        ts->tenant_id  = tenant_id;
        if (rate_bps > 0) {
            ts->token_bucket = std::make_shared<TokenBucket>(rate_bps, burst > 0 ? burst : 1);
        }
        tenants_[tenant_id] = std::move(ts);
    }
}

void QoSManager::unregisterTenantQuota(const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(tenants_mutex_);
    tenants_.erase(tenant_id);
}

void QoSManager::setTenantQuota(const std::string& tenant_id,
                                  uint64_t rate_bps,
                                  uint64_t burst_bytes) {
    registerTenantQuota(tenant_id, rate_bps, burst_bytes);
}

void QoSManager::assignTenant(uint64_t connection_id,
                                const std::string& tenant_id) {
    std::string old_tenant_id;
    {
        std::lock_guard<std::mutex> lock(tenant_assignments_mutex_);
        auto it = tenant_assignments_.find(connection_id);
        if (it != tenant_assignments_.end()) {
            if (it->second == tenant_id) {
                return;  // Already assigned to this tenant
            }
            old_tenant_id = it->second;
        }
        tenant_assignments_[connection_id] = tenant_id;
    }

    // Adjust active_connections counters outside tenant_assignments_mutex_
    // to avoid potential lock-order inversion with tenants_mutex_.
    if (!old_tenant_id.empty()) {
        auto old_ts = findTenant(old_tenant_id);
        if (old_ts) {
            old_ts->active_connections.fetch_sub(1, std::memory_order_relaxed);
        }
    }
    auto new_ts = findTenant(tenant_id);
    if (new_ts) {
        new_ts->active_connections.fetch_add(1, std::memory_order_relaxed);
    }
}

QoSManager::TenantQuotaStats
QoSManager::getTenantStats(const std::string& tenant_id) const {
    auto ts = findTenant(tenant_id);
    if (!ts) {
        return TenantQuotaStats{};
    }

    TenantQuotaStats result;
    result.tenant_id         = ts->tenant_id;
    result.bytes_sent        = ts->bytes_sent.load(std::memory_order_relaxed);
    result.bytes_shaped      = ts->bytes_shaped.load(std::memory_order_relaxed);
    result.active_connections = ts->active_connections.load(std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> tb_lock(ts->token_bucket_mutex);
        if (ts->token_bucket) {
            result.rate_bps    = ts->token_bucket->rateBps();
            result.burst_bytes = ts->token_bucket->burstBytes();
        }
    }
    return result;
}

std::vector<QoSManager::TenantQuotaStats>
QoSManager::getAllTenantStats() const {
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(tenants_mutex_);
        ids.reserve(tenants_.size());
        for (const auto& [id, _] : tenants_) {
            ids.push_back(id);
        }
    }

    std::vector<TenantQuotaStats> result;
    result.reserve(ids.size());
    for (const auto& id : ids) {
        result.push_back(getTenantStats(id));
    }
    return result;
}

}  // namespace network
}  // namespace themis
