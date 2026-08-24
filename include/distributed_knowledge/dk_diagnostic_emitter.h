// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file dk_diagnostic_emitter.h
 * @brief Thread-safe diagnostic emitter for the distributed_knowledge module.
 *
 * Provides structured, machine-parseable event emission for federation
 * incidents, merge failures, dedup collisions, and trust-gate violations.
 *
 * ## Listener pattern
 * Register one or more `IDKDiagnosticListener` implementations to receive
 * `DKDiagnosticEvent` structs.  Listeners are called in registration order
 * under the emitter's internal mutex; a throwing listener does not prevent
 * the remaining listeners from being invoked.
 *
 * ## JSON output contract
 * Each event serialises to a flat JSON object.  The key names are stable
 * across patch releases; new optional fields may be added without a version bump.
 *
 * @code
 * {
 *   "event_type": "MERGE_TIMEOUT",
 *   "severity": "WARNING",
 *   "shard_id": "shard-us-east-1",
 *   "operation_id": "merge-20260824-001",
 *   "timestamp_utc": "2026-08-24T10:00:00Z",
 *   "cause": "shard did not respond within 500 ms"
 * }
 * @endcode
 *
 * @since Phase 3 hardening (Q4 2026)
 */

#pragma once

#include "distributed_knowledge/distributed_knowledge_api_contract.h"

#include <chrono>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace distributed_knowledge {

// ============================================================================
// Listener interface
// ============================================================================

/**
 * @brief Interface for receiving distributed-knowledge diagnostic events.
 *
 * Implement this interface to forward events to a logging backend, telemetry
 * system, or test assertion harness.
 */
class IDKDiagnosticListener {
public:
    virtual ~IDKDiagnosticListener() = default;

    /**
     * @brief Called when a diagnostic event is emitted.
     *
     * Must be thread-safe: the emitter holds its internal lock while invoking
     * this method, so implementations must not re-enter the emitter.
     *
     * @param event  The diagnostic event (read-only).
     */
    virtual void onEvent(const DKDiagnosticEvent& event) = 0;
};

// ============================================================================
// Emitter
// ============================================================================

/**
 * @brief Thread-safe diagnostic event emitter for the distributed_knowledge module.
 *
 * Central fan-out point for all federation incidents.  Components obtain a
 * shared pointer to a single emitter instance (or one per coordinator) and
 * call `emit()` with a populated `DKDiagnosticEvent`.
 *
 * ### Thread safety
 * All public methods are safe for concurrent access from multiple threads.
 * Listener callbacks are invoked with the internal mutex held; each listener
 * must complete quickly and must not call back into the emitter.
 *
 * ### Timestamp auto-fill
 * If `event.timestamp_utc` is empty when `emit()` is called, the emitter
 * populates it with the current UTC time formatted as ISO-8601.
 *
 * @since Phase 3 hardening (Q4 2026)
 */
class DistributedKnowledgeDiagnosticEmitter {
public:
    DistributedKnowledgeDiagnosticEmitter() = default;
    ~DistributedKnowledgeDiagnosticEmitter() = default;

    // Non-copyable; movable.
    DistributedKnowledgeDiagnosticEmitter(const DistributedKnowledgeDiagnosticEmitter&)            = delete;
    DistributedKnowledgeDiagnosticEmitter& operator=(const DistributedKnowledgeDiagnosticEmitter&) = delete;
    DistributedKnowledgeDiagnosticEmitter(DistributedKnowledgeDiagnosticEmitter&&) noexcept            = default;
    DistributedKnowledgeDiagnosticEmitter& operator=(DistributedKnowledgeDiagnosticEmitter&&) noexcept = default;

    // -------------------------------------------------------------------------
    // Listener management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a listener to receive future diagnostic events.
     *
     * Listeners are called in registration order.  A null pointer is ignored.
     *
     * @param listener  Shared pointer to the listener implementation.
     */
    void addListener(std::shared_ptr<IDKDiagnosticListener> listener) {
        if (!listener) { return; }
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.push_back(std::move(listener));
    }

    /**
     * @brief Remove all registered listeners.
     */
    void clearListeners() {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.clear();
    }

