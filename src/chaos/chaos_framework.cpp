/**
 * @file chaos_framework.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB — Chaos Engineering Framework
 *
 * File:    src/chaos/chaos_framework.cpp
 * Module:  chaos
 * Phase:   4.3 — Chaos Testing Framework
 *
 * In-process fault registry and time-driven chaos scheduler.
 * Provides fault injection primitives for HA/failover integration tests.
 */

#include "chaos/chaos_framework.h"

#include <algorithm>
#include <stdexcept>

namespace themis::chaos {

// ─── Helpers ─────────────────────────────────────────────────────────────────

std::string FaultInjector::faultTypeName(FaultType type) noexcept {
    switch (type) {
        case FaultType::NODE_FAILURE:
            return "NODE_FAILURE";
        case FaultType::NETWORK_PARTITION:
            return "NETWORK_PARTITION";
        case FaultType::LEADER_CRASH:
            return "LEADER_CRASH";
        case FaultType::DELAYED_RESPONSE:
            return "DELAYED_RESPONSE";
        case FaultType::DISK_FAILURE:
            return "DISK_FAILURE";
        case FaultType::RANDOM_FAILURE:
            return "RANDOM_FAILURE";
        case FaultType::DISASTER_RECOVERY_DRILL:
            return "DR_DRILL";
    }
    return "UNKNOWN";
}

std::string FaultInjector::makeKey(const std::string &node_id, FaultType type) {
    return node_id + "::" + faultTypeName(type);
}

// ─── FaultInjector ───────────────────────────────────────────────────────────

FaultInjector::FaultInjector(std::string injector_id) : injector_id_(std::move(injector_id)) {}

FaultInjector::~FaultInjector() {
    clearAllFaults();
}

bool FaultInjector::injectFault(const FaultSpec &fault) {
    if (fault.target_node_id.empty()) {
        return false;
    }
    if (fault.probability < 0.0 || fault.probability > 1.0) {
        return false;
    }

    const std::string key = makeKey(fault.target_node_id, fault.type);

    ActiveFault af;
    af.spec        = fault;
    af.injected_at = std::chrono::steady_clock::now();

    if (fault.duration.count() > 0) {
        af.expires_at = af.injected_at + fault.duration;
    } else {
        af.expires_at = std::chrono::steady_clock::time_point::max();
    }

    {
        std::lock_guard<std::mutex> lock(fault_mutex_);
        auto [it, inserted] = active_faults_.emplace(key, af);
        if (!inserted) {
            // Already active for this node+type — update
            it->second = af;
        }
    }

    for (auto &cb : callbacks_) {
        cb(fault, true);
    }
    return true;
}

bool FaultInjector::recoverFault(const std::string &target_node_id) {
    std::lock_guard<std::mutex> lock(fault_mutex_);
    bool any = false;
    auto it  = active_faults_.begin();
    while (it != active_faults_.end()) {
        if (it->second.spec.target_node_id == target_node_id) {
            for (auto &cb : callbacks_) {
                cb(it->second.spec, false);
            }
            it  = active_faults_.erase(it);
            any = true;
        } else {
            ++it;
        }
    }
    return any;
}

bool FaultInjector::recoverFault(const std::string &target_node_id, FaultType type) {
    const std::string key = makeKey(target_node_id, type);
    std::lock_guard<std::mutex> lock(fault_mutex_);
    auto it = active_faults_.find(key);
    if (it == active_faults_.end()) {
        return false;
    }
    for (auto &cb : callbacks_) {
        cb(it->second.spec, false);
    }
    active_faults_.erase(it);
    return true;
}

bool FaultInjector::isFaultActive(const std::string &target_node_id) const {
    std::lock_guard<std::mutex> lock(fault_mutex_);
    for (const auto &[key, af] : active_faults_) {
        if (af.spec.target_node_id == target_node_id && !af.isExpired()) {
            return true;
        }
    }
    return false;
}

bool FaultInjector::isFaultActive(const std::string &target_node_id, FaultType type) const {
    const std::string key = makeKey(target_node_id, type);
    std::lock_guard<std::mutex> lock(fault_mutex_);
    auto it = active_faults_.find(key);
    if (it == active_faults_.end()) {
        return false;
    }
    return !it->second.isExpired();
}

std::vector<ActiveFault> FaultInjector::getActiveFaults() {
    pruneExpired();
    std::lock_guard<std::mutex> lock(fault_mutex_);
    std::vector<ActiveFault> result = {};

    result.reserve(active_faults_.size());
    for (const auto &[_, af] : active_faults_) {
        result.push_back(af);
    }
    return result;
}

size_t FaultInjector::activeFaultCount() {
    pruneExpired();
    std::lock_guard<std::mutex> lock(fault_mutex_);
    return static_cast<int>(active_faults_.size());
}

void FaultInjector::clearAllFaults() {
    std::lock_guard<std::mutex> lock(fault_mutex_);
    active_faults_.clear();
}

void FaultInjector::registerEventCallback(EventCallback cb) {
    callbacks_.push_back(std::move(cb));
}

void FaultInjector::pruneExpired() {
    std::lock_guard<std::mutex> lock(fault_mutex_);
    auto it = active_faults_.begin();
    while (it != active_faults_.end()) {
        if (it->second.isExpired()) {
            it = active_faults_.erase(it);
        } else {
            ++it;
        }
    }
}

// ─── ChaosScheduler ──────────────────────────────────────────────────────────

ChaosScheduler::ChaosScheduler(std::shared_ptr<FaultInjector> injector, Config cfg)
    : injector_(std::move(injector)), cfg_(cfg) {
    if (!injector_) {
        throw std::invalid_argument("ChaosScheduler: injector must not be null");
    }
}

ChaosScheduler::~ChaosScheduler() {
    stop();
}

void ChaosScheduler::schedule(ChaosScheduleEntry entry) {
    {
        std::lock_guard<std::mutex> lock(sched_mutex_);
        pending_.push_back(std::move(entry));
    }
    sched_cv_.notify_one();
}

void ChaosScheduler::scheduleIn(std::chrono::milliseconds delay, const FaultSpec &fault) {
    schedule({std::chrono::steady_clock::now() + delay, fault});
}

void ChaosScheduler::start() {
    if (running_.exchange(true)) {
        return; // already running
    }
    worker_ = std::thread(&ChaosScheduler::runLoop, this);
}

void ChaosScheduler::stop() {
    running_.store(false);
    sched_cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool ChaosScheduler::isRunning() const noexcept {
    return running_.load();
}

size_t ChaosScheduler::pendingCount() const {
    std::lock_guard<std::mutex> lock(sched_mutex_);
    return static_cast<int>(pending_.size());
}

void ChaosScheduler::clearPending() {
    std::lock_guard<std::mutex> lock(sched_mutex_);
    pending_.clear();
}

void ChaosScheduler::runLoop() {
    while (running_.load()) {
        // ── Fire phase: collect and inject all due faults ──────────────────
        const auto now = std::chrono::steady_clock::now();
        std::vector<FaultSpec> to_fire;
        {
            std::lock_guard<std::mutex> lock(sched_mutex_);
            auto new_end = std::remove_if(pending_.begin(), pending_.end(), [&](const ChaosScheduleEntry &e) {
                if (e.trigger_at <= now) {
                    to_fire.push_back(e.fault);
                    return true;
                }
                return false;
            });
            pending_.erase(new_end, pending_.end());
        }

        for (const auto &fault : to_fire) {
            injector_->injectFault(fault);
        }

        // ── Wait phase: sleep until next tick or until woken ───────────────
        if (cfg_.wake_strategy == WakeStrategy::CONDVAR) {
            std::unique_lock<std::mutex> lock(wake_mutex_);
            sched_cv_.wait_for(lock, cfg_.tick_interval, [this] { return !running_.load(); });
        } else {
            std::this_thread::sleep_for(cfg_.tick_interval);
        }
    }
}

} // namespace themis::chaos
