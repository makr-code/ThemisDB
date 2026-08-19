/**
 * @file model_serving.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * Configuration passed to ModelServingEngine at construction time.
 */
struct ModelServingConfig {
    /// Maximum number of models that may be registered simultaneously.
    size_t max_models       = 100;

    /// Maximum number of data-points accepted in a single predictBatch call.
    size_t max_batch_size   = 10'000;

    /// Collect per-call latency observations (small overhead).
    bool   track_latency    = true;

    /// Sliding-window size used to compute p99 latency.
    size_t latency_window   = 1'000;

    /// Require SHA-256 integrity metadata for loadModel() operations.
    bool   require_model_integrity = false;
};

// ============================================================================
// ModelInfo
// ============================================================================

/**
 * Metadata about a model that has been registered with the engine.
 */
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

/**
 * Runtime statistics accumulated for one registered model.
 */
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

/**
 * Central registry for trained AutoML models.
 *
 * Models are identified by (name, version) pairs.  The engine supports
 * online single-record inference with sub-millisecond overhead, batch
 * inference for throughput-optimised workloads, class-probability
 * output, and lightweight health-metric collection.
 *
 * @code
 *   using namespace themisdb::analytics;
 *
 *   // --- Train a model (via AutoML) ---
 *   AutoML automl;
 *   auto model = automl.trainClassifier(training_data, {
 *       .target = "churn",
 *       .metric = AutoMLMetric::F1
 *   });
 *
 *   // --- Register and serve ---
 *   ModelServingEngine engine;
 *   engine.registerModel("churn-predictor", "v1", std::move(model));
 *
 *   // Online inference (single record)
 *   DataPoint dp;
 *   dp.set("age", 35.0);
 *   dp.set("tenure_months", 12.0);
 *   std::string label = engine.predict("churn-predictor", "v1", dp);
 *
 *   // Batch inference
 *   auto labels = engine.predictBatch("churn-predictor", "v1", batch);
 *
 *   // Health metrics
 *   auto h = engine.healthMetrics("churn-predictor", "v1");
 *   if (h) {
 *       std::cout << "avg_latency_ms=" << h->avg_latency_ms << "\n";
 *   }
 * @endcode
 */
class ModelServingEngine {
public:
    explicit ModelServingEngine(ModelServingConfig config = {});
    ~ModelServingEngine();

    ModelServingEngine(const ModelServingEngine&)            = delete;
    ModelServingEngine& operator=(const ModelServingEngine&) = delete;

    // ---- Registry management ----

    /**
     * Register a trained AutoML model under (name, version).
     *
     * @throws std::invalid_argument if name or version is empty.
     * @throws std::runtime_error    if the registry is full
     *                                (exceeds ModelServingConfig::max_models).
     * @throws std::runtime_error    if a model with the same (name,version)
     *                                is already registered.
     */
    void registerModel(const std::string& name,
                       const std::string& version,
                       AutoMLModel        model);

    /**
     * Unregister the model identified by (name, version).
     *
     * @return true if the model was found and removed; false otherwise.
     */
    bool unregisterModel(const std::string& name,
                         const std::string& version);

    // ---- Inference ----

    /**
     * Predict the label / value for a single DataPoint.
     *
     * @throws std::out_of_range if no model is registered under (name,version).
     */
    std::string predict(const std::string& name,
                        const std::string& version,
                        const DataPoint&   point) const;

    /**
     * Predict labels / values for a batch of DataPoints.
     *
     * Returns one string per input point in the same order.
     *
     * @throws std::out_of_range if no model is registered under (name,version).
     * @throws std::invalid_argument if data.size() > ModelServingConfig::max_batch_size.
     */
    std::vector<std::string> predictBatch(
        const std::string&        name,
        const std::string&        version,
        const std::vector<DataPoint>& data) const;

    /**
     * Return class probabilities for a batch (classification models only).
     *
     * Outer vector: one entry per data-point.
     * Inner map: class label → probability in [0,1].
     *
     * For regression models the inner map contains a single entry
     * {"value" → predicted_double}.
     *
     * @throws std::out_of_range if no model is registered under (name,version).
     * @throws std::invalid_argument if data.size() > ModelServingConfig::max_batch_size.
     */
    std::vector<std::map<std::string, double>> predictProba(
        const std::string&            name,
        const std::string&            version,
        const std::vector<DataPoint>& data) const;

    // ---- Registry queries ----

    /**
     * Return metadata for all registered models (unordered).
     */
    std::vector<ModelInfo> listModels() const;

    /**
     * Return metadata for a specific model, or nullopt if not registered.
     */
    std::optional<ModelInfo> modelInfo(const std::string& name,
                                       const std::string& version) const;

    /**
     * Return health metrics for a specific model, or nullopt if not registered.
     */
    std::optional<ModelHealthMetrics> healthMetrics(const std::string& name,
                                                     const std::string& version) const;

    /**
     * Return true iff (name, version) is currently registered.
     */
    bool isRegistered(const std::string& name,
                      const std::string& version) const;

    // ---- Persistence ----

    /**
     * Serialise a registered model to a string (delegates to AutoMLModel::serialize).
     *
     * @throws std::out_of_range if not registered.
     */
    std::string serializeModel(const std::string& name,
                                const std::string& version) const;

    /**
     * Deserialise and register a model previously serialised via serializeModel.
     *
     * Equivalent to constructing an AutoMLModel via AutoMLModel::deserialize
     * and calling registerModel(name, version, std::move(m)).
     *
     * @throws std::invalid_argument if name or version is empty.
     * @throws std::runtime_error    if the registry is full or (name,version)
     *                                is already registered.
     * @throws std::invalid_argument if integrity is required but no hash was provided.
     * @throws std::runtime_error    if a provided SHA-256 hash does not match.
     */
    void loadModel(const std::string& name,
                   const std::string& version,
                   const std::string& serialized_data);

    /**
     * Deserialise and register a model with explicit SHA-256 integrity check.
     *
     * The caller provides the expected lowercase hex SHA-256 digest of
     * serialized_data. The load operation fails closed on mismatch.
     *
     * @param expected_sha256_hex  Expected SHA-256 digest (64 lowercase hex chars).
     * @throws std::invalid_argument if expected_sha256_hex is empty.
     * @throws std::runtime_error    if digest mismatch.
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
 * Build the canonical registry key from (name, version).
 * Exposed so external code can build keys consistently.
 */
inline std::string makeModelKey(const std::string& name,
                                 const std::string& version) {
    return name + ":" + version;
}

} // namespace analytics
} // namespace themisdb
