/**
 * @file distributed_task_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "scheduler/task_scheduler.h"
#include "scheduler/scheduler_api_contract.h"
#include "sharding/distributed_coordinator.h"
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <optional>
#include <vector>
#include <map>
#include <functional>
#include <chrono>

namespace themis {

/**
 * @brief Coordinates TaskScheduler execution across a distributed cluster.
 *
 * DistributedTaskCoordinator is a thin orchestration layer that sits between
 * the application and TaskScheduler.  It listens for leader-election events
 * from DistributedCoordinator and activates / deactivates the local scheduler
 * accordingly, ensuring that only one node in the cluster executes scheduled
 * tasks at any given time.
 *
 * ### Thread safety
 * All public methods are thread-safe.
 *
 * ### Ownership model
 * The coordinator holds **non-owning raw pointers** to both the TaskScheduler
 * and the DistributedCoordinator.  The caller is responsible for keeping both
 * objects alive for at least as long as this coordinator.
 */
class DistributedTaskCoordinator {
public:
    /**
     * @brief Runtime configuration for the coordinator.
     */
    struct Config {
        /**
         * When true (default) the local TaskScheduler is started automatically
         * whenever this node is elected leader, and stopped when leadership is
         * transferred to another node.
         *
         * Set to false if you want manual control via activateScheduler() /
         * deactivateScheduler().
         */
        bool auto_manage_scheduler = true;
    };

    /**
     * @brief Construct a DistributedTaskCoordinator.
     *
     * @param scheduler   Local single-node task scheduler.  Must not be null.
     *                    The scheduler must NOT already be running when the
     *                    coordinator is started.
     * @param coordinator Gossip-based distributed coordinator used for leader
     *                    election.  Must not be null.
     *
     * @throws std::invalid_argument if either scheduler or coordinator is null.
     */
    explicit DistributedTaskCoordinator(
      TaskScheduler* scheduler,
      sharding::DistributedCoordinator* coordinator);

    /**
     * @brief Construct a DistributedTaskCoordinator.
     *
     * @param scheduler   Local single-node task scheduler.  Must not be null.
     *                    The scheduler must NOT already be running when the
     *                    coordinator is started.
     * @param coordinator Gossip-based distributed coordinator used for leader
     *                    election.  Must not be null.
     * @param config      Optional runtime configuration.
     *
     * @throws std::invalid_argument if either scheduler or coordinator is null.
     */
    DistributedTaskCoordinator(
        TaskScheduler* scheduler,
        sharding::DistributedCoordinator* coordinator,
      const Config& config);

    ~DistributedTaskCoordinator() noexcept;

    // Non-copyable, non-movable (holds raw pointers and threads).
    DistributedTaskCoordinator(const DistributedTaskCoordinator&) = delete;
    DistributedTaskCoordinator& operator=(const DistributedTaskCoordinator&) = delete;

    // ── Lifecycle ───────────────────────────────────────────────────────────

    /**
     * @brief Start the coordinator.
     *
     * Registers a leadership callback with the DistributedCoordinator.  If
     * this node is already the leader at start time, the TaskScheduler is
     * activated immediately.
     *
     * The underlying DistributedCoordinator must already be running (or will
     * be started externally).  This method does NOT call
     * DistributedCoordinator::start() on your behalf.
     */
    void start();

    /**
     * @brief Stop the coordinator.
     *
     * Deactivates the local scheduler (if running) and removes the leadership
     * callback from the DistributedCoordinator.  Safe to call multiple times.
     */
    void stop();

    /** @return true if the coordinator has been started and not yet stopped. */
    bool isRunning() const { return running_.load(); }

    // ── Leadership status ────────────────────────────────────────────────────

    /**
     * @return true if this node is currently the cluster leader and is
     *         actively scheduling tasks.
     */
    bool isLeader() const;

    /**
     * @return the node-id of the current cluster leader, or std::nullopt when
     *         no leader has been elected yet.
     */
    std::optional<std::string> getCurrentLeader() const;

