/**
 * @file distributed_task_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scheduler/distributed_task_coordinator.h"
#include "utils/logger.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <stdexcept>

namespace themis {

// ── Constructor / Destructor ─────────────────────────────────────────────────

DistributedTaskCoordinator::DistributedTaskCoordinator(
    TaskScheduler* scheduler,
    sharding::DistributedCoordinator* coordinator)
    : scheduler_(scheduler),
      coordinator_(coordinator),
      config_{},
      running_(false),
      scheduler_active_(false),
      heartbeat_active_(false),
      leadership_acquired_(0),
      leadership_lost_(0),
      coordination_failures_(0),
      last_heartbeat_ms_(std::chrono::milliseconds(0))
{
    // Phase 3: Structured validation error logging
    if (!scheduler_) {
        THEMIS_ERROR(
            "[DistributedTaskCoordinator::DistributedTaskCoordinator] "
            "code={} msg='scheduler cannot be null' context={{}}",
            static_cast<int>(themis::scheduler::SchedulerError::kInternalError));
        throw std::invalid_argument("DistributedTaskCoordinator: scheduler cannot be null");
    }
    if (!coordinator_) {
        THEMIS_ERROR(
            "[DistributedTaskCoordinator::DistributedTaskCoordinator] "
            "code={} msg='coordinator cannot be null' context={{}}",
            static_cast<int>(themis::scheduler::SchedulerError::kInternalError));
        throw std::invalid_argument("DistributedTaskCoordinator: coordinator cannot be null");
    }
}

DistributedTaskCoordinator::DistributedTaskCoordinator(
    TaskScheduler* scheduler,
    sharding::DistributedCoordinator* coordinator,
    const Config& config)
    : scheduler_(scheduler),
      coordinator_(coordinator),
      config_(config),
      running_(false),
      scheduler_active_(false),
      heartbeat_active_(false),
      leadership_acquired_(0),
      leadership_lost_(0),
      coordination_failures_(0),
      last_heartbeat_ms_(std::chrono::milliseconds(0))
{
    // Phase 3: Structured validation error logging
    if (!scheduler_) {
        THEMIS_ERROR(
            "[DistributedTaskCoordinator::DistributedTaskCoordinator] "
            "code={} msg='scheduler cannot be null' context={{}}",
            static_cast<int>(themis::scheduler::SchedulerError::kInternalError));
        throw std::invalid_argument("DistributedTaskCoordinator: scheduler cannot be null");
    }
    if (!coordinator_) {
        THEMIS_ERROR(
            "[DistributedTaskCoordinator::DistributedTaskCoordinator] "
            "code={} msg='coordinator cannot be null' context={{}}",
            static_cast<int>(themis::scheduler::SchedulerError::kInternalError));
        throw std::invalid_argument("DistributedTaskCoordinator: coordinator cannot be null");
    }

    THEMIS_INFO("DistributedTaskCoordinator created for node: {}",
                coordinator_->getLocalShardId());
}

DistributedTaskCoordinator::~DistributedTaskCoordinator() noexcept {
    try {
        stop();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception in DistributedTaskCoordinator destructor: {}", e.what());
    }
}

/**
 * @brief ── Lifecycle ────────────────────────────────────────────────────────────────
 * @details Calls: exchange(), THEMIS_WARN(), setLeaderElectedCallback(), onLeaderElected(), isLeader(), THEMIS_INFO(), activateScheduler().
 */

void DistributedTaskCoordinator::start() {
    if (running_.exchange(true)) {
        THEMIS_WARN("DistributedTaskCoordinator already running");
        return;
    }

    // Register leadership callback so we learn about leader changes.
    coordinator_->setLeaderElectedCallback(
        [this](const std::string& leader_id) {
            onLeaderElected(leader_id);
        });

    // If we are already the leader (coordinator was started before us),
    // activate the scheduler immediately.
    if (coordinator_->isLeader() && config_.auto_manage_scheduler) {
        THEMIS_INFO("DistributedTaskCoordinator: node is already leader at start – activating scheduler");
        activateScheduler();
    }

    THEMIS_INFO("DistributedTaskCoordinator started");
}

/**
 * @brief Stop.
 * @details Calls: exchange(), lock(), notify_all(), joinable(), join(), deactivateScheduler(), setLeaderElectedCallback(), THEMIS_INFO().
 */
