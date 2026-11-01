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
#include <exception>
#include <initializer_list>

namespace themis {
namespace {

TaskScheduler::RequestContext makeInternalSchedulerContext(
    std::initializer_list<std::string> permissions) {
    TaskScheduler::RequestContext context;
    context.user_id = "system:scheduler-coordinator";
    context.authorization_justification = "trusted scheduler coordinator operation";
    context.granted_permissions.insert(permissions.begin(), permissions.end());
    return context;
}

} // namespace

// ── Constructor / Destructor ─────────────────────────────────────────────────

DistributedTaskCoordinator::DistributedTaskCoordinator(
    TaskScheduler* scheduler,
    sharding::DistributedCoordinator* coordinator)
    : scheduler_(scheduler),
      coordinator_(coordinator),
      config_{},
      execution_gate_(std::make_shared<std::atomic<bool>>(false)),
      callback_state_(std::make_shared<LeadershipCallbackState>()),
      transition_mutex_(),
      transition_cv_(),
      running_(false),
      scheduler_active_(false),
      registry_mutex_(),
      task_registry_(),
      leadership_mutex_(),
      current_leader_(),
      leadership_acquired_(0),
      leadership_lost_(0),
      heartbeat_active_(false),
      heartbeat_mutex_(),
      heartbeat_cv_(),
      heartbeat_thread_(),
      last_heartbeat_ms_(std::chrono::milliseconds(0)),
      coordination_failures_(0)
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
    if (config_.heartbeat_check_interval <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument(
            "DistributedTaskCoordinator: heartbeat_check_interval must be positive");
    }
    callback_state_->owner = this;
    scheduler_->setDistributedExecutionGate(execution_gate_);
}

DistributedTaskCoordinator::DistributedTaskCoordinator(
    TaskScheduler* scheduler,
    sharding::DistributedCoordinator* coordinator,
    const Config& config)
    : scheduler_(scheduler),
      coordinator_(coordinator),
      config_(config),
      execution_gate_(std::make_shared<std::atomic<bool>>(false)),
      callback_state_(std::make_shared<LeadershipCallbackState>()),
      transition_mutex_(),
      transition_cv_(),
      running_(false),
      scheduler_active_(false),
      registry_mutex_(),
      task_registry_(),
      leadership_mutex_(),
      current_leader_(),
      leadership_acquired_(0),
      leadership_lost_(0),
      heartbeat_active_(false),
      heartbeat_mutex_(),
      heartbeat_cv_(),
      heartbeat_thread_(),
      last_heartbeat_ms_(std::chrono::milliseconds(0)),
      coordination_failures_(0)
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
    if (config_.heartbeat_check_interval <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument(
            "DistributedTaskCoordinator: heartbeat_check_interval must be positive");
    }
    callback_state_->owner = this;
    scheduler_->setDistributedExecutionGate(execution_gate_);

    THEMIS_INFO("DistributedTaskCoordinator created for node: {}",
                coordinator_->getLocalShardId());
}

DistributedTaskCoordinator::~DistributedTaskCoordinator() noexcept {
    try {
        stop();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception in DistributedTaskCoordinator destructor: {}", e.what());
    }
    {
        std::unique_lock<std::mutex> lock(transition_mutex_);
        transition_cv_.wait(lock, [this] {
            return !transition_in_progress_;
        });
    }
    detachLeadershipCallbackOwnerAndWait();
}

void DistributedTaskCoordinator::dispatchLeadershipCallback(
    const std::shared_ptr<LeadershipCallbackState>& state,
    const std::string& leader_id) {
    DistributedTaskCoordinator* owner = nullptr;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (!state->owner) {
            return;
        }
        owner = state->owner;
        ++state->active_callbacks;
    }

    struct CallbackCompletion {
        std::shared_ptr<LeadershipCallbackState> state;
        ~CallbackCompletion() {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (--state->active_callbacks == 0) {
                state->cv.notify_all();
            }
        }
    } completion{state};

    try {
        owner->onLeaderElected(leader_id);
    } catch (const std::exception& ex) {
        try {
            owner->deactivateScheduler();
        } catch (const std::exception& cleanup_ex) {
            THEMIS_ERROR(
                "DistributedTaskCoordinator: callback cleanup failed: {}",
                cleanup_ex.what());
        } catch (...) {
            THEMIS_ERROR(
                "DistributedTaskCoordinator: callback cleanup failed with an unknown error");
        }
        THEMIS_ERROR(
            "DistributedTaskCoordinator: leadership callback failed for leader {}: {}",
            leader_id, ex.what());
    } catch (...) {
        try {
            owner->deactivateScheduler();
        } catch (...) {
            THEMIS_ERROR(
                "DistributedTaskCoordinator: callback cleanup failed with an unknown error");
        }
        THEMIS_ERROR(
            "DistributedTaskCoordinator: leadership callback failed for leader {}",
            leader_id);
    }
}

