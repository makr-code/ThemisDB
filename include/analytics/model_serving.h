/**
 * @file model_serving.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Model Serving and Online Inference Pipeline
 *
 * A thread-safe registry for trained AutoML models that provides
 * low-latency online inference, batch inference, health-metric
 * tracking, and round-trip serialization / deserialization.
 *
 * Features:
 *   - Named + versioned model registry
 *   - Single-record online inference (predictOne)
 *   - Batch inference (predictBatch)
 *   - Class-probability output (predictProba, classification only)
 *   - Per-model health metrics (prediction count, latency percentiles)
 *   - Model serialization / deserialization (round-trip via AutoMLModel)
 *   - Configurable registry capacity and latency-tracking window
 *
 * Thread-safety:
 *   - registerModel / unregisterModel / loadModel are guarded by an
 *     exclusive lock; they are NOT suitable for high-frequency calls.
 *   - predict / predictBatch / predictProba acquire the registry shared
 *     lock only for a brief pointer capture step, then release it before
 *     running inference.  Inference therefore does NOT starve concurrent
 *     registerModel() / unregisterModel() callers.
 *   - The registry stores std::shared_ptr<Entry> so that an Entry object
 *     remains alive (reference-counted) after a concurrent unregisterModel()
 *     erases its map slot, eliminating use-after-free risk.
 *   - listModels / modelInfo / isRegistered are read-only and hold the
 *     shared lock for their full (short) duration.
 *   - healthMetrics and serializeModel capture a shared_ptr under the
 *     registry lock and then perform work (metrics snapshot / serialisation)
 *     outside any registry lock.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Reuse DataPoint and AutoMLModel from the AutoML module.
#include "analytics/automl.h"

namespace themisdb {
namespace analytics {

// ============================================================================
// Forward declarations
// ============================================================================

class ModelServingEngine;
struct ModelServingEntry;

// ============================================================================
// Configuration
// ============================================================================

struct ModelServingConfig {
    size_t max_models       = 100;

    size_t max_batch_size   = 10'000;

    bool   track_latency    = true;

    size_t latency_window   = 1'000;

    bool   require_model_integrity = false;
};

// ============================================================================
// ModelInfo
// ============================================================================

struct ModelInfo {
    std::string    name;            ///< Logical model name
    std::string    version;         ///< Version string (e.g. "1.0", "2024-01")
    AutoMLTask     task      = AutoMLTask::CLASSIFICATION;
    ModelAlgorithm algorithm = ModelAlgorithm::DECISION_TREE;
    EvalMetrics    metrics;         ///< CV metrics from training
    int64_t        registered_at_ms = 0; ///< Unix epoch (ms) when model was registered
    bool           is_active = true;
};

// ============================================================================
// ModelHealthMetrics
// ============================================================================

struct ModelHealthMetrics {
    std::string name;
    std::string version;

    uint64_t total_predictions  = 0; ///< Cumulative single-record predictions
    uint64_t total_batch_calls  = 0; ///< Cumulative batch inference calls
    uint64_t total_batch_records = 0; ///< Cumulative records processed in batches

    double   avg_latency_ms     = 0.0; ///< Rolling average of per-call latency
    double   p99_latency_ms     = 0.0; ///< p99 latency over the latency_window
    double   last_latency_ms    = 0.0; ///< Latency of the most recent call

    int64_t  last_used_ms       = 0;   ///< Epoch-ms of last inference call (0 = never)
};

// ============================================================================
// ModelServingEngine
// ============================================================================

class ModelServingEngine {
public:
    explicit ModelServingEngine(ModelServingConfig config = {});
    ~ModelServingEngine();

    ModelServingEngine(const ModelServingEngine&)            = delete;
    ModelServingEngine& operator=(const ModelServingEngine&) = delete;


    /**
     * @brief Register Model.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @param[in] model Input parameter.
     */
    void registerModel(const std::string& name,
                       const std::string& version,
                       AutoMLModel        model);

    /**
     * @brief Unregister Model.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @return True when the operation succeeds.
     */
    bool unregisterModel(const std::string& name,
                         const std::string& version);


    /**
     * @brief Predict.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @param[in] point Input parameter.
     * @return Return value.
     */
    std::string predict(const std::string& name,
                        const std::string& version,
                        const DataPoint&   point) const;

    /**
     * @brief Predict Batch.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::vector<std::string> predictBatch(
        const std::string&        name,
        const std::string&        version,
        const std::vector<DataPoint>& data) const;

    std::vector<std::map<std::string, double>> predictProba(
        const std::string&            name,
        const std::string&            version,
        const std::vector<DataPoint>& data) const;


    /**
     * @brief List Models.
     * @return Return value.
     */
    std::vector<ModelInfo> listModels() const;

    /**
     * @brief Model Info.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::optional<ModelInfo> modelInfo(const std::string& name,
                                       const std::string& version) const;

    /**
     * @brief Health Metrics.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::optional<ModelHealthMetrics> healthMetrics(const std::string& name,
                                                     const std::string& version) const;

    /**
     * @brief Is Registered.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @return True when the operation succeeds.
     */
    bool isRegistered(const std::string& name,
                      const std::string& version) const;


    /**
     * @brief Serialize Model.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::string serializeModel(const std::string& name,
                                const std::string& version) const;

    /**
     * @brief Load Model.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @param[in] serialized_data Input parameter.
     */
    void loadModel(const std::string& name,
                   const std::string& version,
                   const std::string& serialized_data);

    /**
     * @brief Load Model.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @param[in] serialized_data Input parameter.
     * @param[in] expected_sha256_hex Input parameter.
     */
    void loadModel(const std::string& name,
                   const std::string& version,
                   const std::string& serialized_data,
                   const std::string& expected_sha256_hex);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    // Captures a reference-counted handle to the named entry under a brief
    // shared_lock, then releases the lock immediately.  Throws std::out_of_range
    // if the entry does not exist.  Callers run inference and metric updates
    // *after* this call so that the registry lock is never held during I/O.
    [[nodiscard]] std::shared_ptr<ModelServingEntry>
    lookupEntryOrThrow_(const std::string& name, const std::string& version) const;

    // Same as lookupEntryOrThrow_ but returns nullptr instead of throwing.
    [[nodiscard]] std::shared_ptr<ModelServingEntry>
    lookupEntryOrNull_(const std::string& name, const std::string& version) const noexcept;
};

// ============================================================================
// Free helpers
// ============================================================================

/**
 * @brief Make Model Key.
 * @param[in] name Input parameter.
 * @param[in] version Input parameter.
 * @return Return value.
 * @details Implements makeModelKey without additional internal calls.
 */
inline std::string makeModelKey(const std::string& name,
                                 const std::string& version) {
    return name + ":" + version;
}

} // namespace analytics
} // namespace themisdb
