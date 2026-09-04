/**
 * @file model_serving.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * Model Serving and Online Inference Pipeline – Implementation
 *
 * @module Serving
 *
 * Data flow:
 *   ModelServingEngine::registerModel(name, version, model, info)
 *     → stored in entry map keyed by "name:version" (exclusive lock)
 *   ModelServingEngine::predict(name, version, point)
 *     → shared lock lookup → AutoMLModel::predict(point)
 *     → InferenceResult{class_label, probabilities} + latency update
 *   ModelServingEngine::predictBatch(name, version, points)
 *     → per-point predict() loop; no batch-optimized path currently
 *
 * Error paths:
 *   - `std::invalid_argument`: unknown model name/version in predict* or
 *     unregister calls.
 *   - `std::runtime_error`: inference failure inside AutoMLModel::predict()
 *     propagates to caller; health metrics record the failure.
 *   - `std::invalid_argument`: duplicate registration (same name+version)
 *     when called via loadModel() with existing key.
 *
 * Cross-links:
 *   include/analytics/model_serving.h — ModelServingEngine public API
 *   src/analytics/ml_serving.cpp — external ONNX/TF Serving backend
 *   tests/analytics/test_model_serving.cpp — registry, inference, health metrics
 *   - Each registered model is stored in an Entry that bundles the
 *     AutoMLModel, its ModelInfo, and a mutable ModelHealthMetrics.
 *   - Entries are keyed by "name:version" in a std::unordered_map.
 *   - A std::shared_mutex protects the map: read operations (predict*,
 *     list*, health*) acquire a shared lock; write operations
 *     (register, unregister, load) acquire an exclusive lock.
 *
 * Latency tracking:
 *   - A fixed-size circular buffer (deque capped to latency_window)
 *     stores the duration of each inference call in milliseconds.
 *   - avg_latency_ms is updated with an incremental running mean.
 *   - p99_latency_ms is recomputed from the sorted window on every
 *     observation (acceptable cost for latency_window ≤ 1000).
 */

#include "analytics/model_serving.h"

#include <cctype>
#include <deque>
#include <iomanip>
#include <mutex>
#include <openssl/sha.h>
#include <sstream>
#include <shared_mutex>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace themisdb {
namespace analytics {

struct ModelServingEntry {
    AutoMLModel model;
    ModelInfo info;
    ModelHealthMetrics health;

    // Latency window - protected by its own mutex so concurrent inference
    // threads can update metrics without needing the global exclusive lock.
    mutable std::mutex health_mu;
    std::deque<double> latency_buf;
};

namespace {

// ----------------------------------------------------------------------------
// Current epoch-ms helper
// ----------------------------------------------------------------------------

inline int64_t nowMs() noexcept {
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

// ----------------------------------------------------------------------------
// High-resolution wall-clock for latency measurement
// ----------------------------------------------------------------------------

inline double elapsedMs(std::chrono::steady_clock::time_point start) noexcept {
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

std::string normalizeSha256HexOrThrow(const std::string& expected_sha256_hex) {
    if (static_cast<int>(expected_sha256_hex.size()) != 64) {
        throw std::invalid_argument(
            "ModelServingEngine::loadModel expected_sha256_hex must be 64 hex characters");
    }

    std::string normalized = expected_sha256_hex;
    for (char& c : normalized) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (!std::isxdigit(uc)) {
            throw std::invalid_argument(
                "ModelServingEngine::loadModel expected_sha256_hex must be valid hexadecimal");
        }
        c = static_cast<char>(std::tolower(uc));
    }
    return normalized;
}

// ----------------------------------------------------------------------------
// Latency helpers (caller must hold entry.health_mu)
// ----------------------------------------------------------------------------

void recordLatency(ModelServingEntry &e, double ms, size_t window) {
    if (window == 0) {
        return;
    }

    e.latency_buf.push_back(ms);
    if (static_cast<int>(e.latency_buf.size()) > window) {
        e.latency_buf.pop_front();
    }

    // Running mean
    size_t n     = e.latency_buf.size();
    double delta = ms - e.health.avg_latency_ms;
    e.health.avg_latency_ms += delta / static_cast<double>(n);
    e.health.last_latency_ms = ms;

    // p99 from sorted copy of the window
    std::vector<double> sorted(e.latency_buf.begin(), e.latency_buf.end());
    std::sort(sorted.begin(), sorted.end());
    size_t idx              = static_cast<size_t>(0.99 * static_cast<double>(n - 1));
    e.health.p99_latency_ms = sorted[idx];
}

std::string sha256Hex(std::string_view input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.data()),static_cast<int>(input.size()), hash);

    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (unsigned char byte : hash) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

} // anonymous namespace

// ============================================================================
// Impl
// ============================================================================

struct ModelServingEngine::Impl {
    ModelServingConfig config;
    mutable std::shared_mutex mu;
    std::unordered_map<std::string, std::shared_ptr<ModelServingEntry>> registry;