void DistributedTaskCoordinator::detachLeadershipCallbackOwnerAndWait() noexcept {
    try {
        std::unique_lock<std::mutex> lock(callback_state_->mutex);
        callback_state_->owner = nullptr;
        callback_state_->cv.wait(lock, [this] {
            return callback_state_->active_callbacks == 0;
        });
    } catch (const std::exception& ex) {
        THEMIS_ERROR(
            "DistributedTaskCoordinator: failed waiting for leadership callbacks: {}",
            ex.what());
    } catch (...) {
        THEMIS_ERROR(
            "DistributedTaskCoordinator: unknown error waiting for leadership callbacks");
    }
}

/**
 * @brief ── Lifecycle ────────────────────────────────────────────────────────────────
 * @details Calls: exchange(), THEMIS_WARN(), setLeaderElectedCallback(), maintainHeartbeat(), onLeaderElected(), isLeader(), THEMIS_INFO(), activateScheduler().
 */

void DistributedTaskCoordinator::start() {
    bool setup_failed = false;
    {
        std::unique_lock<std::mutex> lock(transition_mutex_);
        if (running_.exchange(true)) {
            THEMIS_WARN("DistributedTaskCoordinator already running");
            return;
        }

        try {
            const auto callback_state = callback_state_;
            coordinator_->setLeaderElectedCallback(
                [callback_state](const std::string& leader_id) {
                    DistributedTaskCoordinator::dispatchLeadershipCallback(
                        callback_state, leader_id);
                });

            if (!maintainHeartbeat(config_.heartbeat_check_interval)) {
                setup_failed = true;
                coordinator_->setLeaderElectedCallback(nullptr);
            }
        } catch (...) {
            setup_failed = true;
            try {
                coordinator_->setLeaderElectedCallback(nullptr);
            } catch (...) {
                // Preserve the startup failure; callback state is detached by
                // the destructor and the running flag prevents useful work.
            }
        }

        if (setup_failed) {
            execution_gate_->store(false, std::memory_order_release);
            running_.store(false);
            desired_execution_gate_open_ = false;
            desired_scheduler_active_ = false;
            ++transition_epoch_;
        }
    }

    if (setup_failed) {
        throw std::runtime_error(
            "DistributedTaskCoordinator: failed to start coordination health monitor");
    }

    // If we are already the leader (coordinator was started before us),
    // activate the scheduler immediately. Activation stays outside the setup
    // lock and performs its own epoch/leadership recheck.
    try {
        if (coordinator_->isLeader() && config_.auto_manage_scheduler) {
            THEMIS_INFO("DistributedTaskCoordinator: node is already leader at start – activating scheduler");
            activateScheduler();
        }
    } catch (...) {
        if (!running_.load()) {
            return;  // A concurrent stop won the lifecycle transition.
        }
        stop();
        throw;
    }

    THEMIS_INFO("DistributedTaskCoordinator started");
}

/**
 * @brief Stop.
 * @details Calls: exchange(), lock(), notify_all(), joinable(), join(), deactivateScheduler(), setLeaderElectedCallback(), THEMIS_INFO().
 */