void DistributedTaskCoordinator::stop() {
    if (!running_.exchange(false)) {
        return;  // Already stopped.
    }

    // Stop the heartbeat monitor first
    if (heartbeat_active_.exchange(false)) {
        {
            std::lock_guard<std::mutex> lock(heartbeat_mutex_);
            heartbeat_cv_.notify_all();
        }
        if (heartbeat_thread_ && heartbeat_thread_->joinable()) {
            heartbeat_thread_->join();
        }
    }

    // Deactivate scheduler before removing the leadership callback.
    if (config_.auto_manage_scheduler) {
        deactivateScheduler();
    }

    // Remove the leadership callback so we no longer react to elections.
    coordinator_->setLeaderElectedCallback(nullptr);

    THEMIS_INFO("DistributedTaskCoordinator stopped");
}

// ── Leadership status ─────────────────────────────────────────────────────────

bool DistributedTaskCoordinator::isLeader() const {
    return coordinator_->isLeader();
}

std::optional<std::string> DistributedTaskCoordinator::getCurrentLeader() const {
    return coordinator_->getCurrentLeader();
}

std::string DistributedTaskCoordinator::getLocalNodeId() const {
    return coordinator_->getLocalShardId();
}

/**
 * @brief ── Manual scheduler control ──────────────────────────────────────────────────
 * @details Calls: exchange(), THEMIS_INFO(), lock(), size(), registerTask(), THEMIS_WARN(), what(), start().
 */

void DistributedTaskCoordinator::activateScheduler() {
    if (scheduler_active_.exchange(true)) {
        return;  // Already active.
    }

    THEMIS_INFO("DistributedTaskCoordinator: activating local scheduler");

    // Register all locally stored tasks with the scheduler.
    size_t task_count = 0;
    {
        // Level 1: Acquire registry lock (safe to acquire after all higher-level locks released)
        std::lock_guard<std::mutex> lock(registry_mutex_);
        task_count = task_registry_.size();
        for (const auto& [id, task] : task_registry_) {
            try {
                scheduler_->registerTask(task);
            } catch (const std::exception& ex) {
                THEMIS_WARN("DistributedTaskCoordinator: failed to register task {} on activation: {}",
                            id, ex.what());
            }
        }
    }

    scheduler_->start();
    THEMIS_INFO("DistributedTaskCoordinator: local scheduler activated ({} tasks registered)",
                task_count);
}

/**
 * @brief Deactivate Scheduler.
 * @details Calls: exchange(), THEMIS_INFO(), stop(), lock(), unregisterTask().
 */
void DistributedTaskCoordinator::deactivateScheduler() {
    if (!scheduler_active_.exchange(false)) {
        return;  // Already inactive.
    }

    THEMIS_INFO("DistributedTaskCoordinator: deactivating local scheduler");
    scheduler_->stop();

    // Unregister all tasks from the scheduler so the next activation starts
    // from a clean state (avoids duplicate registrations).
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        for (const auto& [id, task] : task_registry_) {
            try {
                scheduler_->unregisterTask(id);
            } catch (const std::exception& ex) {
                // Ignore: task may not have been registered if there was an
                // error during activation.
            }
        }
    }

    THEMIS_INFO("DistributedTaskCoordinator: local scheduler deactivated");
}

bool DistributedTaskCoordinator::isSchedulerActive() const {
    return scheduler_active_.load();
}

/**
 * @brief ── Task management ───────────────────────────────────────────────────────────
 * @param[in] task Input parameter.
 * @return Return value.
 * @details Calls: empty(), generateId(), lock(), load(), THEMIS_WARN(), what(), THEMIS_DEBUG().
 */

std::string DistributedTaskCoordinator::registerTask(const ScheduledTask& task) {
    // Assign an ID if one was not provided (mirrors TaskScheduler behaviour).
    ScheduledTask stored = task;
    if (stored.id.empty()) {
        stored.id = generateId(task);
    }

    const std::string task_id = stored.id;

    {
        // Level 1: Acquire registry lock
        std::lock_guard<std::mutex> lock(registry_mutex_);
        task_registry_[task_id] = stored;
    }

    // If this node is the active leader, also register with the live scheduler.
    if (scheduler_active_.load()) {
        try {
            scheduler_->registerTask(stored);
        } catch (const std::exception& ex) {
            THEMIS_WARN("DistributedTaskCoordinator: could not register task {} with scheduler: {}",
                        task_id, ex.what());
        }
    }

    THEMIS_DEBUG("DistributedTaskCoordinator: registered task {} (leader={})",
                 task_id, scheduler_active_.load());
    return task_id;
}