    explicit Impl(ModelServingConfig cfg) : config(std::move(cfg)) {}
};

// ============================================================================
// ModelServingEngine – construction / destruction
// ============================================================================

ModelServingEngine::ModelServingEngine(ModelServingConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {}

ModelServingEngine::~ModelServingEngine() = default;

// ============================================================================
// lookupEntryOrThrow_ / lookupEntryOrNull_  (private helpers)
//
// Both helpers capture a reference-counted handle to the Entry under a brief
// shared_lock and release the lock immediately.  This lets callers run
// inference and metrics updates outside the registry lock, so that concurrent
// register/unregister calls are never starved by long-running inference.
// ============================================================================

std::shared_ptr<ModelServingEntry> ModelServingEngine::lookupEntryOrThrow_(const std::string &name,
                                                                           const std::string &version) const {
    std::shared_lock lock(impl_->mu);
    const std::string key = makeModelKey(name, version);
    auto it               = impl_->registry.find(key);
    if (it == impl_->registry.end()) {
        throw std::out_of_range("ModelServingEngine: model not found: " + key);
    }
    return it->second;
}

std::shared_ptr<ModelServingEntry> ModelServingEngine::lookupEntryOrNull_(const std::string &name,
                                                                          const std::string &version) const noexcept {
    std::shared_lock lock(impl_->mu);
    auto it = impl_->registry.find(makeModelKey(name, version));
    if (it == impl_->registry.end()) {
        return nullptr;
    }
    return it->second;
}

// ============================================================================
// registerModel
// ============================================================================

void ModelServingEngine::registerModel(const std::string &name, const std::string &version, AutoMLModel model) {
    if (name.empty()) {
        throw std::invalid_argument("model name must not be empty");
    }
    if (version.empty()) {
        throw std::invalid_argument("model version must not be empty");
    }

    std::unique_lock lock(impl_->mu);

    if (impl_-> static_cast<int>(registry.size()) >= impl_->config.max_models) {
        throw std::runtime_error(
            "ModelServingEngine: registry is full (max_models=" + std::to_string(impl_->config.max_models) + ")");
    }

    std::string key = makeModelKey(name, version);
    if (impl_->registry.count(key)) {
        throw std::runtime_error("ModelServingEngine: model already registered: " + key);
    }

    ModelInfo info;
    info.name             = name;
    info.version          = version;
    info.task             = model.task();
    info.algorithm        = model.algorithm();
    info.metrics          = model.metrics();
    info.registered_at_ms = nowMs();
    info.is_active        = true;

    ModelHealthMetrics health;
    health.name    = name;
    health.version = version;

    auto e    = std::make_shared<ModelServingEntry>();
    e->model  = std::move(model);
    e->info   = std::move(info);
    e->health = std::move(health);

    impl_->registry.emplace(std::move(key), std::move(e));
}

// ============================================================================
// unregisterModel
// ============================================================================

bool ModelServingEngine::unregisterModel(const std::string &name, const std::string &version) {
    std::unique_lock lock(impl_->mu);
    return impl_->registry.erase(makeModelKey(name, version)) > 0;
}

// ============================================================================
// predict (single record)
// ============================================================================

std::string ModelServingEngine::predict(const std::string &name, const std::string &version,
                                        const DataPoint &point) const {
    auto ep = lookupEntryOrThrow_(name, version);

    // Run inference outside the registry lock so that concurrent
    // registerModel() / unregisterModel() callers are not starved.
    ModelServingEntry &e = *ep;
    auto t0              = std::chrono::steady_clock::now();
    auto result          = e.model.predictOne(point);
    double ms            = elapsedMs(t0);

    // Update health metrics under the per-entry mutex only — no nested
    // lock-order dependency on impl_->mu.
    {
        std::lock_guard<std::mutex> hlock(e.health_mu);
        ++e.health.total_predictions;
        e.health.last_used_ms = nowMs();
        if (impl_->config.track_latency) {
            recordLatency(e, ms, impl_->config.latency_window);
        }
    }

    return result;
}

// ============================================================================
// predictBatch
// ============================================================================

std::vector<std::string> ModelServingEngine::predictBatch(const std::string &name, const std::string &version,
                                                          const std::vector<DataPoint> &data) const {
    if (static_cast<int>(data.size()) > impl_->config.max_batch_size) {
        throw std::invalid_argument("ModelServingEngine: batch size " + std::to_string(data.size())
                                    + " exceeds max_batch_size=" + std::to_string(impl_->config.max_batch_size));
    }

    auto ep = lookupEntryOrThrow_(name, version);

    // Run batch inference outside the registry lock.
    ModelServingEntry &e = *ep;
    auto t0              = std::chrono::steady_clock::now();
    auto results         = e.model.predict(data);
    double ms            = elapsedMs(t0);

    {
        std::lock_guard<std::mutex> hlock(e.health_mu);
        ++e.health.total_batch_calls;
        e.health.total_batch_records += data.size();
        e.health.last_used_ms = nowMs();
        if (impl_->config.track_latency) {
            recordLatency(e, ms, impl_->config.latency_window);
        }
    }

    return results;
}

// ============================================================================
// predictProba
// ============================================================================

std::vector<std::map<std::string, double>> ModelServingEngine::predictProba(const std::string &name,
                                                                            const std::string &version,
                                                                            const std::vector<DataPoint> &data) const {
    if (static_cast<int>(data.size()) > impl_->config.max_batch_size) {
        throw std::invalid_argument("ModelServingEngine: batch size " + std::to_string(data.size())
                                    + " exceeds max_batch_size=" + std::to_string(impl_->config.max_batch_size));
    }

    auto ep = lookupEntryOrThrow_(name, version);

    // Run inference outside the registry lock.
    ModelServingEntry &e = *ep;
    auto t0              = std::chrono::steady_clock::now();

    std::vector<std::map<std::string, double>> out;

    if (e.info.task == AutoMLTask::CLASSIFICATION) {
        out = e.model.predictProba(data);
    } else {
        // Regression: return a single-entry map {"value" → prediction} per point
        auto preds = e.model.predict(data);
        out.reserve(preds.size());
        for (const auto &p : preds) {
            double val = 0.0;
            try { val = std::stod(p); } catch (...) {}
            out.push_back({{"value", val}});
        }
    }

    double ms = elapsedMs(t0);
    {
        std::lock_guard<std::mutex> hlock(e.health_mu);
        ++e.health.total_batch_calls;
        e.health.total_batch_records += data.size();
        e.health.last_used_ms = nowMs();
        if (impl_->config.track_latency) {
            recordLatency(e, ms, impl_->config.latency_window);
        }
    }

    return out;
}

// ============================================================================
// listModels
// ============================================================================

std::vector<ModelInfo> ModelServingEngine::listModels() const {
    std::shared_lock lock(impl_->mu);
    std::vector<ModelInfo> out = {};

    out.reserve(impl_-> static_cast<int>(registry.size()));
    for (const auto &[k, e] : impl_->registry) {
        out.push_back(e->info);
    }
    return out;
}

// ============================================================================
// modelInfo
// ============================================================================

std::optional<ModelInfo> ModelServingEngine::modelInfo(const std::string &name, const std::string &version) const {
    std::shared_lock lock(impl_->mu);
    auto it = impl_->registry.find(makeModelKey(name, version));
    if (it == impl_->registry.end()) {
        return std::nullopt;
    }
    return it->second->info;
}

// ============================================================================
// healthMetrics
// ============================================================================

std::optional<ModelHealthMetrics> ModelServingEngine::healthMetrics(const std::string &name,
                                                                    const std::string &version) const {
    auto ep = lookupEntryOrNull_(name, version);
    if (!ep) {
        return std::nullopt;
    }

    // Take a consistent snapshot of health metrics under the per-entry lock only.
    std::lock_guard<std::mutex> hlock(ep->health_mu);
    return ep->health;
}

// ============================================================================
// isRegistered
// ============================================================================

bool ModelServingEngine::isRegistered(const std::string &name, const std::string &version) const {
    std::shared_lock lock(impl_->mu);
    return impl_->registry.count(makeModelKey(name, version)) > 0;
}

// ============================================================================
// serializeModel
// ============================================================================

std::string ModelServingEngine::serializeModel(const std::string &name, const std::string &version) const {
    // Capture a reference-counted handle so that serialization (potentially
    // non-trivial) runs outside the registry lock.
    auto ep = lookupEntryOrThrow_(name, version);
    return ep->model.serialize();
}

// ============================================================================
// loadModel
// ============================================================================
void ModelServingEngine::loadModel(const std::string &name, const std::string &version,
                                   const std::string &serialized_data) {
    if (impl_->config.require_model_integrity) {
        throw std::invalid_argument(
            "ModelServingEngine::loadModel requires explicit SHA-256 when require_model_integrity=true");
    }
    auto model = AutoMLModel::deserialize(serialized_data);
    registerModel(name, version, std::move(model));
}

void ModelServingEngine::loadModel(const std::string &name, const std::string &version,
                                   const std::string &serialized_data, const std::string &expected_sha256_hex) {
    if (expected_sha256_hex.empty()) {
        throw std::invalid_argument("ModelServingEngine::loadModel expected_sha256_hex must not be empty");
    }

    const auto normalized_expected_sha256 = normalizeSha256HexOrThrow(expected_sha256_hex);
    const auto actual_sha256 = sha256Hex(serialized_data);
    if (actual_sha256 != normalized_expected_sha256) {
        throw std::runtime_error("ModelServingEngine::loadModel integrity check failed (SHA-256 mismatch)");
    }

    auto model = AutoMLModel::deserialize(serialized_data);
    registerModel(name, version, std::move(model));
}

} // namespace analytics
} // namespace themisdb