void DistributedTaskCoordinator::stop() {
    bool run_transition = false;
    {
        std::lock_guard<std::mutex> lock(transition_mutex_);
        execution_gate_->store(false, std::memory_order_release);
        running_.store(false);
        desired_execution_gate_open_ = false;
        desired_scheduler_active_ = false;
        ++transition_epoch_;
        if (!transition_in_progress_) {
            transition_in_progress_ = true;
            run_transition = true;
        }
        try {
            // Serialize callback removal against startup installation. Copied
            // callbacks are separately quiesced by callback_state_.
            coordinator_->setLeaderElectedCallback(nullptr);
        } catch (const std::exception& ex) {
            THEMIS_WARN(
                "DistributedTaskCoordinator: failed to remove leadership callback: {}",
                ex.what());
        }
    }

    std::exception_ptr transition_error;
    if (run_transition) {
        try {
            runSchedulerTransitions();
        } catch (...) {
            transition_error = std::current_exception();
        }
    }

    // Stop and join the heartbeat monitor. Exactly one concurrent stop owns
    // the join via heartbeat_active_'s exchange.
    if (heartbeat_active_.exchange(false)) {
        {
            std::lock_guard<std::mutex> lock(heartbeat_mutex_);
            heartbeat_cv_.notify_all();
        }
        if (heartbeat_thread_ && heartbeat_thread_->joinable()) {
            heartbeat_thread_->join();
        }
    }

    THEMIS_INFO("DistributedTaskCoordinator stopped");
    if (transition_error) {
        std::rethrow_exception(transition_error);
    }
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
    bool run_transition = false;
    bool denied = false;
    {
        std::lock_guard<std::mutex> lock(transition_mutex_);
        denied = !hasHealthyLocalLeadershipLocked();
        desired_scheduler_active_ = !denied;
        desired_execution_gate_open_ = !denied;
        ++transition_epoch_;
        execution_gate_->store(false, std::memory_order_release);
        if (!transition_in_progress_) {
            transition_in_progress_ = true;
            run_transition = true;
        }
    }

    if (run_transition) {
        runSchedulerTransitions();
    }
    if (denied) {
        throw std::runtime_error(
            "DistributedTaskCoordinator: activation denied without healthy leadership");
    }
}

bool DistributedTaskCoordinator::hasHealthyLocalLeadershipLocked() const noexcept {
    try {
        if (!running_.load() || !coordinator_->isHealthy() || !coordinator_->isLeader()) {
            return false;
        }

        const std::string local_id = coordinator_->getLocalShardId();
        std::lock_guard<std::mutex> leadership_lock(leadership_mutex_);
        return current_leader_.empty() || current_leader_ == local_id;
    } catch (...) {
        return false;
    }
}

/**
 * @brief Deactivate Scheduler.
 * @details Calls: exchange(), THEMIS_INFO(), stop(), lock(), unregisterTask().
 */
void DistributedTaskCoordinator::deactivateScheduler() {
    bool run_transition = false;
    {
        std::lock_guard<std::mutex> lock(transition_mutex_);
        desired_scheduler_active_ = false;
        desired_execution_gate_open_ = false;
        ++transition_epoch_;
        execution_gate_->store(false, std::memory_order_release);
        if (!transition_in_progress_) {
            transition_in_progress_ = true;
            run_transition = true;
        }
    }
    if (run_transition) {
        runSchedulerTransitions();
    }
}

