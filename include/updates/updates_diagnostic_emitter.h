/**
 * @file updates_diagnostic_emitter.h
 * @brief Thread-safe diagnostic emitter for the Updates module
 * @version 1.0.0
 * @since 1.8.1 (Q3 2026)
 *
 * Provides structured, machine-parseable logging for update operations:
 *  - State transitions
 *  - Patch operations
 *  - Rollout changes
 *  - Error events
 *
 * Supports both human-readable and JSON output via DiagnosticListener interface.
 *
 * Doxygen maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include "updates/updates_diagnostics.h"
#include <mutex>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace themis {
namespace updates {

/**
 * @brief Interface for receiving diagnostic events
 *
 * Implementations can:
 *  - Write to structured logs (JSON lines, binary, etc.)
 *  - Send to telemetry backends
 *  - Trigger alerts or automated remediation
 */
class DiagnosticListener {
public:
    virtual ~DiagnosticListener() = default;
    
    /**
     * @brief Called when a diagnostic event is emitted
     * 
     * @param context Error or progress context
     * @param is_error true if this is an error event; false if informational
     */
    virtual void onDiagnosticEvent(const ErrorContext& context, bool is_error) = 0;
};

/**
 * @brief Thread-safe diagnostic event emitter
 *
 * Centralizes emission of diagnostic messages for state transitions, patch
 * operations, and rollout changes. Listeners receive events in structured
 * form (ErrorContext) suitable for machine parsing, logging, and automation.
 *
 * Example usage:
 * @code
 *   DiagnosticEmitter emitter;
 *   emitter.addListener(std::make_shared<JsonFileListener>("updates.jsonl"));
 *
 *   ErrorContext ctx;
 *   ctx.operation = "apply_patch";
 *   ctx.phase = "applying";
 *   ctx.error_code = DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH;
 *   ctx.severity = DiagnosticSeverity::ERROR;
 *   ctx.message = "Checksum mismatch for bin/themis_server";
 *
 *   emitter.emitError(ctx);
 * @endcode
 */
class DiagnosticEmitter {
public:
    /**
     * @brief Construct a diagnostic emitter
     */
    explicit DiagnosticEmitter();

    ~DiagnosticEmitter() = default;

    // Non-copyable
    DiagnosticEmitter(const DiagnosticEmitter&) = delete;
    DiagnosticEmitter& operator=(const DiagnosticEmitter&) = delete;

    // Movable
    DiagnosticEmitter(DiagnosticEmitter&&) = default;
    DiagnosticEmitter& operator=(DiagnosticEmitter&&) = default;

    // ========================================================================
    // Listener management
    // ========================================================================

    /**
     * @brief Register a listener to receive diagnostic events
     *
     * Listeners are called in registration order. An exception in one
     * listener does not prevent others from being called.
     *
     * @param listener The listener to add (may be null; no-op if null)
     */
    void addListener(std::shared_ptr<DiagnosticListener> listener);

    /**
     * @brief Remove all registered listeners
     */
    void clearListeners();

    /**
     * @brief Get the number of registered listeners
     */
    size_t listenerCount() const;

    // ========================================================================
    // Event emission
    // ========================================================================

    /**
     * @brief Emit an error diagnostic event
     *
     * Logs at ERROR level for severe issues (CRITICAL/ERROR severity),
     * WARN for degraded paths (WARN severity), INFO otherwise.
     *
     * @param context Error context including code, message, and metadata
     */
    void emitError(const ErrorContext& context);

    /**
     * @brief Emit an informational diagnostic event
     *
     * Used for state transitions, patch apply completion, etc.
     * Logs at INFO level.
     *
     * @param operation Human-readable operation name (e.g., "state_transition")
     * @param phase Current update phase (e.g., "applying", "rolling_back")
     * @param message Status message
     * @param node_id Optional node identifier (empty for local)
     * @param version Optional version string
     */
    void emitInfo(const std::string& operation,
                  const std::string& phase,
                  const std::string& message,
                  const std::string& node_id = "",
                  const std::string& version = "");

    /**
     * @brief Emit a state transition event
     *
     * Convenience method for recording update state changes.
     *
     * @param from_state Previous state name (e.g., "IDLE", "APPLYING")
     * @param to_state New state name
     * @param version Version being processed
     */
    void emitStateTransition(const std::string& from_state,
                            const std::string& to_state,
                            const std::string& version = "");

    /**
     * @brief Emit a checkpoint created event
     *
     * @param checkpoint_id Checkpoint identifier
     * @param description Checkpoint description
     * @param version Version at checkpoint time
     */
    void emitCheckpointCreated(uint64_t checkpoint_id,
                              const std::string& description,
                              const std::string& version);

    /**
     * @brief Emit a checkpoint rollback event
     *
     * @param checkpoint_id Checkpoint identifier being rolled back to
     * @param success true if rollback succeeded
     * @param reason Reason for rollback
     */
    void emitCheckpointRollback(uint64_t checkpoint_id,
                               bool success,
                               const std::string& reason);

    /**
     * @brief Emit a patch apply event
     *
     * @param file_path Path to file being patched
     * @param success true if patch applied successfully
     * @param error_msg Error message if failed (empty if success)
     */
    void emitPatchApply(const std::string& file_path,
                       bool success,
                       const std::string& error_msg = "");

    /**
     * @brief Emit a coordinated rollout event
     *
     * @param operation Operation name (e.g., "start", "node_complete", "failed")
     * @param node_id Node identifier
     * @param success Status
     * @param detail Additional detail (e.g., node status, error reason)
     */
    void emitCoordinatedEvent(const std::string& operation,
                             const std::string& node_id,
                             bool success,
                             const std::string& detail = "");

    // ========================================================================
    // Formatting
    // ========================================================================

    /**
     * @brief Generate a human-readable message for an error context
     *
     * Format:
     *   [CODE:NAME] Operation 'op' in phase 'phase' on node 'node':
     *   message [severity=SEV, root_cause=RC]
     *
     * @param context The error context
     * @return Human-readable message
     */
    static std::string formatErrorMessage(const ErrorContext& context);

private:
    mutable std::mutex listeners_mutex_;
    std::vector<std::shared_ptr<DiagnosticListener>> listeners_;

    /**
     * @brief Invoke all registered listeners for an event
     */
    void invokeListeners(const ErrorContext& context, bool is_error) const;
};

} // namespace updates
} // namespace themis
