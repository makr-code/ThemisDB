/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_task_coordinator.h                     ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-13 20:25:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     330                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 39ac8c3efe  2026-03-20  Split default-arg constructors into overloads ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 6bdc7ab4ad  2026-03-01  feat(scheduler): implement distributed cron leader electi... ║
    • 49cd5bf585  2026-02-22  Implement distributed task coordination across nodes (Pha... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file distributed_task_coordinator.h
 * @brief Distributed task coordination across cluster nodes.
 *
 * Wraps a single-node TaskScheduler with leadership awareness so that
 * scheduled tasks are executed on exactly one node at a time within a
 * ThemisDB cluster.
 *
 * ### How it works
 * 1. Every node creates a DistributedTaskCoordinator and registers the
 *    same set of tasks on it.
 * 2. Internally, the coordinator uses the DistributedCoordinator
 *    (gossip-based leader election in the sharding module) to learn
 *    which node is the cluster leader.
 * 3. Only the **leader** node activates the underlying TaskScheduler and
 *    executes tasks.  Non-leader nodes keep the task registry in memory
 *    but do not execute anything.
 * 4. When a new leader is elected (e.g. after a failure), it re-registers
 *    all locally stored tasks and starts the TaskScheduler, achieving
 *    automatic failover with no duplicate executions.
 *
 * ### Integration example
 * @code
 *   // Existing single-node scheduler
 *   TaskScheduler::Config sched_cfg;
 *   TaskScheduler scheduler(query_engine, sched_cfg);
 *
 *   // Sharding infrastructure (already present in a cluster deployment)
 *   auto topology  = std::make_shared<ShardTopology>();
 *   auto gossip    = std::make_shared<GossipConfigManager>(gossip_cfg, topology);
 *   DistributedCoordinator dc("node-1", topology, gossip);
 *
 *   // Wire them together
 *   DistributedTaskCoordinator dtc(&scheduler, &dc);
 *   dtc.start();
 *
 *   // Register tasks – identical on every node.
 *   // The task only *runs* on the current leader.
 *   ScheduledTask t;
 *   t.name = "nightly-cleanup";
 *   t.interval = std::chrono::hours(24);
 *   dtc.registerTask(t);
 *
 *   // ... application runs ...
 *   dtc.stop();
 * @endcode
 */

#ifndef THEMIS_SCHEDULER_DISTRIBUTED_TASK_COORDINATOR_H
#define THEMIS_SCHEDULER_DISTRIBUTED_TASK_COORDINATOR_H

#include "scheduler/task_scheduler.h"
#include "sharding/distributed_coordinator.h"
#include <atomic>
#include <mutex>
#include <string>
#include <optional>
#include <vector>
#include <map>
#include <functional>

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
     * @param config      Optional runtime configuration.
     *
     * @throws std::invalid_argument if either scheduler or coordinator is null.
     */
    explicit DistributedTaskCoordinator(
      TaskScheduler* scheduler,
      sharding::DistributedCoordinator* coordinator);

    DistributedTaskCoordinator(
        TaskScheduler* scheduler,
        sharding::DistributedCoordinator* coordinator,
      const Config& config);

    ~DistributedTaskCoordinator();

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

    // Generate a task ID for tasks without one (mirrors TaskScheduler logic).
    static std::string generateId(const ScheduledTask& task);
};

} // namespace themis

#endif // THEMIS_SCHEDULER_DISTRIBUTED_TASK_COORDINATOR_H
