/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            update_state_machine.h                             ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-22 08:22:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     182                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
#include <nlohmann/json.hpp>

namespace themis {
namespace updates {

using json = nlohmann::json;

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
};

} // namespace updates
} // namespace themis
