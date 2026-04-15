/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            update_state_machine.h                             ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-15 05:39:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     264                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fa516498e3  2026-04-12  [WIP] Update module documentation and inventory (#4521) ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <functional>
#include <vector>
#include <optional>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace updates {

using json = nlohmann::json;

// Forward declaration for optional history-logger integration
class UpdateHistoryLogger;

/**
 * @brief States for the update state machine
 *
 * State transitions:
 *   IDLE → DOWNLOADING → VERIFYING → APPLYING → IDLE (success)
 *                                              → ROLLING_BACK → IDLE
 *   Any state → FAILED (on unrecoverable error)
 *   FAILED → IDLE (after reset)
 */
enum class UpdateState {
    IDLE,           ///< No update in progress
    DOWNLOADING,    ///< Downloading release files
    VERIFYING,      ///< Verifying signatures and hashes
    APPLYING,       ///< Atomically applying file updates
    ROLLING_BACK,   ///< Rolling back to previous version
    FAILED          ///< Unrecoverable failure; requires manual reset
};

/**
 * @brief Entry in the update transaction log
 */
struct UpdateTransactionEntry {
    UpdateState from_state;
    UpdateState to_state;
    std::string version;
    std::string message;
    std::chrono::system_clock::time_point timestamp;

    json toJson() const;
    static std::optional<UpdateTransactionEntry> fromJson(const json& j);
};

/// Opaque identifier returned by createCheckpoint() and accepted by rollbackToCheckpoint().
using CheckpointId = uint64_t;

/**
 * @brief Snapshot of the state machine captured by createCheckpoint().
 *
 * Stored in-memory; checkpoints survive for the lifetime of the
 * UpdateStateMachine instance.
 */
struct Checkpoint {
    /// Monotonically increasing identifier (1-based).
    CheckpointId id{0};
    /// State at the time the checkpoint was created.
    UpdateState state{UpdateState::IDLE};
    /// Version string at the time the checkpoint was created.
    std::string version;
    /// Optional human-readable description supplied by the caller.
    std::string description;
    /// Wall-clock time the checkpoint was recorded.
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Thread-safe state machine for orchestrating update operations
 *
 * Features:
 * - Atomic state transitions with validation
 * - Persistent transaction log for crash recovery
 * - Observer callbacks on state changes
 * - In-flight update detection on startup
 */
class UpdateStateMachine {
public:
    using StateChangeCallback =
        std::function<void(UpdateState /*from*/, UpdateState /*to*/, const std::string& /*version*/)>;

    /**
     * @brief Construct state machine
     * @param log_path  Path to the persistent transaction log (JSON lines file)
     */
    explicit UpdateStateMachine(const std::string& log_path = "");

    ~UpdateStateMachine() = default;

    // Non-copyable, movable
    UpdateStateMachine(const UpdateStateMachine&) = delete;
    UpdateStateMachine& operator=(const UpdateStateMachine&) = delete;
    UpdateStateMachine(UpdateStateMachine&&) = default;
    UpdateStateMachine& operator=(UpdateStateMachine&&) = default;

    /**
     * @brief Get the current state
     */
    UpdateState currentState() const;

    /**
     * @brief Get the version currently being processed (empty if IDLE)
     */
    std::string currentVersion() const;

    /**
     * @brief Attempt a state transition
     * @param to      Target state
     * @param version Version being updated (kept when transitioning within an update)
     * @param message Human-readable reason / progress note
     * @return true   if the transition was valid and applied
     */
    bool transition(UpdateState to,
                    const std::string& version = "",
                    const std::string& message = "");

    /**
     * @brief Reset a FAILED state back to IDLE (manual operator action)
     */
    void reset();

    /**
     * @brief Register a callback that fires on every state change
     */
    void addStateChangeCallback(StateChangeCallback cb);

    /**
     * @brief Check whether an update was in-flight when the process last stopped
     *        (detected by reading the persisted transaction log at startup)
     * @return true if the log indicates an incomplete update
     */
    bool hasInFlightUpdate() const;

    /**
     * @brief Get the last in-flight version from the transaction log
     * @return Version string, empty if none
     */
    std::string inFlightVersion() const;

    /**
     * @brief Get the full transaction log (newest first)
     */
    std::vector<UpdateTransactionEntry> transactionLog() const;

    /**
     * @brief Persist current state to the log file (called automatically on transitions)
     */
    void persistState(const std::string& version, const std::string& message);

    /**
     * @brief Load state from the persistent log (called in constructor)
     */
    void loadPersistedState();

    /**
     * @brief Human-readable name for a state
     */
    static std::string stateName(UpdateState s);

    // -------------------------------------------------------------------------
    // Rollback checkpoint API (v1.8.0 – Q2 2026)
    // -------------------------------------------------------------------------

    /**
     * @brief Attach an UpdateHistoryLogger for auditability.
     *
     * When set, createCheckpoint() and rollbackToCheckpoint() emit entries
     * to the provided logger.  The logger pointer must remain valid for the
     * lifetime of this state machine (or until replaced by another call).
     *
     * @param logger Pointer to the logger, or nullptr to disable.
     */
    void setHistoryLogger(UpdateHistoryLogger* logger);

    /**
     * @brief Capture the current state as a named rollback point.
     *
     * Checkpoints are stored in memory and survive until the state machine is
     * destroyed or clearCheckpoints() is called.  Each checkpoint receives a
     * monotonically increasing @ref CheckpointId.
     *
     * If a history logger has been attached via setHistoryLogger(), a
     * "checkpoint_created" entry is appended to it automatically.
     *
     * @param description Optional human-readable label for the checkpoint.
     * @return            The identifier of the newly created checkpoint.
     */
    CheckpointId createCheckpoint(const std::string& description = "");

    /**
     * @brief Restore the state machine to a previously created checkpoint.
     *
     * The state and current version are overwritten with the values captured
     * at checkpoint creation time.  Checkpoints newer than @p id are removed
     * from the in-memory list.  The restoration is recorded in the transaction
     * log and, if a history logger is attached, emitted as a "checkpoint_rollback"
     * event.
     *
     * @param id  Identifier of the target checkpoint (returned by createCheckpoint()).
     * @return    true if a checkpoint with the given id was found and applied;
     *            false if no such checkpoint exists.
     */
    bool rollbackToCheckpoint(CheckpointId id);

    /**
     * @brief Return all checkpoints in creation order (oldest first).
     */
    std::vector<Checkpoint> listCheckpoints() const;

    /**
     * @brief Remove all stored checkpoints from memory.
     */
    void clearCheckpoints();

private:
    bool isValidTransition(UpdateState from, UpdateState to) const;
    void appendLogEntry(const UpdateTransactionEntry& entry);

    mutable std::mutex mutex_;
    std::atomic<UpdateState> state_{UpdateState::IDLE};
    std::string current_version_;
    std::string log_path_;
    bool has_inflight_update_ = false;
    std::string inflight_version_;
    std::vector<UpdateTransactionEntry> transaction_log_;
    std::vector<StateChangeCallback> callbacks_;
    std::vector<Checkpoint> checkpoints_;
    uint64_t next_checkpoint_id_{1};
    UpdateHistoryLogger* history_logger_{nullptr};
};

} // namespace updates
} // namespace themis
