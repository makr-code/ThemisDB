/**
 * @file training_incident_emitter.h
 * @brief Unified incident emitter for training module incident classes (Phase 3).
 *
 * Provides a listener-pattern emitter that unifies diagnostic incident reporting
 * across the three training incident classes:
 *  - Dataset (labeling, enrichment, data validation)
 *  - Training (step execution, convergence, checkpoint)
 *  - Adapter  (merge, serving, handoff)
 *
 * All three classes emit @ref TrainingIncident events to registered
 * @ref TrainingIncidentListener implementations. The emitter is thread-safe:
 * listener registration and broadcast are protected by an internal mutex.
 *
 * Example:
 * @code
 * TrainingIncidentEmitter emitter;
 * emitter.addListener(std::make_shared<MyAuditListener>());
 *
 * // From auto-labeler code:
 * emitter.emitDatasetIncident(
 *     TrainingErrorCode::DATASET_LABEL_CONFLICT,
 *     "auto_labeler", "label_batch",
 *     "Conflicting labels detected in batch B-42",
 *     /*recoverable=*/true,
 *     "batch_id=B-42 conflict_count=3");
 * @endcode
 *
 * @version 1.0.0
 * @date 2026-08-10
 * @since v2.4.0 (Phase 3: Error Handling & Edge Cases)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "training/training_error_codes.h"

#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace training {

// ============================================================================
// Incident class discriminator
// ============================================================================

/**
 * @brief Discriminator for the three training incident classes.
 *
 * Used by @ref TrainingIncident to identify which subsystem emitted the event.
 */
enum class TrainingIncidentClass : uint8_t {
    /// Labeling, enrichment, dataset selection, or data validation faults.
    DATASET  = 0,
    /// Training-step execution, convergence monitoring, or checkpoint faults.
    TRAINING = 1,
    /// Adapter merge, serving, or handoff faults.
    ADAPTER  = 2,
};

/**
 * @brief Human-readable label for a @ref TrainingIncidentClass value.
 *
 * @param cls Incident class discriminator.
 * @return Null-terminated string literal ("dataset", "training", or "adapter").
 */
inline const char* trainingIncidentClassName(TrainingIncidentClass cls) noexcept {
    switch (cls) {
        case TrainingIncidentClass::DATASET:  return "dataset";
        case TrainingIncidentClass::TRAINING: return "training";
        case TrainingIncidentClass::ADAPTER:  return "adapter";
    }
    return "unknown";
}

// ============================================================================
// Incident event
// ============================================================================

/**
 * @brief A single diagnostic incident emitted by a training subsystem.
 *
 * All fields are value-type (no pointers); incidents may be captured in
 * listener queues, audit logs, or telemetry pipelines without ownership
 * concerns.
 */
struct TrainingIncident {
    /// Which subsystem produced this incident.
    TrainingIncidentClass incident_class = TrainingIncidentClass::TRAINING;

    /// Structured error code from the training error taxonomy.
    TrainingErrorCode error_code = TrainingErrorCode::SUCCESS;

    /// Component identifier (e.g., "auto_labeler", "incremental_lora_trainer").
    std::string component;

    /// Operation name within the component (e.g., "label_batch", "train_step").
    std::string operation;

    /// Human-readable summary of the incident.
    std::string message;

    /// Whether the caller believes the fault is recoverable without operator action.
    bool is_recoverable = false;

    /// Additional key=value context string for post-mortem analysis.
    std::string context;

    /// Wall-clock time at emission (UTC seconds since epoch).
    std::time_t timestamp = 0;
};

// ============================================================================
// Listener interface
// ============================================================================

/**
 * @brief Abstract listener interface for training incident events.
 *
 * Implementations receive every incident emitted by @ref TrainingIncidentEmitter.
 * `onIncident` is called with the emitter's internal mutex held, so implementations
 * MUST NOT call back into the emitter (to avoid deadlock) and SHOULD return quickly
 * to avoid stalling the emitting thread.
 *
 * @par Thread-safety contract
 * The emitter calls `onIncident` under a mutex. Implementations are responsible
 * for their own internal thread safety (e.g., if they forward to a queue).
 */
class TrainingIncidentListener {
public:
    virtual ~TrainingIncidentListener() = default;

    /**
     * @brief Receive a training incident event.
     *
     * @param incident The incident. All fields are populated before the call.
     */
    virtual void onIncident(const TrainingIncident& incident) = 0;
};

// ============================================================================
// Emitter
// ============================================================================

/**
 * @brief Thread-safe incident emitter for all three training incident classes.
 *
 * Centralises structured diagnostic emission across labeling/enrichment
 * (dataset), training execution (training), and adapter lifecycle (adapter)
 * code paths. Any number of @ref TrainingIncidentListener instances may be
 * registered; all are notified synchronously on each `emit*` call.
 *
 * @par Registration
 * Listeners are stored as `shared_ptr`. The emitter does not extend their
 * lifetime beyond its own destruction, but the `shared_ptr` prevents
 * premature deletion while the emitter holds a reference.
 *
 * @par Performance
 * `emit*` calls acquire a `std::mutex`; keep listeners lightweight or
 * forward events asynchronously to avoid stalling training hot paths.
 */