void DistributedTaskCoordinator::runSchedulerTransitions() {
    while (true) {
        bool activate = false;
        uint64_t epoch = 0;
        {
            std::unique_lock<std::mutex> lock(transition_mutex_);
            if (desired_scheduler_active_ && !hasHealthyLocalLeadershipLocked()) {
                desired_scheduler_active_ = false;
                desired_execution_gate_open_ = false;
                ++transition_epoch_;
                execution_gate_->store(false, std::memory_order_release);
            }

            activate = desired_scheduler_active_;
            const bool currently_active = scheduler_active_.load();
            if (activate == currently_active) {
                const bool can_open_gate =
                    desired_execution_gate_open_ && hasHealthyLocalLeadershipLocked();
                execution_gate_->store(can_open_gate, std::memory_order_release);
                transition_in_progress_ = false;
                lock.unlock();
                transition_cv_.notify_all();
                return;
            }

            epoch = transition_epoch_;
            execution_gate_->store(false, std::memory_order_release);
            // Publish the intended state before releasing the lock so task
            // mutations during start/stop follow the existing forwarding path.
            scheduler_active_.store(activate);
        }

        try {
            if (activate) {
                THEMIS_INFO("DistributedTaskCoordinator: activating local scheduler");
                std::optional<TaskScheduler::ScopedRequestContext> internal_context;
                if (!TaskScheduler::hasRequestContext()) {
                    internal_context.emplace(makeInternalSchedulerContext({"task:register"}));
                }

                size_t task_count = 0;
                {
                    std::lock_guard<std::mutex> registry_lock(registry_mutex_);
                    task_count = task_registry_.size();
                    for (const auto& [id, task] : task_registry_) {
                        try {
                            scheduler_->registerTask(task);
                        } catch (const std::exception& ex) {
                            THEMIS_WARN(
                                "DistributedTaskCoordinator: failed to register task {} "
                                "on activation: {}",
                                id, ex.what());
                        }
                    }
                }
                scheduler_->start();
                THEMIS_INFO(
                    "DistributedTaskCoordinator: local scheduler activated "
                    "({} tasks registered)",
                    task_count);
            } else {
                THEMIS_INFO("DistributedTaskCoordinator: deactivating local scheduler");
                scheduler_->stop();

                std::optional<TaskScheduler::ScopedRequestContext> internal_context;
                if (!TaskScheduler::hasRequestContext()) {
                    internal_context.emplace(makeInternalSchedulerContext({"task:unregister"}));
                }
                {
                    std::lock_guard<std::mutex> registry_lock(registry_mutex_);
                    for (const auto& [id, task] : task_registry_) {
                        try {
                            scheduler_->unregisterTask(id);
                        } catch (const std::exception&) {
                            // The task may not have been registered if activation failed.
                        }
                    }
                }
                THEMIS_INFO("DistributedTaskCoordinator: local scheduler deactivated");
            }
        } catch (...) {
            if (activate) {
                try {
                    scheduler_->stop();
                } catch (const std::exception& cleanup_ex) {
                    THEMIS_ERROR(
                        "DistributedTaskCoordinator: scheduler cleanup after activation "
                        "failure failed: {}",
                        cleanup_ex.what());
                } catch (...) {
                    THEMIS_ERROR(
                        "DistributedTaskCoordinator: scheduler cleanup after activation "
                        "failure failed with an unknown error");
                }
            }
            {
                std::lock_guard<std::mutex> lock(transition_mutex_);
                execution_gate_->store(false, std::memory_order_release);
                desired_execution_gate_open_ = false;
                desired_scheduler_active_ = false;
                scheduler_active_.store(false);
                ++transition_epoch_;
                transition_in_progress_ = false;
            }
            transition_cv_.notify_all();
            throw;
        }

        {
            std::unique_lock<std::mutex> lock(transition_mutex_);
            if (activate && epoch == transition_epoch_ &&
                desired_scheduler_active_ && desired_execution_gate_open_ &&
                hasHealthyLocalLeadershipLocked()) {
                // This is the final gate-open point. The transition epoch and
                // local healthy leadership are rechecked after scheduler start.
                execution_gate_->store(true, std::memory_order_release);
                transition_in_progress_ = false;
                lock.unlock();
                transition_cv_.notify_all();
                return;
            }
            if (activate && !hasHealthyLocalLeadershipLocked()) {
                desired_scheduler_active_ = false;
                desired_execution_gate_open_ = false;
                ++transition_epoch_;
            }
            execution_gate_->store(false, std::memory_order_release);
        }
        // A newer lifecycle request arrived while start/stop was in progress.
        // Reconcile it on this thread without holding a lock across the next
        // potentially blocking scheduler operation.
    }
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
    if (!TaskScheduler::hasPermission("task:register")) {
        throw std::runtime_error(
            "DistributedTaskCoordinator: task:register permission required");
    }
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
    if (!TaskScheduler::hasPermission("task:unregister")) {
        throw std::runtime_error(
            "DistributedTaskCoordinator: task:unregister permission required");
    }
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
    if (!TaskScheduler::hasPermission("task:update")) {
        throw std::runtime_error(
            "DistributedTaskCoordinator: task:update permission required");
    }
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
    if (!TaskScheduler::hasPermission("task:update")) {
        throw std::runtime_error(
            "DistributedTaskCoordinator: task:update permission required");
    }
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
    bool run_transition = false;
    bool rejected_leadership = false;
    const std::string my_id = coordinator_->getLocalShardId();
    const bool i_am_leader = (leader_id == my_id);
    {
        std::lock_guard<std::mutex> transition_lock(transition_mutex_);
        // Ignore late callbacks after stop, while still keeping the gate closed.
        if (!running_.load()) {
            execution_gate_->store(false, std::memory_order_release);
            desired_execution_gate_open_ = false;
            desired_scheduler_active_ = false;
            ++transition_epoch_;
            return;
        }

        {
            std::lock_guard<std::mutex> leadership_lock(leadership_mutex_);
            current_leader_ = leader_id;
        }
        execution_gate_->store(false, std::memory_order_release);
        desired_execution_gate_open_ = false;

        if (i_am_leader && !hasHealthyLocalLeadershipLocked()) {
            rejected_leadership = true;
            desired_scheduler_active_ = false;
        } else if (i_am_leader) {
            THEMIS_INFO(
                "DistributedTaskCoordinator: this node ({}) became leader",
                my_id);
            leadership_acquired_.fetch_add(1);
            desired_execution_gate_open_ = true;
            if (config_.auto_manage_scheduler) {
                desired_scheduler_active_ = true;
            }
        } else {
            desired_execution_gate_open_ = false;
            if (scheduler_active_.load()) {
                THEMIS_INFO(
                    "DistributedTaskCoordinator: leadership transferred to {}",
                    leader_id);
                leadership_lost_.fetch_add(1);
            }
            if (config_.auto_manage_scheduler) {
                desired_scheduler_active_ = false;
            }
        }

        ++transition_epoch_;
        if (!transition_in_progress_) {
            if (desired_scheduler_active_ != scheduler_active_.load()) {
                transition_in_progress_ = true;
                run_transition = true;
            } else {
                const bool can_open_gate =
                    desired_execution_gate_open_ && hasHealthyLocalLeadershipLocked();
                execution_gate_->store(can_open_gate, std::memory_order_release);
            }
        }
    }

    if (run_transition) {
        runSchedulerTransitions();
    }
    if (rejected_leadership) {
        THEMIS_ERROR(
            "DistributedTaskCoordinator: leadership rejected because coordination is unhealthy");
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
    std::lock_guard<std::mutex> lifecycle_lock(transition_mutex_);
    if (!running_.load()) {
        execution_gate_->store(false, std::memory_order_release);
        desired_execution_gate_open_ = false;
        desired_scheduler_active_ = false;
        ++transition_epoch_;
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
        if (coordinator_->isLeader() && coordinator_->isHealthy()) {
            acquired = true;
            THEMIS_DEBUG("DistributedTaskCoordinator: already leader");
        } else {
            // Try to participate in the leadership election with a timeout.
            // This is a simplified check; production systems may use more
            // sophisticated timing or heartbeat mechanisms.
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > timeout_ms) {
                execution_gate_->store(false, std::memory_order_release);
                desired_execution_gate_open_ = false;
                desired_scheduler_active_ = false;
                ++transition_epoch_;
                THEMIS_WARN("DistributedTaskCoordinator: leadership acquisition timed out after {}ms",
                           timeout_ms.count());
                coordination_failures_.fetch_add(1);
                return false;
            }
            // For now, if we're not already leader, acquisition fails.
            // In production, this would interact with the leader-election protocol.
            THEMIS_INFO("DistributedTaskCoordinator: not currently leader; election in progress");
            execution_gate_->store(false, std::memory_order_release);
            desired_execution_gate_open_ = false;
            desired_scheduler_active_ = false;
            ++transition_epoch_;
            acquired = false;
        }

        return acquired;
    } catch (const std::exception& ex) {
        execution_gate_->store(false, std::memory_order_release);
        desired_execution_gate_open_ = false;
        desired_scheduler_active_ = false;
        ++transition_epoch_;
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
    if (heartbeat_interval_ms <= std::chrono::milliseconds::zero()) {
        THEMIS_ERROR("DistributedTaskCoordinator: heartbeat interval must be positive");
        return false;
    }
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

            // Both a healthy cluster and local leadership are required. A
            // healthy remote leader must not leave this node's gate open.
            const bool local_leader = coordinator_->isLeader();
            const bool coordinator_healthy = coordinator_->isHealthy();
            if (!coordinator_healthy || !local_leader) {
                bool run_transition = false;
                bool leadership_lost = false;
                {
                    std::lock_guard<std::mutex> transition_lock(transition_mutex_);
                    // Recheck under transition serialization so an outdated
                    // health observation cannot close a newly healthy leader.
                    if (!hasHealthyLocalLeadershipLocked()) {
                        leadership_lost = true;
                        desired_execution_gate_open_ = false;
                        desired_scheduler_active_ = false;
                        ++transition_epoch_;
                        execution_gate_->store(false, std::memory_order_release);
                        if (!transition_in_progress_) {
                            transition_in_progress_ = true;
                            run_transition = true;
                        }
                    }
                }
                if (leadership_lost) {
                    THEMIS_WARN(
                        "DistributedTaskCoordinator: closing execution gate "
                        "(coordinator_healthy={}, local_leader={})",
                        coordinator_healthy, local_leader);
                    coordination_failures_.fetch_add(1);
                    if (run_transition) {
                        runSchedulerTransitions();
                    }
                    if (scheduler_active_.load()) {
                        THEMIS_WARN(
                            "DistributedTaskCoordinator: scheduler deactivation "
                            "requested after coordination or leadership loss");
                    }
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
            bool run_transition = false;
            {
                std::lock_guard<std::mutex> transition_lock(transition_mutex_);
                execution_gate_->store(false, std::memory_order_release);
                desired_execution_gate_open_ = false;
                desired_scheduler_active_ = false;
                ++transition_epoch_;
                if (!transition_in_progress_) {
                    transition_in_progress_ = true;
                    run_transition = true;
                }
            }
            if (run_transition) {
                try {
                    runSchedulerTransitions();
                } catch (const std::exception& deactivate_ex) {
                    THEMIS_ERROR(
                        "DistributedTaskCoordinator: scheduler deactivation after "
                        "heartbeat error also failed: {}",
                        deactivate_ex.what());
                }
            }
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
    try {
        // Check for split-brain conditions: multiple leaders, stale replicas, etc.
        const std::string my_id = coordinator_->getLocalShardId();
        const std::optional<std::string> current_leader = coordinator_->getCurrentLeader();
        bool split_brain_detected = false;
        bool run_transition = false;

        {
            std::lock_guard<std::mutex> transition_lock(transition_mutex_);
            if (!running_.load()) {
                execution_gate_->store(false, std::memory_order_release);
                return themis::scheduler::SchedulerError::kCoordinationError;
            }

            {
                std::lock_guard<std::mutex> leadership_lock(leadership_mutex_);
                if (!current_leader_.empty() && current_leader.has_value() &&
                    current_leader_ != current_leader.value()) {
                    THEMIS_WARN(
                        "[DistributedTaskCoordinator::handleSplitBrainDetection] "
                        "code={} msg='split-brain detected' "
                        "context={{node_id='{}', previous_leader='{}', current_leader='{}', "
                        "scheduler_active={}}}",
                        static_cast<int>(themis::scheduler::SchedulerError::kCoordinationError),
                        my_id, current_leader_, *current_leader, scheduler_active_.load());
                    split_brain_detected = true;
                } else if (current_leader) {
                    current_leader_ = *current_leader;
                }
            }

            if (split_brain_detected) {
                execution_gate_->store(false, std::memory_order_release);
                desired_execution_gate_open_ = false;
                desired_scheduler_active_ = false;
                ++transition_epoch_;
                if (!transition_in_progress_) {
                    transition_in_progress_ = true;
                    run_transition = true;
                }
            }
        }

        if (split_brain_detected) {
            if (run_transition) {
                runSchedulerTransitions();
            }
            coordination_failures_.fetch_add(1);
            return themis::scheduler::SchedulerError::kCoordinationError;
        }

        THEMIS_DEBUG("DistributedTaskCoordinator: split-brain check passed (leader={})",
                    current_leader ? *current_leader : "unknown");
        return themis::scheduler::SchedulerError::kSuccess;
    } catch (const std::exception& ex) {
        bool run_transition = false;
        {
            std::lock_guard<std::mutex> transition_lock(transition_mutex_);
            execution_gate_->store(false, std::memory_order_release);
            desired_execution_gate_open_ = false;
            desired_scheduler_active_ = false;
            ++transition_epoch_;
            if (!transition_in_progress_) {
                transition_in_progress_ = true;
                run_transition = true;
            }
        }
        if (run_transition) {
            try {
                runSchedulerTransitions();
            } catch (const std::exception& deactivate_ex) {
                THEMIS_ERROR(
                    "DistributedTaskCoordinator: scheduler deactivation after "
                    "split-brain check error also failed: {}",
                    deactivate_ex.what());
            }
        }
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
