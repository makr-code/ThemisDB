/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            update_state_machine.cpp                           ║
  Version:         0.0.36                                             ║
  Last Modified:   2026-04-13 04:32:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     496                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fa516498e3  2026-04-12  [WIP] Update module documentation and inventory (#4521) ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 02c0a65e1e  2026-02-23  audit: fix stale Stubs:1 banners, add Phase 10 smoke test... ║
    • 8f53829d27  2026-02-22  Finalize canary rollout: move to Completed in ROADMAP, cl... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "updates/update_state_machine.h"
#include "updates/update_history_logger.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace themis {
namespace updates {

// ============================================================================
// UpdateTransactionEntry serialisation
// ============================================================================

static std::string stateToString(UpdateState s) {
    switch (s) {
        case UpdateState::IDLE:         return "idle";
        case UpdateState::DOWNLOADING:  return "downloading";
        case UpdateState::VERIFYING:    return "verifying";
        case UpdateState::APPLYING:     return "applying";
        case UpdateState::ROLLING_BACK: return "rolling_back";
        case UpdateState::FAILED:       return "failed";
    }
    return "unknown";
}

static UpdateState stateFromString(const std::string& s) {
    if (s == "idle")         return UpdateState::IDLE;
    if (s == "downloading")  return UpdateState::DOWNLOADING;
    if (s == "verifying")    return UpdateState::VERIFYING;
    if (s == "applying")     return UpdateState::APPLYING;
    if (s == "rolling_back") return UpdateState::ROLLING_BACK;
    if (s == "failed")       return UpdateState::FAILED;
    return UpdateState::IDLE;
}

json UpdateTransactionEntry::toJson() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm_val = {};
#ifdef _WIN32
    gmtime_s(&tm_val, &time_t_val);
#else
    gmtime_r(&time_t_val, &tm_val);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_val);

    json j;
    j["from_state"] = stateToString(from_state);
    j["to_state"]   = stateToString(to_state);
    j["version"]    = version;
    j["message"]    = message;
    j["timestamp"]  = buf;
    return j;
}

std::optional<UpdateTransactionEntry> UpdateTransactionEntry::fromJson(const json& j) {
    try {
        UpdateTransactionEntry e;
        e.from_state = stateFromString(j.value("from_state", "idle"));
        e.to_state   = stateFromString(j.value("to_state",   "idle"));
        e.version    = j.value("version", "");
        e.message    = j.value("message", "");

        std::string ts = j.value("timestamp", "");
        if (!ts.empty()) {
            std::tm tm_val = {};
            std::istringstream ss(ts);
            ss >> std::get_time(&tm_val, "%Y-%m-%dT%H:%M:%SZ");
            if (!ss.fail()) {
#ifdef _WIN32
                auto time_t_val = _mkgmtime(&tm_val);
#else
                auto time_t_val = timegm(&tm_val);
#endif
                e.timestamp = std::chrono::system_clock::from_time_t(time_t_val);
            }
        }
        return e;
    } catch (...) {
        return std::nullopt;
    }
}

// ============================================================================
// UpdateStateMachine
// ============================================================================

UpdateStateMachine::UpdateStateMachine(const std::string& log_path)
    : log_path_(log_path) {
    if (!log_path_.empty()) {
        loadPersistedState();
    }
}

UpdateState UpdateStateMachine::currentState() const {
    return state_.load(std::memory_order_acquire);
}

std::string UpdateStateMachine::currentVersion() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_version_;
}

bool UpdateStateMachine::isValidTransition(UpdateState from, UpdateState to) const {
    switch (from) {
        case UpdateState::IDLE:
            return to == UpdateState::DOWNLOADING;

        case UpdateState::DOWNLOADING:
            return to == UpdateState::VERIFYING
                || to == UpdateState::ROLLING_BACK
                || to == UpdateState::FAILED;

        case UpdateState::VERIFYING:
            return to == UpdateState::APPLYING
                || to == UpdateState::ROLLING_BACK
                || to == UpdateState::FAILED;

        case UpdateState::APPLYING:
            return to == UpdateState::IDLE       // success
                || to == UpdateState::ROLLING_BACK
                || to == UpdateState::FAILED;

        case UpdateState::ROLLING_BACK:
            return to == UpdateState::IDLE       // rollback complete
                || to == UpdateState::FAILED;

        case UpdateState::FAILED:
            return false;  // must call reset()
    }
    return false;
}