/**
 * @brief Unregister Task.
 * @param[in] task_id Identifier of the task.
 * @details Calls: lock(), erase(), load(), THEMIS_WARN(), what().
 */
void DistributedTaskCoordinator::unregisterTask(const std::string& task_id) {
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        task_registry_.erase(task_id);
    }

    if (scheduler_active_.load()) {
        try {
            scheduler_->unregisterTask(task_id);
        } catch (const std::exception& ex) {
            THEMIS_WARN("DistributedTaskCoordinator: could not unregister task {} from scheduler: {}",
                        task_id, ex.what());
        }
    }
}

/**
 * @brief Enable Task.
 * @param[in] task_id Identifier of the task.
 * @details Calls: lock(), find(), end(), load(), THEMIS_WARN(), what().
 */
void DistributedTaskCoordinator::enableTask(const std::string& task_id) {
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        auto it = task_registry_.find(task_id);
        if (it != task_registry_.end()) {
            it->second.enabled = true;
        }
    }

    if (scheduler_active_.load()) {
        try {
            scheduler_->enableTask(task_id);
        } catch (const std::exception& ex) {
            THEMIS_WARN("DistributedTaskCoordinator: could not enable task {} in scheduler: {}",
                        task_id, ex.what());
        }
    }
}

/**
 * @brief Disable Task.
 * @param[in] task_id Identifier of the task.
 * @details Calls: lock(), find(), end(), load(), THEMIS_WARN(), what().
 */
void DistributedTaskCoordinator::disableTask(const std::string& task_id) {
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        auto it = task_registry_.find(task_id);
        if (it != task_registry_.end()) {
            it->second.enabled = false;
        }
    }

    if (scheduler_active_.load()) {
        try {
            scheduler_->disableTask(task_id);
        } catch (const std::exception& ex) {
            THEMIS_WARN("DistributedTaskCoordinator: could not disable task {} in scheduler: {}",
                        task_id, ex.what());
        }
    }
}

