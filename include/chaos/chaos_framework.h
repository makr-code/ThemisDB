/**
 * @file chaos_framework.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class FaultInjector {
public:
    using EventCallback = std::function<void(const FaultSpec&, bool /*injected*/)>;

    explicit FaultInjector(std::string injector_id = "default");
    ~FaultInjector();

    /**
     * @brief Inject a fault.
     * @param[in] fault Input parameter.
     * @return True when the operation succeeds.
     * @details If the same node + type is already active, the existing entry is updated in-place (last-writer-wins) and true is returned.
     */
    bool injectFault(const FaultSpec& fault);

    /**
     * @brief Clear the active fault on target_node_id (all types).
     * @param[in] target_node_id Identifier of the target node.
     * @return True when the operation succeeds.
     * @details Returns false if no fault was registered.
     */
    bool recoverFault(const std::string& target_node_id);

    /**
     * @brief Clear the active fault for a specific type on target_node_id.
     * @param[in] target_node_id Identifier of the target node.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    bool recoverFault(const std::string& target_node_id, FaultType type);

    /**
     * @brief Returns true when target_node_id currently has any active, non-expired fault.
     * @param[in] target_node_id Identifier of the target node.
     * @return True when the operation succeeds.
     */
    bool isFaultActive(const std::string& target_node_id) const;

    /**
     * @brief Returns true when target_node_id has an active fault of the given type.
     * @param[in] target_node_id Identifier of the target node.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isFaultActive(const std::string& target_node_id, FaultType type) const;

    /**
     * @brief Snapshot of all faults (expires ones are pruned on access).
     * @return Return value.
     */
    std::vector<ActiveFault> getActiveFaults();

    /**
     * @brief Total active (non-expired) fault count.
     * @return Return value.
     */
    size_t activeFaultCount();

    /**
     * @brief Remove all active faults.
     */
    void clearAllFaults();

    /**
     * @brief Register a callback invoked on every inject/recover event.
     * @param[in] cb Input parameter.
     */
    void registerEventCallback(EventCallback cb);

    const std::string& id() const noexcept { return injector_id_; }

private:
    /**
     * @brief Prune Expired.
     */
    void pruneExpired();

    std::string   injector_id_;
    mutable std::mutex fault_mutex_;

    // Key: node_id+"::"+type_string
    std::unordered_map<std::string, ActiveFault> active_faults_;

    std::vector<EventCallback> callbacks_;

    /**
     * @brief Make Key.
     * @param[in] node_id Identifier of the node.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    static std::string makeKey(const std::string& node_id, FaultType type);
    /**
     * @brief Fault Type Name.
     * @param[in] type Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static std::string faultTypeName(FaultType type) noexcept;
};

// ─── ChaosSchedule entry ─────────────────────────────────────────────────────

struct ChaosScheduleEntry {
    std::chrono::steady_clock::time_point trigger_at;
    FaultSpec fault;
};

// ─── Wake strategy ───────────────────────────────────────────────────────────

enum class WakeStrategy {
    FIXED_TICK,  ///< Plain sleep_for(tick_interval) — simple, deterministic
    CONDVAR      ///< Condition-variable with tick_interval timeout — lower latency stop
};

// ─── ChaosScheduler configuration ───────────────────────────────────────────

struct ChaosSchedulerConfig {
    std::chrono::milliseconds tick_interval{10};
    WakeStrategy wake_strategy{WakeStrategy::FIXED_TICK};
};

// ─── ChaosScheduler ──────────────────────────────────────────────────────────
//
// Time-driven fault scheduler: fires scheduled faults in a background thread.
// Designed to compose with FaultInjector for orchestrated chaos scenarios.

class ChaosScheduler {
public:
    using Config = ChaosSchedulerConfig;

    explicit ChaosScheduler(std::shared_ptr<FaultInjector> injector,
                            Config cfg = Config{});
    ~ChaosScheduler();

    /**
     * @brief Schedule a future fault injection.
     * @param[in] entry Input parameter.
     */
    void schedule(ChaosScheduleEntry entry);

    /**
     * @brief Schedule using a relative delay from "now".
     * @param[in] delay Input parameter.
     * @param[in] fault Input parameter.
     */
    void scheduleIn(std::chrono::milliseconds delay, const FaultSpec& fault);

    /**
     * @brief Start the scheduler background thread.
     */
    void start();

    /**
     * @brief Stop the scheduler (drains pending entries, does not fire them).
     */
    void stop();

    /**
     * @brief Is Running.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isRunning() const noexcept;

    /**
     * @brief Pending Count.
     * @return Return value.
     */
    size_t pendingCount() const;

    /**
     * @brief Clear all pending (unfired) schedule entries.
     */
    void clearPending();

private:
    /**
     * @brief Run Loop.
     */
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