bool UpdateStateMachine::transition(UpdateState to,
                                    const std::string& version,
                                    const std::string& message) {
    // Collect callbacks and state under the lock, then invoke them outside it.
    // This prevents a deadlock when a callback calls currentState() or
    // currentVersion() (both of which also acquire mutex_).
    std::vector<StateChangeCallback> callbacks_copy;
    UpdateState from;
    std::string notify_version;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        from = state_.load(std::memory_order_relaxed);

        if (!isValidTransition(from, to)) {
            LOG_WARN("UpdateStateMachine: invalid transition {} -> {} (version={})",
                     stateToString(from), stateToString(to), version);
            return false;
        }

        // Update version for new downloads; keep existing version for subsequent transitions
        if (to == UpdateState::DOWNLOADING && !version.empty()) {
            current_version_ = version;
        } else if (!version.empty()) {
            current_version_ = version;
        }
        // else: keep current_version_

        state_.store(to, std::memory_order_release);

        UpdateTransactionEntry entry{from, to, current_version_, message,
                                     std::chrono::system_clock::now()};
        transaction_log_.push_back(entry);

        if (!log_path_.empty()) {
            appendLogEntry(entry);
        }

        LOG_INFO("UpdateStateMachine: {} -> {} (version={}, msg={})",
                 stateToString(from), stateToString(to), current_version_, message);

        // Clear in-flight flag and version only when reaching IDLE
        if (to == UpdateState::IDLE) {
            has_inflight_update_ = false;
            current_version_.clear();
        }

        notify_version   = current_version_;
        callbacks_copy   = callbacks_;  // shallow copy of function wrappers
    }  // lock released here

    for (auto& cb : callbacks_copy) {
        try {
            cb(from, to, notify_version);
        } catch (...) {
            // Never let callbacks crash the state machine
        }
    }

    return true;
}

void UpdateStateMachine::reset() {
    std::vector<StateChangeCallback> callbacks_copy;
    UpdateState from;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        from = state_.load(std::memory_order_relaxed);
        if (from != UpdateState::FAILED) {
            LOG_WARN("UpdateStateMachine::reset() called while not in FAILED state");
        }

        state_.store(UpdateState::IDLE, std::memory_order_release);
        current_version_.clear();
        has_inflight_update_ = false;

        UpdateTransactionEntry entry{from, UpdateState::IDLE, "",
                                     "manual reset", std::chrono::system_clock::now()};
        transaction_log_.push_back(entry);
        if (!log_path_.empty()) {
            appendLogEntry(entry);
        }

        LOG_INFO("UpdateStateMachine: reset from {} to IDLE", stateToString(from));

        callbacks_copy = callbacks_;
    }  // lock released here

    for (auto& cb : callbacks_copy) {
        try {
            cb(from, UpdateState::IDLE, "");
        } catch (...) {
            // Never let callbacks crash
        }
    }
}

void UpdateStateMachine::addStateChangeCallback(StateChangeCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(std::move(cb));
}

bool UpdateStateMachine::hasInFlightUpdate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return has_inflight_update_;
}

std::string UpdateStateMachine::inFlightVersion() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return inflight_version_;
}

std::vector<UpdateTransactionEntry> UpdateStateMachine::transactionLog() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto log = transaction_log_;
    std::reverse(log.begin(), log.end());  // newest first
    return log;
}

void UpdateStateMachine::persistState(const std::string& version,
                                      const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Record a checkpoint entry (from == to == current state) to note progress
    // within a state without a state transition.
    UpdateState cur = state_.load(std::memory_order_relaxed);
    UpdateTransactionEntry entry{
        cur, cur,
        version, message,
        std::chrono::system_clock::now()
    };
    appendLogEntry(entry);
}

void UpdateStateMachine::loadPersistedState() {
    try {
        std::ifstream f(log_path_);
        if (!f) {
            return;  // No log yet – clean start
        }

        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            try {
                auto j = json::parse(line);
                auto entry = UpdateTransactionEntry::fromJson(j);
                if (entry) {
                    transaction_log_.push_back(*entry);
                }
            } catch (...) {
                // Skip malformed lines
            }
        }

        if (transaction_log_.empty()) {
            return;
        }

        // The last entry shows where the process was when it last wrote
        const auto& last = transaction_log_.back();
        UpdateState persisted = last.to_state;

        // If the last persisted state is not IDLE, an update was in-flight
        if (persisted != UpdateState::IDLE && persisted != UpdateState::FAILED) {
            has_inflight_update_ = true;
            inflight_version_    = last.version;
            LOG_WARN("UpdateStateMachine: detected in-flight update for version {} in state {}",
                     inflight_version_, stateToString(persisted));
        }
    } catch (const std::exception& e) {
        LOG_ERROR("UpdateStateMachine: failed to load persisted state: {}", e.what());
    }
}

