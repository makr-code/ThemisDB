/**
 * @file update_state_machine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.45
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * @brief Strategy for handling rollback failures in coordinated scenarios
 * @since 1.8.1 (Q3 2026)
 */
enum class RollbackFallbackStrategy {
    IMMEDIATE_ABORT,    ///< Stop on first rollback failure (fail-fast)
    PARTIAL_CONTINUE,   ///< Continue rolling back other nodes despite local failure
    DEFER               ///< Queue failed rollback for later retry
};

/**
 * @brief Callback for handling rollback completion or failure
 * 
 * Invoked after rollbackToCheckpoint() completes (success or failure).
 * Allows callers to implement custom recovery logic.
 * 
 * @param checkpoint_id The checkpoint that was rolled back to
 * @param success true if rollback succeeded, false otherwise
 * @param error_msg Error description (empty if success)
 * @since 1.8.1 (Q3 2026)
 */
using RollbackCallback = std::function<void(CheckpointId checkpoint_id, 
                                            bool success, 
                                            const std::string& error_msg)>;

/**
 * @brief Snapshot of the state machine captured by createCheckpoint().
 *
 * Stored in-memory; checkpoints survive for the lifetime of the
 * UpdateStateMachine instance. Can be persisted to a log file for
 * crash recovery.
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

    /// Serialize checkpoint to JSON for persistence
    json toJson() const;
    /// Deserialize checkpoint from JSON
    static std::optional<Checkpoint> fromJson(const json& j);
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
     * @param checkpoints_log_path  Path to the persistent checkpoints log (JSON lines file)
     */
    explicit UpdateStateMachine(const std::string& log_path = "",
                                const std::string& checkpoints_log_path = "");

    ~UpdateStateMachine() = default;

    // Non-copyable, movable
    UpdateStateMachine(const UpdateStateMachine&) = delete;
    UpdateStateMachine& operator=(const UpdateStateMachine&) = delete;
    UpdateStateMachine(UpdateStateMachine&&) noexcept = default;
    UpdateStateMachine& operator=(UpdateStateMachine&&) noexcept = default;

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
     * IDEMPOTENCY: Calling this method multiple times with the same checkpoint ID
     * will return true on all calls, but the actual rollback operation is only
     * performed once. Subsequent calls are logged as idempotent and do not modify state.
     *
     * @param id  Identifier of the target checkpoint (returned by createCheckpoint()).
     * @return    true if a checkpoint with the given id was found and applied;
     *            false if no such checkpoint exists or was never rolled back to.
     *            Returns true for idempotent repeat calls to the same checkpoint.
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

    // -------------------------------------------------------------------------
    // Partial and coordinated rollback enhancements (v1.8.1 – Q3 2026)
    // -------------------------------------------------------------------------

    /**
     * @brief Register a callback that fires when rollback completes
     *
     * When set, rollbackToCheckpoint() and rollbackToLatestCheckpoint() 
     * invoke this callback with the result.  Allows callers to implement
     * custom recovery strategies.
     *
     * @param callback The callback to invoke on rollback completion
     * @since 1.8.1
     */
    void setRollbackCallback(RollbackCallback callback);

    /**
     * @brief Perform partial rollback to the latest checkpoint
     *
     * This is the primary entry point for coordinated rollback scenarios.
     * If no checkpoints exist, logs ERROR and returns false.
     *
     * Emits diagnostic WARN or ERROR depending on success.
     *
     * @return true if rollback succeeded; false if no checkpoints or rollback failed
     * @since 1.8.1
     */
    bool rollbackToLatestCheckpoint();

    /**
     * @brief Perform rollback with a fallback strategy for failure handling
     *
     * When rollback to @p checkpoint_id fails, applies the @p fallback_strategy:
     *  - IMMEDIATE_ABORT: returns false immediately (fail-fast)
     *  - PARTIAL_CONTINUE: attempts recovery and returns true (optimistic)
     *  - DEFER: queues the rollback for retry and returns false
     *
     * The decision enables coordinated managers to handle per-node rollback
     * failures without cascading the failure across the cluster.
     *
     * @param checkpoint_id Target checkpoint
     * @param fallback_strategy Strategy to apply on failure
     * @return true if rollback succeeded or was deferred with PARTIAL_CONTINUE
     * @since 1.8.1
     */
    bool rollbackToCheckpointWithFallback(
        CheckpointId checkpoint_id,
        RollbackFallbackStrategy fallback_strategy = RollbackFallbackStrategy::IMMEDIATE_ABORT
    );

    /**
     * @brief Get the count of stored checkpoints
     * @return Number of in-memory checkpoints
     * @since 1.8.1
     */
    size_t checkpointCount() const;

    /**
     * @brief Check if any deferred rollbacks are pending
     *
     * When using DEFER fallback strategy, failed rollbacks are queued.
     * Callers can poll this to trigger retry logic.
     *
     * @return true if there are pending rollback retries
     * @since 1.8.1
     */
    bool hasPendingRollback() const;

    /**
     * @brief Emit diagnostic message on rollback attempt
     *
     * Called internally when a rollback is attempted. Logs at ERROR level
     * if rollback fails, WARN if it succeeds but with degradation.
     *
     * @param checkpoint_id The checkpoint being rolled back to
     * @param success true if rollback succeeded
     * @param reason Human-readable reason for the rollback
     * @since 1.8.1
     */
    void emitRollbackDiagnostic(CheckpointId checkpoint_id, 
                                bool success, 
                                const std::string& reason);

    /**
     * @brief Check if a rollback to the specified checkpoint is safe to perform.
     *
     * A rollback is considered safe if:
     * - The checkpoint exists in the checkpoints list
     * - The checkpoint's state is different from the current state
     * - No in-flight update is in progress
     *
     * Error Code: 7407 - Unsafe rollback detected
     *
     * @param id The checkpoint to validate
     * @return true if rollback would be safe; false otherwise
     * @since 1.8.2 (Wave A)
     */
    bool isRollbackSafe(CheckpointId id) const;

    /**
     * @brief Validate that a rollback operation was correctly applied to the state machine.
     *
     * After performing a rollback, this method can be called to verify that:
     * - The current state matches the checkpoint state
     * - The current version matches the checkpoint version
     * - No partial updates are visible
     *
     * Error Code: 7406 - Rollback state validation failed
     *
     * @param id The checkpoint that was rolled back to
     * @param reason Optional human-readable reason for the validation (logged on failure)
     * @return true if the rollback state is valid; false otherwise
     * @since 1.8.2 (Wave A)
     */
    bool validateRollbackState(CheckpointId id, const std::string& reason = "");

    /**
     * @brief Get the checkpoint ID of the last rollback operation.
     *
     * Returns the checkpoint ID that was most recently rolled back to.
     * Useful for diagnostics and idempotency tracking.
     *
     * @return Checkpoint ID of last rollback, or 0 if no rollback has been performed
     * @since 1.8.2 (Wave A)
     */
    CheckpointId lastRollbackCheckpoint() const;

    /**
     * @brief Get the number of successful rollback attempts.
     *
     * This counter is incremented only when rollbackToCheckpoint() finds the
     * requested checkpoint (i.e., the checkpoint exists).  Failed calls where
     * the checkpoint is not found do **not** increment the counter.
     * Idempotent repeats (rolling back to the same checkpoint as the previous
     * successful call) are counted.
     * Useful for diagnostics and monitoring.
     *
     * @return Number of successful rollback attempts
     * @since 1.8.2 (Wave A)
     */
    uint32_t rollbackAttemptCount() const;

    /**
     * @brief Check if the last rollback operation was idempotent.
     *
     * Returns true if the most recent rollback call was an idempotent repeat
     * (i.e., rolled back to the same checkpoint as the previous call).
     *
     * @return true if last rollback was idempotent; false if it was a new rollback
     * @since 1.8.2 (Wave A)
     */
    bool isLastRollbackIdempotent() const;

