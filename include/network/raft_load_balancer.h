/**
 * @file raft_load_balancer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {
namespace network {

// =============================================================================
// LoadBalancingStrategy
// =============================================================================

/**
 * @brief Routing strategy applied by the active load balancer.
 */
enum class LoadBalancingStrategy {
    ROUND_ROBIN,         ///< Simple, predictable sequential cycling.
    LEAST_CONNECTIONS,   ///< Route to the backend with fewest active connections.
    WEIGHTED_ROUND_ROBIN,///< Distribute based on capacity weights.
    HEALTH_BASED,        ///< Exclude unhealthy backends; fall back to round-robin.
    CONSISTENT_HASH,     ///< Sticky routing for session/cache affinity.
};

// =============================================================================
// RaftRole
// =============================================================================

/**
 * @brief Role of this load-balancer node in the Raft cluster.
 */
enum class RaftRole {
    FOLLOWER,   ///< Accepts reads but defers writes to the leader.
    CANDIDATE,  ///< Campaigning for leadership (transient).
    LEADER,     ///< Performs health checks and propagates routing decisions.
};

// =============================================================================
// RaftLoadBalancer
// =============================================================================

/**
 * @brief Raft-coordinated load balancer for distributed query routing.
 *
 * Thread-safe.  All public methods may be called concurrently.
 */
class RaftLoadBalancer {
public:
    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------

    /**
     * @brief Configuration for RaftLoadBalancer.
     */
    struct Config {
        /// Port used for intra-cluster Raft communication.
        uint16_t raft_port = 8774;

        /// Interval between backend health checks (milliseconds).
        uint32_t health_check_interval_ms = 5000;

        /// Fractional load imbalance that triggers a rebalance event.
        /// E.g., 0.2 means rebalance when any backend deviates >20 % from mean.
        double rebalance_threshold = 0.2;

        /// Number of consecutive health-check failures before a backend is marked
        /// unhealthy.
        uint32_t unhealthy_threshold = 3;

        /// Number of consecutive health-check successes before a previously
        /// unhealthy backend is re-admitted.
        uint32_t recovery_threshold = 2;

        /// Routing algorithm to use.
        LoadBalancingStrategy strategy = LoadBalancingStrategy::LEAST_CONNECTIONS;

        /// Datacenter / region label for this node.  Used for cross-DC routing.
        std::string datacenter;

        /// Prefer local-datacenter backends when available.
        bool prefer_local_datacenter = true;

        /// Raft election timeout range (milliseconds).
        uint32_t election_timeout_min_ms = 150;
        uint32_t election_timeout_max_ms = 300;

        /// Raft heartbeat interval (milliseconds).
        uint32_t heartbeat_interval_ms = 50;
    };

    // -------------------------------------------------------------------------
    // Backend
    // -------------------------------------------------------------------------

    /**
     * @brief Represents a single backend server managed by the load balancer.
     */
    struct Backend {
        std::string address;            ///< "host:port" string.
        double      weight = 1.0;       ///< Relative capacity weight (>=0).
        bool        healthy = true;     ///< False when the backend fails health checks.
        std::string datacenter;         ///< Datacenter/region label (empty = default).

        // Runtime counters (updated by the load balancer)
        std::atomic<uint64_t> active_connections{0};
        std::atomic<uint64_t> total_requests{0};
        std::atomic<uint64_t> failed_requests{0};
        std::atomic<uint32_t> consecutive_failures{0};
        std::atomic<uint32_t> consecutive_successes{0};
        std::chrono::steady_clock::time_point last_health_check{};

        // Non-copyable due to atomics; allow move.
        Backend() = default;
        Backend(const Backend&) = delete;
        Backend& operator=(const Backend&) = delete;
        Backend(Backend&& o) noexcept;
        Backend& operator=(Backend&& o) noexcept;
    };

    // -------------------------------------------------------------------------
    // Stats
    // -------------------------------------------------------------------------

    /**
     * @brief Aggregate statistics for this load-balancer node.
     */
    struct Stats {
        uint64_t total_requests    = 0;  ///< Requests routed since start.
        uint64_t failed_requests   = 0;  ///< Requests that resulted in failure.
        uint64_t failed_backends   = 0;  ///< Backends currently unhealthy.
        uint64_t rebalance_events  = 0;  ///< Times dynamic weight adjustment ran.
        uint64_t failover_events   = 0;  ///< Times a backend was marked unhealthy.
        uint64_t recovery_events   = 0;  ///< Times a backend recovered.
        std::map<std::string, uint64_t> requests_per_backend; ///< Per-backend totals.
    };

    // -------------------------------------------------------------------------
    // R17, R18: Connection Lifecycle Management (RAII Guard)
    // -------------------------------------------------------------------------