    /**
     * @return the local node-id as reported by the DistributedCoordinator.
     */
    std::string getLocalNodeId() const;

    // ── Manual scheduler control ─────────────────────────────────────────────

    /**
     * @brief Manually activate the local scheduler.
     *
     * Registers all locally stored tasks with the TaskScheduler and starts it.
     * Normally called automatically when this node becomes leader; exposed here
     * for testing and custom integration scenarios.
     *
     * Idempotent – calling it when the scheduler is already active is a no-op.
     */
    void activateScheduler();

    /**
     * @brief Manually deactivate the local scheduler.
     *
     * Stops the TaskScheduler (waits for in-flight tasks to complete) and
     * unregisters all tasks from it.  The tasks remain in the local registry
     * and will be re-registered the next time activateScheduler() is called.
     *
     * Idempotent – calling it when the scheduler is already inactive is a no-op.
     */
    void deactivateScheduler();

    /** @return true if the local TaskScheduler is currently running. */
    bool isSchedulerActive() const;

    // ── Task management ──────────────────────────────────────────────────────

    /**
     * @brief Register a task with the coordinator.
     *
     * The task is stored in the local registry.  If this node is the current
     * leader, the task is also registered with the active TaskScheduler.
     *
     * @param task  Task definition.
     * @return      Assigned task ID.
     */
    std::string registerTask(const ScheduledTask& task);

    /**
     * @brief Unregister a task by ID.
     *
     * Removes the task from the local registry and, if this node is the leader,
     * from the active TaskScheduler as well.
     */
    void unregisterTask(const std::string& task_id);

    /**
     * @brief Enable a previously disabled task.
     *
     * Updates the local registry.  If this node is the leader, also enables
     * the task in the active TaskScheduler.
     */
    void enableTask(const std::string& task_id);

    /**
     * @brief Disable a task so it is not executed until re-enabled.
     *
     * Updates the local registry.  If this node is the leader, also disables
     * the task in the active TaskScheduler.
     */
    void disableTask(const std::string& task_id);

    /**
     * @brief Return all tasks in the local registry.
     */
    std::vector<ScheduledTask> listTasks() const;

    /**
     * @brief Return a specific task from the local registry.
     * @return Shared pointer to the task, or nullptr if not found.
     */
    std::shared_ptr<ScheduledTask> getTask(const std::string& task_id) const;

    // ── Accessors ────────────────────────────────────────────────────────────

    /** @return non-owning pointer to the underlying TaskScheduler. */
    TaskScheduler* getScheduler() const { return scheduler_; }

    /** @return non-owning pointer to the underlying DistributedCoordinator. */
    sharding::DistributedCoordinator* getCoordinator() const { return coordinator_; }

    // ── Statistics ───────────────────────────────────────────────────────────

    struct Stats {
        size_t registered_tasks = 0;   ///< Tasks in the local registry
        bool   scheduler_active = false; ///< Whether the local scheduler is running
        bool   is_leader        = false; ///< Whether this node is currently the leader
        size_t leadership_acquired = 0; ///< Number of times this node became leader
        size_t leadership_lost     = 0; ///< Number of times this node lost leadership
    };

    Stats getStats() const;

    // ── Leadership notification ──────────────────────────────────────────────

    /**
     * @brief Notify the coordinator that a new leader has been elected.
     *
     * This method is called automatically via the DistributedCoordinator
     * leadership callback.  It is also exposed publicly to allow:
     * - Testing without a real gossip network (call directly with a synthetic
     *   leader ID)
     * - Custom integration in deployments that use alternative leader-election
     *   mechanisms
     *
     * @param leader_id  Node-ID of the newly elected leader.
     */
    void onLeaderElected(const std::string& leader_id);

    // ── Coordination Health and Resilience ────────────────────────────────────