std::vector<ScheduledTask> DistributedTaskCoordinator::listTasks() const {
    /**
     * @brief Lock.
     * @param[in] registry_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<ScheduledTask> result = {};

    result.reserve(task_registry_.size());
    for (const auto& [id, task] : task_registry_) {
        result.push_back(task);
    }
    return result;
}

std::shared_ptr<ScheduledTask> DistributedTaskCoordinator::getTask(
    const std::string& task_id) const
{
    /**
     * @brief Lock.
     * @param[in] registry_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = task_registry_.find(task_id);
    if (it == task_registry_.end()) {
        return nullptr;
    }
    return std::make_shared<ScheduledTask>(it->second);
}

// ── Statistics ────────────────────────────────────────────────────────────────

DistributedTaskCoordinator::Stats DistributedTaskCoordinator::getStats() const {
    /**
     * @brief Level 1: Acquire registry lock for task count
     * @param[in] registry_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(registry_mutex_);
    Stats s;
    s.registered_tasks    = task_registry_.size();
    s.scheduler_active    = scheduler_active_.load();
    s.is_leader           = coordinator_->isLeader();
    s.leadership_acquired = leadership_acquired_.load();
    s.leadership_lost     = leadership_lost_.load();
    return s;
}

/**
 * @brief ── Private helpers ───────────────────────────────────────────────────────────
 * @param[in] leader_id Identifier of the leader.
 * @details Calls: load(), getLocalShardId(), lock(), THEMIS_INFO(), fetch_add(), activateScheduler(), deactivateScheduler().
 */

void DistributedTaskCoordinator::onLeaderElected(const std::string& leader_id) {
    // If the coordinator has been stopped, ignore late callbacks that may
    // fire in the window between running_.exchange(false) and
    // setLeaderElectedCallback(nullptr) in stop().
    if (!running_.load()) {
        return;
    }

    const std::string my_id = coordinator_->getLocalShardId();
    const bool i_am_leader  = (leader_id == my_id);

    {
        // Level 2: Acquire leadership lock (can be acquired after no lower locks)
        std::lock_guard<std::mutex> lock(leadership_mutex_);
        current_leader_ = leader_id;
    }

    if (i_am_leader) {
        THEMIS_INFO("DistributedTaskCoordinator: this node ({}) became leader – activating scheduler",
                    my_id);
        leadership_acquired_.fetch_add(1);
        if (config_.auto_manage_scheduler) {
            // activateScheduler() will acquire registry_mutex_ (Level 1)
            activateScheduler();
        }
    } else {
        if (scheduler_active_.load()) {
            THEMIS_INFO("DistributedTaskCoordinator: leadership transferred to {} – deactivating scheduler",
                        leader_id);
            leadership_lost_.fetch_add(1);
            if (config_.auto_manage_scheduler) {
                // deactivateScheduler() will acquire registry_mutex_ (Level 1)
                deactivateScheduler();
            }
        }
    }
}

/**
 * @brief Generate Id.
 * @param[in] task Input parameter.
 * @return Return value.
 * @details Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), empty(), std::isalnum(), str().
 */
std::string DistributedTaskCoordinator::generateId(const ScheduledTask& task) {
    // Mirror the simple ID generation from TaskScheduler::generateTaskId.
    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()).count();
    std::ostringstream oss = {};
    oss << "task_" << std::hex << ms;
    if (!task.name.empty()) {
        oss << "_";
        for (char c : task.name) {
            oss << (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
        }
    }
    return oss.str();
}

/**
 * @brief ── Coordination Health and Resilience ──────────────────────────────────────
 * @param[in] timeout_ms Input parameter.
 * @return True when the operation succeeds.
 */

bool DistributedTaskCoordinator::acquireLeadershipWithTimeout(
    std::chrono::milliseconds timeout_ms)
{
    if (!running_.load()) {
        THEMIS_WARN("DistributedTaskCoordinator: cannot acquire leadership; coordinator not running");
        return false;
    }

    try {
        // Attempt to acquire leadership with explicit timeout.
        // This is a fail-fast check to detect coordination layer issues.
        auto start = std::chrono::steady_clock::now();
        bool acquired = false;

        // Use the coordinator's internal mechanism to attempt leadership.
        // If the coordinator is unresponsive, this should timeout.
        // (Implementation assumes coordinator has some form of timeout-aware API)
        if (coordinator_->isLeader()) {
            acquired = true;
            THEMIS_DEBUG("DistributedTaskCoordinator: already leader");
        } else {
            // Try to participate in the leadership election with a timeout.
            // This is a simplified check; production systems may use more
            // sophisticated timing or heartbeat mechanisms.
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > timeout_ms) {
                THEMIS_WARN("DistributedTaskCoordinator: leadership acquisition timed out after {}ms",
                           timeout_ms.count());
                coordination_failures_.fetch_add(1);
                return false;
            }
            // For now, if we're not already leader, acquisition fails.
            // In production, this would interact with the leader-election protocol.
            THEMIS_INFO("DistributedTaskCoordinator: not currently leader; election in progress");
            acquired = false;
        }

        return acquired;
    } catch (const std::exception& ex) {
        THEMIS_ERROR("DistributedTaskCoordinator: error during leadership acquisition: {}", ex.what());
        coordination_failures_.fetch_add(1);
        return false;
    }
}

/**
 * @brief Maintain Heartbeat.
 * @param[in] heartbeat_interval_ms Input parameter.
 * @return True when the operation succeeds.
 */
bool DistributedTaskCoordinator::maintainHeartbeat(
    std::chrono::milliseconds heartbeat_interval_ms)
{
    if (heartbeat_active_.exchange(true)) {
        THEMIS_WARN("DistributedTaskCoordinator: heartbeat already active");
        return false;
    }

    THEMIS_INFO("DistributedTaskCoordinator: starting heartbeat monitoring (interval={}ms)",
               heartbeat_interval_ms.count());

    try {
        heartbeat_thread_ = std::make_unique<std::thread>(
            &DistributedTaskCoordinator::heartbeatMonitorThread,
            this,
            heartbeat_interval_ms
        );
        return true;
    } catch (const std::exception& ex) {
        THEMIS_ERROR("DistributedTaskCoordinator: failed to start heartbeat thread: {}", ex.what());
        heartbeat_active_.store(false);
        return false;
    }
}

/**
 * @brief Heartbeat Monitor Thread.
 * @param[in] interval_ms Input parameter.
 */