    /**
     * @brief RAII guard for connection lifecycle management.
     *
     * Ensures onConnectionOpened() and onConnectionClosed() are called in pairs,
     * even if errors occur or exceptions are thrown. Prevents connection leaks
     * from imbalanced callback invocations.
     *
     * Example:
     * @code
     *   auto conn = lb->selectBackend();  // Get backend address
     *   lb->onConnectionOpened(conn);      // Manual open (or use guard)
     *   // ... use connection ...
     *   lb->onConnectionClosed(conn);      // Manual close
     *
     *   // OR using guard (exception-safe):
     *   {
     *       ConnectionGuard guard(*lb, backend_address);
     *       // ... use connection (guard.address() == backend_address)
     *       // guard destructor calls onConnectionClosed even if exception
     *   }
     * @endcode
     */
    class ConnectionGuard {
    public:
        /**
         * @brief Create a connection guard for the given backend.
         * Automatically calls onConnectionOpened() on construction.
         * @param lb Reference to RaftLoadBalancer
         * @param backend_address Backend address returned by selectBackend()
         */
        ConnectionGuard(RaftLoadBalancer& lb, const std::string& backend_address) noexcept
            : lb_(lb), backend_address_(backend_address)
        {
            lb_.get().onConnectionOpened(backend_address_);
        }

        /**
         * @brief Destructor automatically calls onConnectionClosed().
         * Exception-safe: marked noexcept and suppresses any exceptions.
         */
        ~ConnectionGuard() noexcept
        {
            try {
                lb_.get().onConnectionClosed(backend_address_);
            }
            catch (...) {
                // Suppress exceptions during cleanup to maintain noexcept contract
            }
        }

        // Non-copyable to prevent double-close
        ConnectionGuard(const ConnectionGuard&) = delete;
        ConnectionGuard& operator=(const ConnectionGuard&) = delete;

        // Movable to support scope transfer
        ConnectionGuard(ConnectionGuard&& other) noexcept
            : lb_(other.lb_), backend_address_(std::move(other.backend_address_))
        {
            other.backend_address_.clear();
        }

        ConnectionGuard& operator=(ConnectionGuard&& other) noexcept
        {
            if (this != &other) {
                // Close current connection before moving
                try {
                    lb_.get().onConnectionClosed(backend_address_);
                }
                catch (...) {}

                lb_ = other.lb_;
                backend_address_ = std::move(other.backend_address_);
                other.backend_address_.clear();
            }
            return *this;
        }

        /**
         * @brief Get the backend address this guard is managing.
         */
        const std::string& address() const noexcept { return backend_address_; }

    private:
        std::reference_wrapper<RaftLoadBalancer> lb_;
        std::string backend_address_;
    };

    // -------------------------------------------------------------------------
    // Construction / Destruction
    // -------------------------------------------------------------------------

    explicit RaftLoadBalancer(const Config& config);
    ~RaftLoadBalancer();