    /**
     * @brief Return the number of currently registered listeners.
     */
    [[nodiscard]] std::size_t listenerCount() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return listeners_.size();
    }

    // -------------------------------------------------------------------------
    // Emission
    // -------------------------------------------------------------------------

    /**
     * @brief Emit a diagnostic event to all registered listeners.
     *
     * If `event.timestamp_utc` is empty, it is populated with the current UTC
     * time before dispatch.  Each listener is called in registration order; an
     * exception thrown by a listener is caught and suppressed so that subsequent
     * listeners still receive the event.
     *
     * @param event  Diagnostic event to broadcast (copied internally).
     */
    void emit(DKDiagnosticEvent event) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (event.timestamp_utc.empty()) {
            event.timestamp_utc = utcNow();
        }
        for (auto& listener : listeners_) {
            if (!listener) { continue; }
            try {
                listener->onEvent(event);
            } catch (...) {
                // Swallow exceptions from individual listeners to ensure
                // all registered listeners receive the event.
            }
        }
    }

    /**
     * @brief Convenience overload: emit a MERGE_TIMEOUT event.
     *
     * @param shard_id       Shard that timed out.
     * @param operation_id   Merge operation identifier.
     * @param cause          Human-readable description of the timeout.
     * @param severity       Diagnostic severity (default: WARNING).
     */
    void emitMergeTimeout(const std::string& shard_id,
                          const std::string& operation_id,
                          const std::string& cause,
                          DKDiagnosticSeverity severity = DKDiagnosticSeverity::WARNING) {
        DKDiagnosticEvent ev;
        ev.type         = DKDiagnosticEventType::MERGE_TIMEOUT;
        ev.severity     = severity;
        ev.shard_id     = shard_id;
        ev.operation_id = operation_id;
        ev.cause        = cause;
        emit(std::move(ev));
    }

    /**
     * @brief Convenience overload: emit a DEDUP_COLLISION event.
     *
     * @param shard_id       Shard that produced the duplicate entry.
     * @param operation_id   Operation identifier.
     * @param duplicate_key  The duplicated doc_id or summary_id.
     */
    void emitDedupCollision(const std::string& shard_id,
                            const std::string& operation_id,
                            const std::string& duplicate_key) {
        DKDiagnosticEvent ev;
        ev.type              = DKDiagnosticEventType::DEDUP_COLLISION;
        ev.severity          = DKDiagnosticSeverity::INFO;
        ev.shard_id          = shard_id;
        ev.operation_id      = operation_id;
        ev.cause             = "duplicate key: " + duplicate_key;
        ev.metadata["key"]   = duplicate_key;
        emit(std::move(ev));
    }

    /**
     * @brief Convenience overload: emit a TRUST_GATE_REJECT event.
     *
     * @param shard_id       Shard whose announcement was rejected.
     * @param operation_id   Operation identifier.
     * @param cause          Reason for rejection.
     */
    void emitTrustGateReject(const std::string& shard_id,
                             const std::string& operation_id,
                             const std::string& cause) {
        DKDiagnosticEvent ev;
        ev.type         = DKDiagnosticEventType::TRUST_GATE_REJECT;
        ev.severity     = DKDiagnosticSeverity::ERROR;
        ev.shard_id     = shard_id;
        ev.operation_id = operation_id;
        ev.cause        = cause;
        emit(std::move(ev));
    }

    /**
     * @brief Convenience overload: emit a PARTIAL_SHARD_MERGE event.
     *
     * @param operation_id       Merge operation identifier.
     * @param responding_shards  Number of shards that responded.
     * @param total_shards       Total number of shards contacted.
     */
    void emitPartialShardMerge(const std::string& operation_id,
                               std::size_t responding_shards,
                               std::size_t total_shards) {
        DKDiagnosticEvent ev;
        ev.type                         = DKDiagnosticEventType::PARTIAL_SHARD_MERGE;
        ev.severity                     = DKDiagnosticSeverity::WARNING;
        ev.operation_id                 = operation_id;
        ev.cause                        = "partial response: " + std::to_string(responding_shards)
                                          + "/" + std::to_string(total_shards) + " shards";
        ev.metadata["responding_shards"] = std::to_string(responding_shards);
        ev.metadata["total_shards"]      = std::to_string(total_shards);
        emit(std::move(ev));
    }

    /**
     * @brief Convenience overload: emit a FEDERATION_ROLLBACK event.
     *
     * @param operation_id  Rollback operation identifier.
     * @param cause         Reason for rollback.
     * @param severity      Diagnostic severity (default: ERROR).
     */
    void emitFederationRollback(const std::string& operation_id,
                                const std::string& cause,
                                DKDiagnosticSeverity severity = DKDiagnosticSeverity::ERROR) {
        DKDiagnosticEvent ev;
        ev.type         = DKDiagnosticEventType::FEDERATION_ROLLBACK;
        ev.severity     = severity;
        ev.operation_id = operation_id;
        ev.cause        = cause;
        emit(std::move(ev));
    }

private:
    mutable std::mutex                                    mutex_;
    std::vector<std::shared_ptr<IDKDiagnosticListener>>   listeners_;

    /// Return current UTC time as an ISO-8601 string (seconds precision).
    [[nodiscard]] static std::string utcNow() {
        const auto now  = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        char buf[32]{};
        std::tm tm_buf{};
#if defined(_WIN32)
        gmtime_s(&tm_buf, &time);
#else
        gmtime_r(&time, &tm_buf);
#endif
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
        return buf;
    }
};

} // namespace distributed_knowledge
} // namespace themis
