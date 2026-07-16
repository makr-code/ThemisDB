/**
 * @file distributed_task_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: distributed_task_coordinator.cpp | Version: 0.0.18 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 344
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0
 * PR History (last 5): #3364 [scheduler] Distributed cro... (2026-03-12) | #2568 [scheduler] Distributed tas... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
{
    const Config default_config{};
    if (!scheduler) {
        throw std::invalid_argument("DistributedTaskCoordinator: scheduler cannot be null");
    }
    if (!coordinator) {
        throw std::invalid_argument("DistributedTaskCoordinator: coordinator cannot be null");
    }
    scheduler_ = scheduler;
    coordinator_ = coordinator;
    config_ = default_config;
}

DistributedTaskCoordinator::DistributedTaskCoordinator(
    TaskScheduler* scheduler,
    sharding::DistributedCoordinator* coordinator,
    const Config& config)
    : scheduler_(scheduler),
      coordinator_(coordinator),
      config_(config)
{
    if (!scheduler_) {
        throw std::invalid_argument("DistributedTaskCoordinator: scheduler cannot be null");
    }
    if (!coordinator_) {
        throw std::invalid_argument("DistributedTaskCoordinator: coordinator cannot be null");
    }

    THEMIS_INFO("DistributedTaskCoordinator created for node: {}",
                coordinator_->getLocalShardId());
}

DistributedTaskCoordinator::~DistributedTaskCoordinator() {
    stop();
}

// ── Lifecycle ────────────────────────────────────────────────────────────────

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

void DistributedTaskCoordinator::stop() {
    if (!running_.exchange(false)) {
        return;  // Already stopped.
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

// ── Manual scheduler control ──────────────────────────────────────────────────

void DistributedTaskCoordinator::activateScheduler() {
    if (scheduler_active_.exchange(true)) {
        return;  // Already active.
    }

    THEMIS_INFO("DistributedTaskCoordinator: activating local scheduler");

    // Register all locally stored tasks with the scheduler.
    size_t task_count = 0;
    {
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
            } catch ([[maybe_unused]] const std::exception& ex) {
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

// ── Task management ───────────────────────────────────────────────────────────

std::string DistributedTaskCoordinator::registerTask(const ScheduledTask& task) {
    // Assign an ID if one was not provided (mirrors TaskScheduler behaviour).
    ScheduledTask stored = task;
    if (stored.id.empty()) {
        stored.id = generateId(task);
    }

    const std::string task_id = stored.id;

    {
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
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<ScheduledTask> result;
    result.reserve(task_registry_.size());
    for (const auto& [id, task] : task_registry_) {
        result.push_back(task);
    }
    return result;
}

std::shared_ptr<ScheduledTask> DistributedTaskCoordinator::getTask(
    const std::string& task_id) const
{
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = task_registry_.find(task_id);
    if (it == task_registry_.end()) {
        return nullptr;
    }
    return std::make_shared<ScheduledTask>(it->second);
}

// ── Statistics ────────────────────────────────────────────────────────────────

DistributedTaskCoordinator::Stats DistributedTaskCoordinator::getStats() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    Stats s;
    s.registered_tasks    = task_registry_.size();
    s.scheduler_active    = scheduler_active_.load();
    s.is_leader           = coordinator_->isLeader();
    s.leadership_acquired = leadership_acquired_.load();
    s.leadership_lost     = leadership_lost_.load();
    return s;
}

// ── Private helpers ───────────────────────────────────────────────────────────

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
        std::lock_guard<std::mutex> lock(leadership_mutex_);
        current_leader_ = leader_id;
    }

    if (i_am_leader) {
        THEMIS_INFO("DistributedTaskCoordinator: this node ({}) became leader – activating scheduler",
                    my_id);
        leadership_acquired_.fetch_add(1);
        if (config_.auto_manage_scheduler) {
            activateScheduler();
        }
    } else {
        if (scheduler_active_.load()) {
            THEMIS_INFO("DistributedTaskCoordinator: leadership transferred to {} – deactivating scheduler",
                        leader_id);
            leadership_lost_.fetch_add(1);
            if (config_.auto_manage_scheduler) {
                deactivateScheduler();
            }
        }
    }
}

/*static*/ std::string DistributedTaskCoordinator::generateId(const ScheduledTask& task) {
    // Mirror the simple ID generation from TaskScheduler::generateTaskId.
    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()).count();
    std::ostringstream oss;
    oss << "task_" << std::hex << ms;
    if (!task.name.empty()) {
        oss << "_";
        for (char c : task.name) {
            oss << (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
        }
    }
    return oss.str();
}

} // namespace themis