    // Non-copyable, non-movable (background threads capture this)
    RaftLoadBalancer(const RaftLoadBalancer&) = delete;
    RaftLoadBalancer& operator=(const RaftLoadBalancer&) = delete;
    RaftLoadBalancer(RaftLoadBalancer&&) = delete;
    RaftLoadBalancer& operator=(RaftLoadBalancer&&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Start health-check and Raft coordination threads.
     * @throws std::runtime_error if already started.
     */
    void start();

    /**
     * @brief Stop all background threads and release resources.
     */
    void stop();

    // -------------------------------------------------------------------------
    // Backend Management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a new backend.
     * @param address  "host:port" string.
     * @param weight   Capacity weight (default 1.0).
     * @param datacenter  Optional datacenter/region label.
     */
    void addBackend(const std::string& address,
                    double weight = 1.0,
                    const std::string& datacenter = "");

    /**
     * @brief Deregister a backend.
     * @param address Backend to remove.
     */
    void removeBackend(const std::string& address);

    /**
     * @brief Adjust a backend's weight (dynamic rebalancing).
     * @param address Backend to update.
     * @param weight  New weight (>= 0).
     */
    void updateWeight(const std::string& address, double weight);

    /**
     * @brief Return a snapshot of all registered backends.
     *
     * Atomics in the returned structs reflect the latest counts at the time of
     * the snapshot.  The values should be treated as approximate.
     */
    std::vector<Backend*> getBackends() const;

    // -------------------------------------------------------------------------
    // Routing
    // -------------------------------------------------------------------------

    /**
     * @brief Select a backend according to the configured strategy.
     * @return Address of the selected backend, or empty string if none available.
     */
    std::string selectBackend();

    /**
     * @brief Select a backend using consistent hashing on @p key.
     *
     * Provides sticky routing: the same key always maps to the same (healthy)
     * backend unless that backend becomes unavailable.
     *
     * @param key  Routing key (e.g., session ID, user ID).
     * @return Address of the selected backend, or empty string if none available.
     */
    std::string selectBackend(const std::string& key);

    /**
     * @brief Notify the load balancer that a routed request completed.
     *
     * Must be called after every request routed via selectBackend().
     *
     * @param address  Backend address returned by selectBackend().
     * @param success  True if the request completed successfully.
     */
    void onRequestComplete(const std::string& address, bool success);

    /**
     * @brief Increment active connection count for a backend.
     * @param address Backend address.
     */
    void onConnectionOpened(const std::string& address);

    /**
     * @brief Decrement active connection count for a backend.
     * @param address Backend address.
     */
    void onConnectionClosed(const std::string& address);

    // -------------------------------------------------------------------------
    // Raft State
    // -------------------------------------------------------------------------

    /** @brief Return true if this node is the current Raft leader. */
    bool isLeader() const;

    /** @brief Return the current Raft role. */
    RaftRole getRole() const;

    /** @brief Return the current Raft term. */
    uint64_t getCurrentTerm() const;

    // -------------------------------------------------------------------------
    // Observability
    // -------------------------------------------------------------------------

    /** @brief Return aggregate load-balancer statistics. */
    Stats getStats() const;

    /** @brief Return the active configuration. */
    const Config& getConfig() const { return config_; }

    /**
     * @brief Override the load-balancing strategy at runtime.
     * @param strategy New strategy to apply.
     */
    void setStrategy(LoadBalancingStrategy strategy);

    // -------------------------------------------------------------------------
    // Health Check Override (for testing)
    // -------------------------------------------------------------------------

    /**
     * @brief Inject a custom health-check function.
     *
     * Called once per health-check cycle per backend.  Must return true when
     * the backend is reachable and healthy, false otherwise.
     *
     * @param fn  Callable accepting a const Backend& and returning bool.
     */
    void setHealthCheckFn(std::function<bool(const Backend&)> fn);

private:
    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    /// Locate a backend by address (caller must hold backends_mutex_).
    Backend* findBackend(const std::string& address);

    /// Select via round-robin (caller must hold backends_mutex_).
    std::string selectRoundRobin();

    /// Select via least-connections (caller must hold backends_mutex_).
    std::string selectLeastConnections();

    /// Select via weighted round-robin (caller must hold backends_mutex_).
    std::string selectWeightedRoundRobin();

    /// Select via health-based routing (caller must hold backends_mutex_).
    std::string selectHealthBased();

    /// Select via consistent hashing (caller must hold backends_mutex_).
    std::string selectConsistentHash(const std::string& key);

    /// Filter to healthy backends in the preferred datacenter (or all if none).
    /// Caller must hold backends_mutex_.
    std::vector<Backend*> healthyBackends() const;

    /// Run one round of health checks (called from health_check_thread_).
    void runHealthChecks();

    /// Background health-check loop.
    void healthCheckLoop();

    /// Background Raft heartbeat / election loop.
    void raftLoop();

    /// Trigger dynamic weight rebalancing if load imbalance exceeds threshold.
    /// Called from the health-check thread.
    void maybeRebalance();

    /// Default health-check implementation (always returns true in unit tests).
    static bool defaultHealthCheck(const Backend& backend);

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    Config config_;

    mutable std::mutex backends_mutex_;
    std::vector<std::unique_ptr<Backend>> backends_;

    // Round-robin cursor
    std::atomic<size_t> rr_index_{0};

    // Smooth weighted round-robin: per-backend effective (current) weight,
    // keyed by backend address.  Protected by backends_mutex_.
    std::unordered_map<std::string, double> wrr_effective_weights_;

    // Raft state
    mutable std::mutex raft_mutex_;
    std::atomic<RaftRole> role_{RaftRole::FOLLOWER};
    std::atomic<uint64_t> current_term_{0};
    std::string voted_for_;
    std::chrono::steady_clock::time_point last_heartbeat_{};
    std::atomic<uint64_t> vote_count_{0};

    // Statistics
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> total_failed_{0};
    std::atomic<uint64_t> rebalance_events_{0};
    std::atomic<uint64_t> failover_events_{0};
    std::atomic<uint64_t> recovery_events_{0};

    // Health-check override
    std::function<bool(const Backend&)> health_check_fn_;

    // Background threads
    std::thread health_check_thread_;
    std::thread raft_thread_;
    std::atomic<bool> started_{false};   ///< True once start() launches threads.
    std::atomic<bool> shutdown_{false};  ///< Signals background threads to stop.
    std::mutex shutdown_mutex_;
    std::condition_variable shutdown_cv_;

    // Strategy (may be changed at runtime)
    std::atomic<LoadBalancingStrategy> strategy_;
};

} // namespace network
} // namespace themis