private:
    bool isValidTransition(UpdateState from, UpdateState to) const;
    void appendLogEntry(const UpdateTransactionEntry& entry);
    
    /// Persist a single checkpoint to the checkpoints log file
    /// Error Code: 7401 - Checkpoint file write failed
    void persistCheckpoint(const Checkpoint& cp);
    
    /// Load all checkpoints from the checkpoints log file at startup
    /// Error Code: 7402 - Checkpoint file read failed
    void loadCheckpoints();

    mutable std::mutex mutex_;
    std::atomic<UpdateState> state_{UpdateState::IDLE};
    std::string current_version_;
    std::string log_path_;
    std::string checkpoints_log_path_;  ///< Path to persistent checkpoints log
    bool has_inflight_update_ = false;
    std::string inflight_version_;
    std::vector<UpdateTransactionEntry> transaction_log_;
    std::vector<StateChangeCallback> callbacks_;
    std::vector<Checkpoint> checkpoints_;
    uint64_t next_checkpoint_id_{1};
    UpdateHistoryLogger* history_logger_{nullptr};
    
    // Rollback enhancement members (v1.8.1 – Q3 2026)
    RollbackCallback rollback_callback_;
    std::vector<CheckpointId> deferred_rollbacks_;
    RollbackFallbackStrategy current_fallback_strategy_{RollbackFallbackStrategy::IMMEDIATE_ABORT};

    // Idempotent rollback tracking (v1.8.2 – Wave A)
    /// @brief ID of the checkpoint last rolled back to (0 if none)
    /// @since 1.8.2
    std::optional<CheckpointId> last_rollback_id_;
    
    /// @brief Timestamp of the most recent rollback operation
    /// @since 1.8.2
    std::chrono::system_clock::time_point last_rollback_time_;
    
    /// @brief Number of rollback attempts (including idempotent repeats)
    /// @since 1.8.2
    uint32_t rollback_attempt_count_{0};
    
    /// @brief Whether the last rollback was an idempotent repeat
    /// @since 1.8.2
    bool last_rollback_was_idempotent_{false};
};

} // namespace updates
} // namespace themis