    /**
     * @brief Attempt to acquire leadership with an explicit timeout.
     *
     * This method performs a synchronous leadership acquisition attempt against
     * the underlying DistributedCoordinator, with a bounded timeout to detect
     * coordination layer unavailability.  If the coordinator is unreachable or
     * fails to respond within the timeout, this method fails explicitly rather
     * than blocking indefinitely.
     *
     * Used to detect and fail fast on coordination layer failures in production
     * scenarios where the gossip network may be partitioned or degraded.
     *
     * @param timeout_ms  Maximum milliseconds to wait for leadership acquisition.
     * @return true if this node successfully acquired leadership, false otherwise.
     *         Note: false does NOT mean "not leader yet" but specifically that
     *         the acquisition attempt timed out or the coordinator is unavailable.
     *
     * @throws std::exception on internal coordinator errors.
     */
    bool acquireLeadershipWithTimeout(std::chrono::milliseconds timeout_ms);

    /**
     * @brief Start a background heartbeat thread to detect coordinator failure.
     *
     * This method spawns a background thread that periodically checks the health
     * of the underlying DistributedCoordinator.  If the coordinator becomes
     * unresponsive (heartbeat timeout), the method automatically deactivates the
     * local scheduler to prevent split-brain execution.
     *
     * The heartbeat interval is derived from the coordinator's health check period.
     * If this coordinator's running state becomes false, the heartbeat thread
     * exits gracefully.
     *
     * This implements the fail-closed contract: if coordination is lost, the
     * scheduler stops accepting new task executions.
     *
     * @param heartbeat_interval_ms  Interval between heartbeat checks.
     *
     * @return true if heartbeat monitoring was successfully activated,
     *         false if it was already running or if activation failed.
     */
    bool maintainHeartbeat(std::chrono::milliseconds heartbeat_interval_ms);

    /**
     * @brief Detect and handle split-brain / consensus failure scenarios.
     *
     * Examines the current coordinator state to detect conditions where the
     * distributed consensus mechanism has diverged (e.g., multiple leaders,
     * replicas out of sync).  When such a condition is detected:
     * - Logs the divergence with node IDs and timestamps
     * - Deactivates the local scheduler if this node is a leader
     * - Returns an error code indicating the consensus failure
     *
     * This is a fail-closed operation: in case of doubt, the scheduler stops.
     *
     * @return SchedulerError::kSuccess if consensus is healthy,
     *         SchedulerError::kCoordinationError if split-brain detected,
     *         other SchedulerError codes for other failures.
     *
     * @see scheduler_api_contract.h for error taxonomy.
     */
    SchedulerError handleSplitBrainDetection();

private:
    TaskScheduler*                      scheduler_;
    sharding::DistributedCoordinator*   coordinator_;
    Config                              config_;

    std::atomic<bool>   running_{false};
    std::atomic<bool>   scheduler_active_{false};

    // Local task registry – tasks stored here regardless of leader status.
    mutable std::mutex                              registry_mutex_;
    std::map<std::string, ScheduledTask>            task_registry_;

    // Leadership tracking
    mutable std::mutex  leadership_mutex_;
    std::string         current_leader_;
    std::atomic<size_t> leadership_acquired_{0};
    std::atomic<size_t> leadership_lost_{0};

    // Heartbeat and coordination health tracking
    std::atomic<bool>                   heartbeat_active_{false};
    mutable std::mutex                  heartbeat_mutex_;
    std::condition_variable             heartbeat_cv_;
    std::unique_ptr<std::thread>        heartbeat_thread_;
    std::atomic<std::chrono::milliseconds> last_heartbeat_ms_{0};
    std::atomic<size_t>                 coordination_failures_{0};

    // Generate a task ID for tasks without one (mirrors TaskScheduler logic).
    static std::string generateId(const ScheduledTask& task);

    // Helper: runs heartbeat monitoring in a background thread
    void heartbeatMonitorThread(std::chrono::milliseconds interval_ms);
};

} // namespace themis