void UpdateStateMachine::appendLogEntry(const UpdateTransactionEntry& entry) {
    try {
        std::ofstream f(log_path_, std::ios::app);
        if (f) {
            f << entry.toJson().dump() << "\n";
        }
    } catch (const std::exception& e) {
        LOG_ERROR("UpdateStateMachine: failed to append log entry: {}", e.what());
    }
}

/*static*/
std::string UpdateStateMachine::stateName(UpdateState s) {
    return stateToString(s);
}

// ============================================================================
// Rollback checkpoint API
// ============================================================================

void UpdateStateMachine::setHistoryLogger(UpdateHistoryLogger* logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    history_logger_ = logger;
}

CheckpointId UpdateStateMachine::createCheckpoint(const std::string& description) {
    CheckpointId assigned_id = 0;
    std::string  snap_version;
    UpdateState  snap_state;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        snap_state   = state_.load(std::memory_order_relaxed);
        snap_version = current_version_;

        Checkpoint cp;
        cp.id          = next_checkpoint_id_++;
        cp.state       = snap_state;
        cp.version     = snap_version;
        cp.description = description;
        cp.timestamp   = std::chrono::system_clock::now();

        checkpoints_.push_back(cp);
        assigned_id = cp.id;

        LOG_INFO("UpdateStateMachine: checkpoint {} created (state={}, version={}, desc={})",
                 assigned_id, stateToString(snap_state), snap_version, description);
    }

    // Audit trail – record outside the lock to avoid holding it during I/O
    if (history_logger_) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        UpdateHistoryEntry e;
        e.who           = "state_machine";
        e.timestamp_ms  = now_ms;
        e.from_version  = snap_version;
        e.to_version    = snap_version;
        e.event_type    = "checkpoint_created";
        e.success       = true;
        e.error_message = description;
        history_logger_->record(e);
    }

    return assigned_id;
}

bool UpdateStateMachine::rollbackToCheckpoint(CheckpointId id) {
    std::vector<StateChangeCallback> callbacks_copy;
    UpdateState from_state;
    UpdateState to_state;
    std::string notify_version;
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = std::find_if(checkpoints_.begin(), checkpoints_.end(),
                               [id](const Checkpoint& cp) { return cp.id == id; });
        if (it == checkpoints_.end()) {
            LOG_WARN("UpdateStateMachine: rollbackToCheckpoint({}) – checkpoint not found", id);
            return false;
        }

        from_state = state_.load(std::memory_order_relaxed);
        to_state   = it->state;

        current_version_ = it->version;
        state_.store(to_state, std::memory_order_release);

        // Discard checkpoints newer than the target
        checkpoints_.erase(it + 1, checkpoints_.end());

        // Record the restoration in the transaction log
        UpdateTransactionEntry entry{
            from_state, to_state,
            current_version_,
            "rollback to checkpoint " + std::to_string(id),
            std::chrono::system_clock::now()
        };
        transaction_log_.push_back(entry);
        if (!log_path_.empty()) {
            appendLogEntry(entry);
        }

        LOG_INFO("UpdateStateMachine: rolled back to checkpoint {} (state={}, version={})",
                 id, stateToString(to_state), current_version_);

        notify_version = current_version_;
        callbacks_copy = callbacks_;
        found = true;
    }

    // Notify callbacks outside the lock
    for (auto& cb : callbacks_copy) {
        try {
            cb(from_state, to_state, notify_version);
        } catch (...) {}
    }

    // Audit trail
    if (found && history_logger_) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        UpdateHistoryEntry e;
        e.who           = "state_machine";
        e.timestamp_ms  = now_ms;
        e.from_version  = "";  // pre-rollback version already overwritten
        e.to_version    = notify_version;
        e.event_type    = "checkpoint_rollback";
        e.success       = true;
        e.error_message = "checkpoint_id=" + std::to_string(id);
        history_logger_->record(e);
    }

    return found;
}

std::vector<Checkpoint> UpdateStateMachine::listCheckpoints() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return checkpoints_;
}

void UpdateStateMachine::clearCheckpoints() {
    std::lock_guard<std::mutex> lock(mutex_);
    checkpoints_.clear();
    LOG_INFO("UpdateStateMachine: all checkpoints cleared");
}

} // namespace updates
} // namespace themis
