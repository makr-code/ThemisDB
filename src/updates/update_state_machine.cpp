/**
 * @file update_state_machine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.45
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/update_state_machine.h"
#include <stdexcept>
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
// Checkpoint serialisation
// ============================================================================

json Checkpoint::toJson() const {
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
    j["id"]          = id;
    j["state"]       = stateToString(state);
    j["version"]     = version;
    j["description"] = description;
    j["timestamp"]   = buf;
    return j;
}

std::optional<Checkpoint> Checkpoint::fromJson(const json& j) {
    try {
        Checkpoint cp;
        cp.id          = j.value("id", 0);
        cp.state       = stateFromString(j.value("state", "idle"));
        cp.version     = j.value("version", "");
        cp.description = j.value("description", "");

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
                cp.timestamp = std::chrono::system_clock::from_time_t(time_t_val);
            }
        }
        return cp;
    } catch (...) {
        return std::nullopt;
    }
}

// ============================================================================
// UpdateStateMachine
// ============================================================================

UpdateStateMachine::UpdateStateMachine(const std::string& log_path,
                                       const std::string& checkpoints_log_path)
    : log_path_(log_path), checkpoints_log_path_(checkpoints_log_path) {
    if (!log_path_.empty()) {
        loadPersistedState();
    }
    if (!checkpoints_log_path_.empty()) {
        loadCheckpoints();
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
            // Error Code: 7490 - Never let callbacks crash the state machine
            // Log and silently ignore to ensure state integrity is maintained
            LOG_WARN("UpdateStateMachine: state change callback threw exception; silently caught");
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

void UpdateStateMachine::persistCheckpoint(const Checkpoint& cp) {
    // Error Code: 7401 - Checkpoint file write failed
    try {
        std::ofstream f(checkpoints_log_path_, std::ios::app);
        if (f) {
            f << cp.toJson().dump() << "\n";
            LOG_DEBUG("UpdateStateMachine: persisted checkpoint {} to {}", cp.id, checkpoints_log_path_);
        } else {
            LOG_ERROR("UpdateStateMachine: failed to open checkpoints log file for writing: {}", checkpoints_log_path_);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("UpdateStateMachine: failed to persist checkpoint: {} (Error Code: 7401)", e.what());
    }
}

void UpdateStateMachine::loadCheckpoints() {
    // Error Code: 7402 - Checkpoint file read failed
    try {
        std::ifstream f(checkpoints_log_path_);
        if (!f) {
            return;  // No checkpoints log yet – clean start
        }

        std::string line;
        uint64_t max_id = 0;
        
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            try {
                auto j = json::parse(line);
                auto cp = Checkpoint::fromJson(j);
                if (cp) {
                    checkpoints_.push_back(*cp);
                    max_id = std::max(max_id, cp->id);
                    LOG_DEBUG("UpdateStateMachine: loaded checkpoint {} from persistent log", cp->id);
                }
            } catch (const std::exception& e) {
                LOG_WARN("UpdateStateMachine: skipping malformed checkpoint line: {}", e.what());
            }
        }

        // Set next checkpoint ID to be one more than the highest loaded ID
        if (max_id > 0) {
            next_checkpoint_id_ = max_id + 1;
        }
        
        if (!checkpoints_.empty()) {
            LOG_INFO("UpdateStateMachine: loaded {} checkpoints from {}", 
                     checkpoints_.size(), checkpoints_log_path_);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("UpdateStateMachine: failed to load checkpoints: {} (Error Code: 7402)", e.what());
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
    Checkpoint   cp_to_persist;

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
        cp_to_persist  = cp;  // Copy for persistence outside the lock
        assigned_id = cp.id;

        LOG_INFO("UpdateStateMachine: checkpoint {} created (state={}, version={}, desc={})",
                 assigned_id, stateToString(snap_state), snap_version, description);
    }

    // Persist checkpoint outside the lock to avoid holding it during I/O
    // Error Code: 7401 - Checkpoint file write failed
    if (!checkpoints_log_path_.empty()) {
        persistCheckpoint(cp_to_persist);
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

        // Check if this is an idempotent call (same checkpoint as last rollback)
        if (last_rollback_id_.has_value() && last_rollback_id_.value() == id) {
            // This is an idempotent rollback call – already rolled back to this checkpoint
            // Increment counter only for found/idempotent checkpoints, not for not-found ones.
            rollback_attempt_count_++;
            last_rollback_was_idempotent_ = true;
            last_rollback_time_ = std::chrono::system_clock::now();

            LOG_WARN("UpdateStateMachine: idempotent rollback to checkpoint {} (attempt #{})",
                     id, rollback_attempt_count_);

            // Record the idempotent attempt in the transaction log
            UpdateTransactionEntry entry{
                state_.load(std::memory_order_relaxed), 
                state_.load(std::memory_order_relaxed),
                current_version_,
                "rollback_idempotent checkpoint " + std::to_string(id),
                std::chrono::system_clock::now()
            };
            transaction_log_.push_back(entry);
            if (!log_path_.empty()) {
                appendLogEntry(entry);
            }

            // Audit trail for idempotent call
            if (history_logger_) {
                auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                UpdateHistoryEntry e;
                e.who           = "state_machine";
                e.timestamp_ms  = now_ms;
                e.from_version  = current_version_;
                e.to_version    = current_version_;
                e.event_type    = "rollback_idempotent";
                e.success       = true;
                e.error_message = "checkpoint_id=" + std::to_string(id);
                history_logger_->record(e);
            }

            return true;  // Idempotent calls are considered successful
        }

        // Find the target checkpoint
        auto it = std::find_if(checkpoints_.begin(), checkpoints_.end(),
                               [id](const Checkpoint& cp) { return cp.id == id; });
        if (it == checkpoints_.end()) {
            LOG_WARN("UpdateStateMachine: rollbackToCheckpoint({}) – checkpoint not found", id);
            return false;
        }

        // Checkpoint exists – count this as a rollback attempt
        rollback_attempt_count_++;

        from_state = state_.load(std::memory_order_relaxed);
        to_state   = it->state;

        // Perform the actual rollback (state change)
        current_version_ = it->version;
        state_.store(to_state, std::memory_order_release);

        // Update rollback tracking
        last_rollback_id_ = id;
        last_rollback_time_ = std::chrono::system_clock::now();
        last_rollback_was_idempotent_ = false;

        // Discard checkpoints newer than the target
        checkpoints_.erase(it + 1, checkpoints_.end());

        // Record the restoration in the transaction log
        UpdateTransactionEntry entry{
            from_state, to_state,
            current_version_,
            "rollback_attempt checkpoint " + std::to_string(id),
            std::chrono::system_clock::now()
        };
        transaction_log_.push_back(entry);
        if (!log_path_.empty()) {
            appendLogEntry(entry);
        }

        LOG_INFO("UpdateStateMachine: rolled back to checkpoint {} (state={}, version={}, attempt #{})",
                 id, stateToString(to_state), current_version_, rollback_attempt_count_);

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
        e.event_type    = "rollback_attempt";
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

// ============================================================================
// Partial and coordinated rollback enhancements (v1.8.1 – Q3 2026)
// ============================================================================

void UpdateStateMachine::setRollbackCallback(RollbackCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    rollback_callback_ = std::move(callback);
}

bool UpdateStateMachine::rollbackToLatestCheckpoint() {
    CheckpointId latest_id;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (checkpoints_.empty()) {
            LOG_ERROR("UpdateStateMachine: no checkpoints available for rollback");
            return false;
        }
        latest_id = checkpoints_.back().id;
    }
    return rollbackToCheckpoint(latest_id);
}

bool UpdateStateMachine::rollbackToCheckpointWithFallback(
    CheckpointId checkpoint_id,
    RollbackFallbackStrategy fallback_strategy) {
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        current_fallback_strategy_ = fallback_strategy;
    }
    
    // Attempt rollback without holding the lock (rollbackToCheckpoint takes it internally)
    bool success = rollbackToCheckpoint(checkpoint_id);
    
    if (!success) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fallback_strategy == RollbackFallbackStrategy::DEFER) {
            // Queue for later retry
            deferred_rollbacks_.push_back(checkpoint_id);
            LOG_WARN("UpdateStateMachine: deferring rollback for checkpoint {}",
                     checkpoint_id);
            return false;
        } else if (fallback_strategy == RollbackFallbackStrategy::PARTIAL_CONTINUE) {
            // Log but allow continuation
            LOG_WARN("UpdateStateMachine: continuing despite rollback failure for checkpoint {}", 
                     checkpoint_id);
            return true;  // Optimistic: allow caller to continue
        } else {
            // IMMEDIATE_ABORT - already handled
            LOG_ERROR("UpdateStateMachine: rollback aborted for checkpoint {}", checkpoint_id);
            return false;
        }
    }
    
    return true;
}

size_t UpdateStateMachine::checkpointCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return checkpoints_.size();
}

bool UpdateStateMachine::hasPendingRollback() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !deferred_rollbacks_.empty();
}

void UpdateStateMachine::emitRollbackDiagnostic(CheckpointId checkpoint_id,
                                               bool success,
                                               const std::string& reason) {
    if (success) {
        LOG_WARN("UpdateStateMachine: rollback_checkpoint[id={}, success=true]",
                checkpoint_id);
    } else {
        LOG_ERROR("UpdateStateMachine: rollback_checkpoint[id={}, success=false, reason={}]",
                 checkpoint_id, reason);
    }
    
    // Invoke registered callback if set
    std::lock_guard<std::mutex> lock(mutex_);
    if (rollback_callback_) {
        try {
            rollback_callback_(checkpoint_id, success, 
                             success ? "" : reason);
        } catch (const std::exception& e) {
            LOG_ERROR("UpdateStateMachine: rollback callback threw: {}", e.what());
        }
    }
}

bool UpdateStateMachine::isRollbackSafe(CheckpointId id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check: checkpoint exists
    auto it = std::find_if(checkpoints_.begin(), checkpoints_.end(),
                           [id](const Checkpoint& cp) { return cp.id == id; });
    if (it == checkpoints_.end()) {
        LOG_WARN("UpdateStateMachine::isRollbackSafe({}) – checkpoint not found", id);
        return false;
    }

    // Check: checkpoint state differs from current state (no-op rollback is allowed but not "safe")
    UpdateState current = state_.load(std::memory_order_acquire);
    if (it->state == current) {
        LOG_WARN("UpdateStateMachine::isRollbackSafe({}) – checkpoint state matches current, no rollback needed", id);
        return false;  // Already at target state; rollback is a no-op
    }

    // Check: no in-flight update in progress
    if (has_inflight_update_) {
        LOG_WARN("UpdateStateMachine::isRollbackSafe({}) – in-flight update detected, rollback unsafe", id);
        return false;
    }

    // All checks passed; rollback is safe
    LOG_DEBUG("UpdateStateMachine::isRollbackSafe({}) – safe to rollback", id);
    return true;
}

bool UpdateStateMachine::validateRollbackState(CheckpointId id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find the target checkpoint
    auto it = std::find_if(checkpoints_.begin(), checkpoints_.end(),
                           [id](const Checkpoint& cp) { return cp.id == id; });
    if (it == checkpoints_.end()) {
        LOG_ERROR("UpdateStateMachine::validateRollbackState({}) – checkpoint not found: {}", id, reason);
        return false;
    }

    // Verify current state matches checkpoint state
    UpdateState current = state_.load(std::memory_order_acquire);
    if (current != it->state) {
        LOG_ERROR("UpdateStateMachine::validateRollbackState({}) – state mismatch; current={}, expected={}; reason={}",
                  id, stateToString(current), stateToString(it->state), reason);
        return false;
    }

    // Verify current version matches checkpoint version
    if (current_version_ != it->version) {
        LOG_ERROR("UpdateStateMachine::validateRollbackState({}) – version mismatch; current='{}', expected='{}'; reason={}",
                  id, current_version_, it->version, reason);
        return false;
    }

    // Validation passed
    LOG_INFO("UpdateStateMachine::validateRollbackState({}) – rollback state validated successfully", id);
    return true;
}

CheckpointId UpdateStateMachine::lastRollbackCheckpoint() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_rollback_id_.has_value() ? last_rollback_id_.value() : 0;
}

uint32_t UpdateStateMachine::rollbackAttemptCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rollback_attempt_count_;
}

bool UpdateStateMachine::isLastRollbackIdempotent() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_rollback_was_idempotent_;
}

} // namespace updates
} // namespace themis
