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

class IDKDiagnosticListener {
public:
    /**
     * @brief IDKDiagnostic Listener.
     * @return Return value.
     */
    virtual ~IDKDiagnosticListener() = default;

    /**
     * @brief On Event.
     * @param[in] event Input parameter.
     */
    virtual void onEvent(const DKDiagnosticEvent& event) = 0;
};

// ============================================================================
// Emitter
// ============================================================================

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
     * @brief Add Listener.
     * @param[in] listener Input parameter.
     * @details Calls: lock(), push_back(), std::move().
     */
    void addListener(std::shared_ptr<IDKDiagnosticListener> listener) {
        if (!listener) { return; }
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.push_back(std::move(listener));
    }

    /**
     * @brief Clear Listeners.
     * @details Calls: lock(), clear().
     */
    void clearListeners() {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.clear();
    }

    [[nodiscard]] std::size_t listenerCount() const noexcept {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return listeners_.size();
    }

    // -------------------------------------------------------------------------
    // Emission
    // -------------------------------------------------------------------------

    /**
     * @brief Emit.
     * @param[in] event Input parameter.
     * @details Calls: lock(), empty(), utcNow(), onEvent().
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
     * @brief Emit Dedup Collision.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] operation_id Identifier of the operation.
     * @param[in] duplicate_key Input parameter.
     * @details Calls: emit(), std::move().
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
     * @brief Emit Trust Gate Reject.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] operation_id Identifier of the operation.
     * @param[in] cause Input parameter.
     * @details Calls: emit(), std::move().
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
     * @brief Emit Partial Shard Merge.
     * @param[in] operation_id Identifier of the operation.
     * @param[in] responding_shards Input parameter.
     * @param[in] total_shards Input parameter.
     * @details Calls: std::to_string(), emit(), std::move().
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