void DistributedTaskCoordinator::heartbeatMonitorThread(
    std::chrono::milliseconds interval_ms)
{
    THEMIS_DEBUG("DistributedTaskCoordinator: heartbeat monitor thread started");

    while (running_.load() && heartbeat_active_.load()) {
        try {
            // Record the heartbeat timestamp
            auto now = std::chrono::steady_clock::now();
            last_heartbeat_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch());

            // Check coordinator health
            if (!coordinator_->isHealthy()) {
                THEMIS_WARN("DistributedTaskCoordinator: heartbeat failure detected – coordinator unhealthy");
                coordination_failures_.fetch_add(1);

                // Fail-closed: deactivate the scheduler to prevent split-brain
                if (scheduler_active_.load()) {
                    THEMIS_WARN("DistributedTaskCoordinator: deactivating scheduler due to coordination failure");
                    deactivateScheduler();
                }
            }

            // Sleep until the next heartbeat interval or until shutdown
            {
                /**
                 * @brief Lock.
                 * @param[in] heartbeat_mutex_ Input parameter.
                 * @return Return value.
                 */
                std::unique_lock<std::mutex> lock(heartbeat_mutex_);
                heartbeat_cv_.wait_for(lock, interval_ms,
                    [this]() { return !running_.load() || !heartbeat_active_.load(); });
            }
        } catch (const std::exception& ex) {
            THEMIS_ERROR("DistributedTaskCoordinator: error in heartbeat monitor: {}", ex.what());
            coordination_failures_.fetch_add(1);
        }
    }

    THEMIS_DEBUG("DistributedTaskCoordinator: heartbeat monitor thread exiting");
}

/**
 * @brief Handle Split Brain Detection.
 * @return Return value.
 */
SchedulerError DistributedTaskCoordinator::handleSplitBrainDetection()
{
    if (!running_.load()) {
        THEMIS_WARN(
            "[DistributedTaskCoordinator::handleSplitBrainDetection] "
            "code={} msg='split-brain check requested but coordinator not running' "
            "context={{running={}, coordinator_active=false}}",
            static_cast<int>(themis::scheduler::SchedulerError::kCoordinationError),
            running_.load());
        return themis::scheduler::SchedulerError::kCoordinationError;
    }

    try {
        // Check for split-brain conditions: multiple leaders, stale replicas, etc.
        const std::string my_id = coordinator_->getLocalShardId();
        const std::optional<std::string> current_leader = coordinator_->getCurrentLeader();

        {
            /**
             * @brief Lock.
             * @param[in] leadership_mutex_ Input parameter.
             * @return Return value.
             */
            std::lock_guard<std::mutex> lock(leadership_mutex_);

            // If we have a stored leader ID and it differs from the current leader,
            // this may indicate a leadership change or split-brain condition.
            if (!current_leader_.empty() && current_leader.has_value()) {
                if (current_leader_ != current_leader.value()) {
                    THEMIS_WARN(
                        "[DistributedTaskCoordinator::handleSplitBrainDetection] "
                        "code={} msg='split-brain detected' "
                        "context={{node_id='{}', previous_leader='{}', current_leader='{}', "
                        "scheduler_active={}}}",
                        static_cast<int>(themis::scheduler::SchedulerError::kCoordinationError),
                        my_id, current_leader_, current_leader ? *current_leader : "unknown",
                        scheduler_active_.load());

                    // Fail-closed: deactivate scheduler if this node is a leader
                    if (scheduler_active_.load()) {
                        THEMIS_INFO("DistributedTaskCoordinator: deactivating scheduler due to split-brain detection");
                        deactivateScheduler();
                    }

                    coordination_failures_.fetch_add(1);
                    return themis::scheduler::SchedulerError::kCoordinationError;
                }
            }

            // Update the current leader for next check
            if (current_leader) {
                current_leader_ = *current_leader;
            }
        }

        THEMIS_DEBUG("DistributedTaskCoordinator: split-brain check passed (leader={})",
                    current_leader ? *current_leader : "unknown");
        return themis::scheduler::SchedulerError::kSuccess;
    } catch (const std::exception& ex) {
        THEMIS_ERROR(
            "[DistributedTaskCoordinator::handleSplitBrainDetection] "
            "code={} msg='error during split-brain detection' exception='{}' "
            "context={{coordinator_available=true}}",
            static_cast<int>(themis::scheduler::SchedulerError::kCoordinationError),
            ex.what());
        coordination_failures_.fetch_add(1);
        return themis::scheduler::SchedulerError::kCoordinationError;
    }
}

} // namespace themis