class TrainingIncidentEmitter {
public:
    TrainingIncidentEmitter() = default;

    // Non-copyable, non-movable (listener list stability)
    TrainingIncidentEmitter(const TrainingIncidentEmitter&)            = delete;
    TrainingIncidentEmitter& operator=(const TrainingIncidentEmitter&) = delete;

    // -------------------------------------------------------------------------
    // Listener management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a listener to receive all subsequent incidents.
     *
     * @param listener Non-null shared_ptr to the listener. Duplicate registrations
     *                 result in duplicate notifications.
     */
    void addListener(std::shared_ptr<TrainingIncidentListener> listener) {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.push_back(std::move(listener));
    }

    /**
     * @brief Remove all registered listeners.
     *
     * After this call no incidents will be forwarded until new listeners are added.
     */
    void removeListeners() {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.clear();
    }

    /**
     * @brief Number of currently registered listeners.
     */
    size_t listenerCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return listeners_.size();
    }

    // -------------------------------------------------------------------------
    // Dataset incident class
    // -------------------------------------------------------------------------

    /**
     * @brief Emit a dataset-class incident (labeling, enrichment, validation).
     *
     * @param error_code    Structured error code from the training taxonomy.
     * @param component     Emitting component (e.g., "auto_labeler").
     * @param operation     Operation name (e.g., "label_batch").
     * @param message       Human-readable incident summary.
     * @param recoverable   Whether the fault can recover without operator action.
     * @param context       Additional key=value diagnostic context string.
     */
    void emitDatasetIncident(TrainingErrorCode  error_code,
                             const std::string& component,
                             const std::string& operation,
                             const std::string& message,
                             bool               recoverable = false,
                             const std::string& context     = {}) {
        broadcast(makeIncident(TrainingIncidentClass::DATASET,
                               error_code, component, operation,
                               message, recoverable, context));
    }

    // -------------------------------------------------------------------------
    // Training incident class
    // -------------------------------------------------------------------------

    /**
     * @brief Emit a training-class incident (step, convergence, checkpoint).
     *
     * @param error_code    Structured error code from the training taxonomy.
     * @param component     Emitting component (e.g., "incremental_lora_trainer").
     * @param operation     Operation name (e.g., "train_step").
     * @param message       Human-readable incident summary.
     * @param recoverable   Whether the fault can recover without operator action.
     * @param context       Additional key=value diagnostic context string.
     */
    void emitTrainingIncident(TrainingErrorCode  error_code,
                              const std::string& component,
                              const std::string& operation,
                              const std::string& message,
                              bool               recoverable = false,
                              const std::string& context     = {}) {
        broadcast(makeIncident(TrainingIncidentClass::TRAINING,
                               error_code, component, operation,
                               message, recoverable, context));
    }

    // -------------------------------------------------------------------------
    // Adapter incident class
    // -------------------------------------------------------------------------

    /**
     * @brief Emit an adapter-class incident (merge, serving, handoff).
     *
     * @param error_code    Structured error code from the training taxonomy.
     * @param component     Emitting component (e.g., "adapter_serving").
     * @param operation     Operation name (e.g., "deploy_version").
     * @param message       Human-readable incident summary.
     * @param recoverable   Whether the fault can recover without operator action.
     * @param context       Additional key=value diagnostic context string.
     */
    void emitAdapterIncident(TrainingErrorCode  error_code,
                             const std::string& component,
                             const std::string& operation,
                             const std::string& message,
                             bool               recoverable = false,
                             const std::string& context     = {}) {
        broadcast(makeIncident(TrainingIncidentClass::ADAPTER,
                               error_code, component, operation,
                               message, recoverable, context));
    }

private:
    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    static TrainingIncident makeIncident(TrainingIncidentClass cls,
                                         TrainingErrorCode  error_code,
                                         const std::string& component,
                                         const std::string& operation,
                                         const std::string& message,
                                         bool               recoverable,
                                         const std::string& context) {
        TrainingIncident inc;
        inc.incident_class = cls;
        inc.error_code     = error_code;
        inc.component      = component;
        inc.operation      = operation;
        inc.message        = message;
        inc.is_recoverable = recoverable;
        inc.context        = context;
        inc.timestamp      = std::time(nullptr);
        return inc;
    }

    void broadcast(const TrainingIncident& incident) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& listener : listeners_) {
            if (listener) {
                listener->onIncident(incident);
            }
        }
    }

    mutable std::mutex                                       mutex_;
    std::vector<std::shared_ptr<TrainingIncidentListener>>   listeners_;
};

}  // namespace training
}  // namespace themis
