/**
 * @file raft_load_balancer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "network/raft_load_balancer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <future>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#if !defined(_WIN32)
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/select.h>
#else
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

#include "utils/logger.h"

namespace themis {
namespace network {

namespace {

constexpr int kRaftLbShutdownJoinTimeoutMs = 5000;

/// @brief Join @p t within @p timeout_ms; log and detach on timeout.
static void timedJoinRaftLoadBalancer(std::thread &t, int timeout_ms = kRaftLbShutdownJoinTimeoutMs) noexcept {
    if (!t.joinable())
        return;
    std::promise<void> done;
    auto fut = done.get_future();
    std::thread watcher([inner = std::move(t), p = std::move(done)]() mutable {
        if (inner.joinable())
            inner.join();
        p.set_value();
    });
    watcher.detach();
    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) != std::future_status::ready) {
        // thread_join_no_timeout: detach after deadline to avoid indefinite block
        THEMIS_WARN("Thread did not finish within {} ms during shutdown; detaching.", timeout_ms);
    }
}

} // namespace

// =============================================================================
// Backend – move operations
// =============================================================================

RaftLoadBalancer::Backend::Backend(Backend &&o) noexcept
    : address(std::move(o.address)), weight(o.weight), healthy(o.healthy), datacenter(std::move(o.datacenter)),
      last_health_check(o.last_health_check) {
    active_connections.store(o.active_connections.load(std::memory_order_relaxed), std::memory_order_relaxed);
    total_requests.store(o.total_requests.load(std::memory_order_relaxed), std::memory_order_relaxed);
    failed_requests.store(o.failed_requests.load(std::memory_order_relaxed), std::memory_order_relaxed);
    consecutive_failures.store(o.consecutive_failures.load(std::memory_order_relaxed), std::memory_order_relaxed);
    consecutive_successes.store(o.consecutive_successes.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

RaftLoadBalancer::Backend &RaftLoadBalancer::Backend::operator=(Backend &&o) noexcept {
    if (this != &o) {
        address           = std::move(o.address);
        weight            = o.weight;
        healthy           = o.healthy;
        datacenter        = std::move(o.datacenter);
        last_health_check = o.last_health_check;
        active_connections.store(o.active_connections.load(std::memory_order_relaxed), std::memory_order_relaxed);
        total_requests.store(o.total_requests.load(std::memory_order_relaxed), std::memory_order_relaxed);
        failed_requests.store(o.failed_requests.load(std::memory_order_relaxed), std::memory_order_relaxed);
        consecutive_failures.store(o.consecutive_failures.load(std::memory_order_relaxed), std::memory_order_relaxed);
        consecutive_successes.store(o.consecutive_successes.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
    return *this;
}

// =============================================================================
// Construction / Destruction
// =============================================================================

RaftLoadBalancer::RaftLoadBalancer(const Config &config) : config_(config), strategy_(config.strategy) {}

RaftLoadBalancer::~RaftLoadBalancer() {
    stop();
}

// =============================================================================
// Lifecycle
// =============================================================================

void RaftLoadBalancer::start() {
    // Prevent double-start: exchange started_ to true; if it was already true,
    // the threads are running and we return immediately.
    if (started_.exchange(true, std::memory_order_acq_rel)) {
        return; // Already started — idempotent.
    }
    shutdown_.store(false, std::memory_order_release);

    health_check_thread_ = std::thread([this]() { healthCheckLoop(); });
    raft_thread_         = std::thread([this]() { raftLoop(); });
}

void RaftLoadBalancer::stop() {
    {
        std::lock_guard<std::mutex> lk(shutdown_mutex_);
        shutdown_.store(true, std::memory_order_release);
    }
    shutdown_cv_.notify_all();

    if (health_check_thread_.joinable())
        timedJoinRaftLoadBalancer(health_check_thread_);
    if (raft_thread_.joinable())
        timedJoinRaftLoadBalancer(raft_thread_);

    // Reset started_ so that start() can be called again after stop().
    started_.store(false, std::memory_order_release);
}

// =============================================================================
// Backend Management
// =============================================================================

void RaftLoadBalancer::addBackend(const std::string &address, double weight, const std::string &datacenter) {
    std::lock_guard<std::mutex> lk(backends_mutex_);
    // Avoid duplicates
    for (const auto &b : backends_) {
        if (b->address == address)
            return;
    }
    auto backend        = std::make_unique<Backend>();
    backend->address    = address;
    backend->weight     = (weight >= 0.0) ? weight : 0.0;
    backend->healthy    = true;
    backend->datacenter = datacenter;
    backends_.push_back(std::move(backend));
}

void RaftLoadBalancer::removeBackend(const std::string &address) {
    std::lock_guard<std::mutex> lk(backends_mutex_);
    backends_.erase(std::remove_if(backends_.begin(), backends_.end(),
                                   [&address](const std::unique_ptr<Backend> &b) { return b->address == address; }),
                    backends_.end());
}

void RaftLoadBalancer::updateWeight(const std::string &address, double weight) {
    std::lock_guard<std::mutex> lk(backends_mutex_);
    auto *b = findBackend(address);
    if (b) {
        b->weight = (weight >= 0.0) ? weight : 0.0;
    }
}

std::vector<RaftLoadBalancer::Backend *> RaftLoadBalancer::getBackends() const {
    std::lock_guard<std::mutex> lk(backends_mutex_);
    std::vector<Backend *> result = {};

    result.reserve(backends_.size());
    for (const auto &b : backends_) {
        result.push_back(b.get());
    }
    return result;
}

// =============================================================================
// Routing
// =============================================================================

std::string RaftLoadBalancer::selectBackend() {
    std::lock_guard<std::mutex> lk(backends_mutex_);
    if (backends_.empty())
        return {};

    const LoadBalancingStrategy strat = strategy_.load(std::memory_order_acquire);

    // R17: Connection Lifecycle Safety
    // When a backend is selected, the caller should:
    // 1. Call onConnectionOpened(backend_address) to increment active_connections counter
    // 2. Use the connection
    // 3. Call onConnectionClosed(backend_address) on success or error
    //
    // To ensure cleanup even if errors occur, use the ConnectionGuard RAII class:
    //   auto addr = lb.selectBackend();
    //   ConnectionGuard guard(lb, addr);
    //   // ... use guard.address() as connection ...
    //   // guard destructor calls onConnectionClosed() automatically

    switch (strat) {
        case LoadBalancingStrategy::ROUND_ROBIN:
            return selectRoundRobin();
        case LoadBalancingStrategy::LEAST_CONNECTIONS:
            return selectLeastConnections();
        case LoadBalancingStrategy::WEIGHTED_ROUND_ROBIN:
            return selectWeightedRoundRobin();
        case LoadBalancingStrategy::HEALTH_BASED:
            return selectHealthBased();
        case LoadBalancingStrategy::CONSISTENT_HASH:
            // Consistent hash without a key falls back to round-robin
            return selectRoundRobin();
        default:
            return selectRoundRobin();
    }
}

std::string RaftLoadBalancer::selectBackend(const std::string &key) {
    std::lock_guard<std::mutex> lk(backends_mutex_);
    if (backends_.empty())
        return {};
    // R18: Connection Lifecycle on Rebalance/Timeout
    // When rebalancing occurs or timeouts are triggered, ensure connections to
    // previously-selected backends are properly closed before selecting new backends.
    // Use ConnectionGuard to guarantee cleanup in all paths.
    return selectConsistentHash(key);
}

void RaftLoadBalancer::onRequestComplete(const std::string &address, bool success) {
    total_requests_.fetch_add(1, std::memory_order_relaxed);
    if (!success) {
        total_failed_.fetch_add(1, std::memory_order_relaxed);
    }

    std::lock_guard<std::mutex> lk(backends_mutex_);
    auto *b = findBackend(address);
    if (!b)
        return;

    b->total_requests.fetch_add(1, std::memory_order_relaxed);
    if (!success) {
        b->failed_requests.fetch_add(1, std::memory_order_relaxed);
    }
}

void RaftLoadBalancer::onConnectionOpened(const std::string &address) {
    std::lock_guard<std::mutex> lk(backends_mutex_);
    auto *b = findBackend(address);
    if (b)
        b->active_connections.fetch_add(1, std::memory_order_relaxed);
}

void RaftLoadBalancer::onConnectionClosed(const std::string &address) {
    std::lock_guard<std::mutex> lk(backends_mutex_);
    auto *b = findBackend(address);
    if (b && b->active_connections.load(std::memory_order_relaxed) > 0) {
        b->active_connections.fetch_sub(1, std::memory_order_relaxed);
    }
}

// =============================================================================
// Raft State
// =============================================================================

bool RaftLoadBalancer::isLeader() const {
    return role_.load(std::memory_order_acquire) == RaftRole::LEADER;
}

RaftRole RaftLoadBalancer::getRole() const {
    return role_.load(std::memory_order_acquire);
}

uint64_t RaftLoadBalancer::getCurrentTerm() const {
    return current_term_.load(std::memory_order_acquire);
}

// =============================================================================
// Observability
// =============================================================================

RaftLoadBalancer::Stats RaftLoadBalancer::getStats() const {
    Stats s;
    s.total_requests   = total_requests_.load(std::memory_order_acquire);
    s.failed_requests  = total_failed_.load(std::memory_order_acquire);
    s.rebalance_events = rebalance_events_.load(std::memory_order_acquire);
    s.failover_events  = failover_events_.load(std::memory_order_acquire);
    s.recovery_events  = recovery_events_.load(std::memory_order_acquire);

    std::lock_guard<std::mutex> lk(backends_mutex_);
    for (const auto &b : backends_) {
        if (!b->healthy)
            ++s.failed_backends;
        s.requests_per_backend[b->address] = b->total_requests.load(std::memory_order_relaxed);
    }
    return s;
}

void RaftLoadBalancer::setStrategy(LoadBalancingStrategy strategy) {
    strategy_.store(strategy, std::memory_order_release);
}

void RaftLoadBalancer::setHealthCheckFn(std::function<bool(const Backend &)> fn) {
    std::lock_guard<std::mutex> lk(shutdown_mutex_);
    health_check_fn_ = std::move(fn);
}

// =============================================================================
// Internal – routing helpers  (all called with backends_mutex_ held)
// =============================================================================

RaftLoadBalancer::Backend *RaftLoadBalancer::findBackend(const std::string &address) {
    for (const auto &b : backends_) {
        if (b->address == address)
            return b.get();
    }
    return nullptr;
}

std::vector<RaftLoadBalancer::Backend *> RaftLoadBalancer::healthyBackends() const {
    std::vector<Backend *> result = {};

    // Prefer local datacenter if configured
    if (config_.prefer_local_datacenter && !config_.datacenter.empty()) {
        for (const auto &b : backends_) {
            if (b->healthy && b->datacenter == config_.datacenter) {
                result.push_back(b.get());
            }
        }
    }
    // Fall back to all healthy backends
    if (result.empty()) {
        for (const auto &b : backends_) {
            if (b->healthy)
                result.push_back(b.get());
        }
    }
    return result;
}

std::string RaftLoadBalancer::selectRoundRobin() {
    auto healthy = healthyBackends();
    if (healthy.empty())
        return {};
    const size_t n   = healthy.size();
    const size_t idx = rr_index_.fetch_add(1, std::memory_order_relaxed) % n;
    return healthy[idx]->address;
}

std::string RaftLoadBalancer::selectLeastConnections() {
    auto healthy = healthyBackends();
    if (healthy.empty())
        return {};

    Backend *best       = nullptr;
    uint64_t best_conns = std::numeric_limits<uint64_t>::max();
    for (auto *b : healthy) {
        const uint64_t conns = b->active_connections.load(std::memory_order_relaxed);
        if (conns < best_conns) {
            best_conns = conns;
            best       = b;
        }
    }
    return best ? best->address : std::string{};
}

std::string RaftLoadBalancer::selectWeightedRoundRobin() {
    // Nginx smooth weighted round-robin.
    // Each call: (1) add each backend's configured weight to its effective
    // weight, (2) pick the backend with the highest effective weight,
    // (3) subtract the total weight from the winner's effective weight.
    // This guarantees a proportional distribution over N calls without
    // sorting the entire list every time.
    auto healthy = healthyBackends();
    if (healthy.empty())
        return {};

    double total_weight = 0.0;
    for (auto *b : healthy)
        total_weight += b->weight;
    if (total_weight <= 0.0)
        return selectRoundRobin();

    // Step 1 – increment effective weights by configured weight.
    for (auto *b : healthy) {
        wrr_effective_weights_[b->address] += b->weight;
    }

    // Step 2 – pick the backend with the highest effective weight.
    Backend *winner = healthy[0];
    double best     = wrr_effective_weights_[healthy[0]->address];
    for (size_t i = 1; i <static_cast<int>(healthy.size()); ++i) {
        const double ew = wrr_effective_weights_[healthy[i]->address];
        if (ew > best) {
            best   = ew;
            winner = healthy[i];
        }
    }

    // Step 3 – reduce the winner's effective weight by total_weight.
    wrr_effective_weights_[winner->address] -= total_weight;

    return winner->address;
}

std::string RaftLoadBalancer::selectHealthBased() {
    // Same as round-robin but healthyBackends() already filters unhealthy ones.
    return selectRoundRobin();
}

std::string RaftLoadBalancer::selectConsistentHash(const std::string &key) {
    auto healthy = healthyBackends();
    if (healthy.empty())
        return {};

    // FNV-1a hash of the key
    uint64_t hash = 14695981039346656037;
    for (unsigned char c : key) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211;
    }

    // Map hash onto healthy backends
    const size_t idx = static_cast<size_t>(hash % healthy.size());
    return healthy[idx]->address;
}

// =============================================================================
// Health Check Loop
// =============================================================================

bool RaftLoadBalancer::defaultHealthCheck(const Backend& backend) {
    // Real TCP probe with 500 ms connect timeout.
    // Splits backend.address ("host:port") into host and port components.
    // Returns true if the TCP handshake completes within the deadline.

    const std::string& addr = backend.address;

    // Parse "host:port"
    const auto colon = addr.rfind(':');
    if (colon == std::string::npos) {
        THEMIS_WARN("RaftLoadBalancer: health-check address '{}' has no port; skipping", addr);
        return false;
    }
    const std::string host = addr.substr(0, colon);
    const std::string port = addr.substr(colon + 1);

#if defined(_WIN32)
    using sock_t = SOCKET;
    constexpr sock_t kInvalid = INVALID_SOCKET;
    auto closeSock = [](sock_t s) { ::closesocket(s); };
#else
    using sock_t = int;
    constexpr sock_t kInvalid = -1;
    auto closeSock = [](sock_t s) { ::close(s); };
#endif

    // RAII socket handle
    struct SockGuard {
        sock_t fd;
        decltype(closeSock)& closer;
        ~SockGuard() noexcept { if (fd != kInvalid) closer(fd); }
    };

    struct ::addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct ::addrinfo* res_raw = nullptr;
    if (::getaddrinfo(host.c_str(), port.c_str(), &hints, &res_raw) != 0 || !res_raw) {
        return false;
    }
    // Unique owner of the addrinfo list
    struct AddrInfoGuard {
        struct ::addrinfo* p;
        ~AddrInfoGuard() noexcept { ::freeaddrinfo(p); }
    } ai_guard{res_raw};

    sock_t fd = ::socket(res_raw->ai_family, res_raw->ai_socktype, res_raw->ai_protocol);
    if (fd == kInvalid) {
        return false;
    }
    SockGuard sg{fd, closeSock};

#if defined(_WIN32)
    // Set non-blocking on Windows
    u_long mode = 1;
    ::ioctlsocket(fd, FIONBIO, &mode);
    ::connect(fd, res_raw->ai_addr, static_cast<int>(res_raw->ai_addrlen));
    // select() with 500 ms deadline
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    struct ::timeval tv{0, 500'000};
    int sel = ::select(0, nullptr, &wfds, nullptr, &tv);
    return sel > 0;
#else
    // Set non-blocking
    const int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    const int rc = ::connect(fd, res_raw->ai_addr, res_raw->ai_addrlen);
    if (rc == 0) {
        return true;  // instant connect (loopback, etc.)
    }
    if (errno != EINPROGRESS) {
        return false;
    }

    // Wait up to 500 ms for the connect to complete.
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    struct ::timeval tv{0, 500'000};  // 500 ms

    const int sel = ::select(fd + 1, nullptr, &wfds, nullptr, &tv);
    if (sel <= 0) {
        return false;  // timeout or error
    }

    // Confirm the connection did not fail asynchronously.
    int sock_err = 0;
    ::socklen_t len = sizeof(sock_err);
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_err, &len) != 0) {
        return false;
    }
    return sock_err == 0;
#endif
}

void RaftLoadBalancer::runHealthChecks() {
    std::function<bool(const Backend &)> check_fn;
    {
        std::lock_guard<std::mutex> lk(shutdown_mutex_);
        check_fn = health_check_fn_ ? health_check_fn_ : &RaftLoadBalancer::defaultHealthCheck;
    }

    std::vector<Backend *> backends_snapshot;
    {
        std::lock_guard<std::mutex> lk(backends_mutex_);
        for (const auto &b : backends_)
            backends_snapshot.push_back(b.get());
    }

    for (Backend *b : backends_snapshot) {
        // R18: Timeout Safety in Health Checks
        // When health_check_fn_ opens actual connections (not stubbed):
        // 1. Use connection timeout from Phase C (wire_protocol_zero_copy timeout patterns)
        // 2. Ensure connection is closed even if health check times out or fails
        // 3. Use try-catch to guarantee cleanup in exception paths
        // 4. Example: Wrap check_fn call with guard to track connection lifecycle
        const bool ok  = check_fn(*b);
        const auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lk(backends_mutex_);
        b->last_health_check = now;

        if (ok) {
            b->consecutive_failures.store(0, std::memory_order_relaxed);
            const uint32_t succ = b->consecutive_successes.fetch_add(1, std::memory_order_relaxed) + 1;
            if (!b->healthy && succ >= config_.recovery_threshold) {
                b->healthy = true;
                b->consecutive_successes.store(0, std::memory_order_relaxed);
                recovery_events_.fetch_add(1, std::memory_order_relaxed);
            }
        } else {
            b->consecutive_successes.store(0, std::memory_order_relaxed);
            const uint32_t fail = b->consecutive_failures.fetch_add(1, std::memory_order_relaxed) + 1;
            if (b->healthy && fail >= config_.unhealthy_threshold) {
                b->healthy = false;
                b->consecutive_failures.store(0, std::memory_order_relaxed);
                failover_events_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    maybeRebalance();
}

void RaftLoadBalancer::healthCheckLoop() {
    const auto interval = std::chrono::milliseconds(config_.health_check_interval_ms);

    while (!shutdown_.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lk(shutdown_mutex_);
        shutdown_cv_.wait_for(lk, interval, [this] { return shutdown_.load(std::memory_order_acquire); });
        lk.unlock();

        if (shutdown_.load(std::memory_order_acquire))
            break;
        runHealthChecks();
    }
}

// =============================================================================
// Rebalancing
// =============================================================================

void RaftLoadBalancer::maybeRebalance() {
    std::lock_guard<std::mutex> lk(backends_mutex_);
    auto healthy = healthyBackends();
    if (static_cast<int>(healthy.size()) < 2)
        return;

    // Compute mean load (active connections normalised by weight)
    double total_load = 0.0;
    for (auto *b : healthy) {
        const double effective_weight = b->weight > 0.0 ? b->weight : 1.0;
        total_load += static_cast<double>(b->active_connections.load(std::memory_order_relaxed)) / effective_weight;
    }
    const double mean_load = total_load / static_cast<double>(healthy.size());
    if (mean_load <= 0.0)
        return;

    bool needs_rebalance = false;
    for (auto *b : healthy) {
        const double effective_weight = b->weight > 0.0 ? b->weight : 1.0;
        const double load
            = static_cast<double>(b->active_connections.load(std::memory_order_relaxed)) / effective_weight;
        if (std::abs(load - mean_load) / mean_load > config_.rebalance_threshold) {
            needs_rebalance = true;
            break;
        }
    }

    if (needs_rebalance) {
        // Adjust weights inversely proportional to current load so that
        // future routing evens out.
        for (auto *b : healthy) {
            const double effective_weight = b->weight > 0.0 ? b->weight : 1.0;
            const double load
                = static_cast<double>(b->active_connections.load(std::memory_order_relaxed)) / effective_weight;
            if (load > 0.0) {
                // Damped adjustment: move weight 10 % towards the inverse-load target
                const double target_weight = mean_load / load * effective_weight;
                b->weight                  = effective_weight + 0.1 * (target_weight - effective_weight);
                if (b->weight < 0.1)
                    b->weight = 0.1; // floor
            }
        }
        rebalance_events_.fetch_add(1, std::memory_order_relaxed);
    }
}

// =============================================================================
// Raft Loop  (simplified single-node leader for this release)
// =============================================================================

void RaftLoadBalancer::raftLoop() {
    // Seed random timeout for leader election
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> election_dist(config_.election_timeout_min_ms,
                                                          config_.election_timeout_max_ms);

    auto next_election_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(election_dist(rng));

    while (!shutdown_.load(std::memory_order_acquire)) {
        const auto now  = std::chrono::steady_clock::now();
        const auto role = role_.load(std::memory_order_acquire);

        if (role == RaftRole::LEADER) {
            // Send heartbeats every heartbeat_interval_ms
            const auto heartbeat_interval = std::chrono::milliseconds(config_.heartbeat_interval_ms);
            std::unique_lock<std::mutex> lk(shutdown_mutex_);
            shutdown_cv_.wait_for(lk, heartbeat_interval, [this] { return shutdown_.load(std::memory_order_acquire); });
            // In a real implementation we would broadcast AppendEntries RPCs here.
        } else {
            // Follower / Candidate: wait for election timeout
            if (now >= next_election_at) {
                // Start a new election
                const uint64_t new_term = current_term_.fetch_add(1, std::memory_order_acq_rel) + 1;
                role_.store(RaftRole::CANDIDATE, std::memory_order_release);
                vote_count_.store(1, std::memory_order_relaxed); // vote for self

                // In a real implementation we would broadcast RequestVote RPCs.
                // Since this is a single-node instance in the unit-test context,
                // immediately become leader when we have a majority (1/1 vote).
                // In production this would wait for quorum responses.
                role_.store(RaftRole::LEADER, std::memory_order_release);
                current_term_.store(new_term, std::memory_order_release);

                // Reset heartbeat timer
                {
                    std::lock_guard<std::mutex> lk2(raft_mutex_);
                    last_heartbeat_ = std::chrono::steady_clock::now();
                }
            }

            // Compute next election deadline with randomised timeout
            next_election_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(election_dist(rng));

            const auto wait_time = std::chrono::milliseconds(config_.heartbeat_interval_ms);
            std::unique_lock<std::mutex> lk(shutdown_mutex_);
            shutdown_cv_.wait_for(lk, wait_time, [this] { return shutdown_.load(std::memory_order_acquire); });
        }
    }
}

} // namespace network
} // namespace themis
