/**
 * @file chaos_framework.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis::chaos {

// ─── Fault types ─────────────────────────────────────────────────────────────

enum class FaultType {
    NODE_FAILURE,           ///< Simulate a complete node crash
    NETWORK_PARTITION,      ///< Isolate a node from the cluster network
    LEADER_CRASH,           ///< Kill the current leader abruptly
    DELAYED_RESPONSE,       ///< Add artificial latency to responses
    DISK_FAILURE,           ///< Simulate storage I/O failure
    RANDOM_FAILURE,         ///< Inject random failure with configurable probability
    DISASTER_RECOVERY_DRILL ///< Simulate DR restore procedure
};

// ─── Fault specification ─────────────────────────────────────────────────────

struct FaultSpec {
    FaultType   type;
    std::string target_node_id = {};
    std::chrono::milliseconds duration{0};   ///< 0 = permanent until manually cleared
    double      probability{1.0};            ///< [0.0, 1.0] — used for RANDOM_FAILURE
    std::string description = {};

    FaultSpec() = default;
    FaultSpec(FaultType t, std::string node, std::chrono::milliseconds dur = {},
              double prob = 1.0, std::string desc = "")
        : type(t), target_node_id(std::move(node)), duration(dur),
          probability(prob), description(std::move(desc)) {}
};

// ─── Active fault record ─────────────────────────────────────────────────────

struct ActiveFault {
    FaultSpec                            spec;
    std::chrono::steady_clock::time_point injected_at;
    std::chrono::steady_clock::time_point expires_at;  ///< steady_clock::time_point::max() if permanent

    bool isExpired() const noexcept {
        if (expires_at == std::chrono::steady_clock::time_point::max()) {
          return false;
        }
        return std::chrono::steady_clock::now() >= expires_at;
    }
};

// ─── FaultInjector ───────────────────────────────────────────────────────────
//
// In-process fault registry: tracks which nodes are currently subject to which
// faults.  Does NOT perform real network/disk manipulation — designed for unit
// and integration tests where the SUT queries isFaultActive() before performing
// cluster operations.

/** @brief cluster operations. */
class FaultInjector {
public:
    using EventCallback = std::function<void(const FaultSpec&, bool /*injected*/)>;

    explicit FaultInjector(std::string injector_id = "default");
    ~FaultInjector();

    // Inject a fault. If the same node + type is already active, the existing
    // entry is updated in-place (last-writer-wins) and true is returned.
    bool injectFault(const FaultSpec& fault);

    // Clear the active fault on target_node_id (all types).
    // Returns false if no fault was registered.
    bool recoverFault(const std::string& target_node_id);

    // Clear the active fault for a specific type on target_node_id.
    bool recoverFault(const std::string& target_node_id, FaultType type);

    // Returns true when target_node_id currently has any active, non-expired fault.
    bool isFaultActive(const std::string& target_node_id) const;

    // Returns true when target_node_id has an active fault of the given type.
    bool isFaultActive(const std::string& target_node_id, FaultType type) const;

    // Snapshot of all faults (expires ones are pruned on access).
    std::vector<ActiveFault> getActiveFaults();

    // Total active (non-expired) fault count.
    size_t activeFaultCount();

    // Remove all active faults.
    void clearAllFaults();

    // Register a callback invoked on every inject/recover event.
    void registerEventCallback(EventCallback cb);

    const std::string& id() const noexcept { return injector_id_; }

private:
    void pruneExpired();

    std::string   injector_id_;
    mutable std::mutex fault_mutex_;

    // Key: node_id+"::"+type_string
    std::unordered_map<std::string, ActiveFault> active_faults_;

    std::vector<EventCallback> callbacks_;

    static std::string makeKey(const std::string& node_id, FaultType type);
    static std::string faultTypeName(FaultType type) noexcept;
};

// ─── ChaosSchedule entry ─────────────────────────────────────────────────────

struct ChaosScheduleEntry {
    std::chrono::steady_clock::time_point trigger_at;
    FaultSpec fault;
};

// ─── Wake strategy ───────────────────────────────────────────────────────────

/// Controls how the ChaosScheduler background thread wakes between ticks.
/// FIXED_TICK: sleep a fixed tick_interval on every loop iteration (original behaviour).
/// CONDVAR:    use a condition variable; wakes early when a new entry is scheduled
///             or when stop() is called.  tick_interval serves as the maximum wait.
enum class WakeStrategy {
    FIXED_TICK,  ///< Plain sleep_for(tick_interval) — simple, deterministic
    CONDVAR      ///< Condition-variable with tick_interval timeout — lower latency stop
};

// ─── ChaosScheduler configuration ───────────────────────────────────────────

/// Configuration for the ChaosScheduler background thread.
struct ChaosSchedulerConfig {
    /// Polling / maximum-wait interval for the background thread (default: 10 ms).
    std::chrono::milliseconds tick_interval{10};
    /// Wake strategy: FIXED_TICK (original) or CONDVAR (lower latency stop/schedule).
    WakeStrategy wake_strategy{WakeStrategy::FIXED_TICK};
};

// ─── ChaosScheduler ──────────────────────────────────────────────────────────
//
// Time-driven fault scheduler: fires scheduled faults in a background thread.
// Designed to compose with FaultInjector for orchestrated chaos scenarios.

/** @brief Designed to compose with FaultInjector for orchestrated chaos scenarios. */
class ChaosScheduler {
public:
    /// Convenience alias so callers can write ChaosScheduler::Config.
    using Config = ChaosSchedulerConfig;

    explicit ChaosScheduler(std::shared_ptr<FaultInjector> injector,
                            Config cfg = Config{});
    ~ChaosScheduler();

    // Schedule a future fault injection.
    void schedule(ChaosScheduleEntry entry);

    // Schedule using a relative delay from "now".
    void scheduleIn(std::chrono::milliseconds delay, const FaultSpec& fault);

    // Start the scheduler background thread.
    void start();

    // Stop the scheduler (drains pending entries, does not fire them).
    void stop();

    bool isRunning() const noexcept;

    size_t pendingCount() const;

    // Clear all pending (unfired) schedule entries.
    void clearPending();

private:
    void runLoop();

    std::shared_ptr<FaultInjector>    injector_;
    Config                            cfg_;
    std::vector<ChaosScheduleEntry>   pending_;
    mutable std::mutex                sched_mutex_;
    // Separate mutex for the condition variable so that schedule() can insert
    // entries into pending_ (using sched_mutex_) without being blocked by the
    // background thread holding wake_mutex_ during wait_for.
    std::mutex                        wake_mutex_;
    std::condition_variable           sched_cv_;
    std::atomic<bool>                 running_{false};
    std::thread                       worker_;
};

}  // namespace themis::chaos
