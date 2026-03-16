/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            model_serving.cpp                                  ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-16 04:13:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     396                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 5a7ca4018  2026-02-24  audit: remove unused headers, fix spelling, complete ROAD... ║
    • 90cdb41ff  2026-02-24  feat(analytics): implement model serving and online infer... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Model Serving and Online Inference Pipeline – Implementation
 *
 * Internal layout:
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

#include <deque>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

namespace themisdb {
namespace analytics {

namespace {

// ----------------------------------------------------------------------------
// Current epoch-ms helper
// ----------------------------------------------------------------------------

inline int64_t nowMs() noexcept {
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// ----------------------------------------------------------------------------
// High-resolution wall-clock for latency measurement
// ----------------------------------------------------------------------------

inline double elapsedMs(std::chrono::steady_clock::time_point start) noexcept {
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// ----------------------------------------------------------------------------
// Per-model runtime entry
// ----------------------------------------------------------------------------

struct Entry {
    AutoMLModel          model;
    ModelInfo            info;
    ModelHealthMetrics   health;

    // Latency window – protected by its own mutex so concurrent inference
    // threads can update metrics without needing the global exclusive lock.
    mutable std::mutex   health_mu;
    std::deque<double>   latency_buf;
};

// ----------------------------------------------------------------------------
// Latency helpers (caller must hold entry.health_mu)
// ----------------------------------------------------------------------------

void recordLatency(Entry& e, double ms, size_t window) {
    if (window == 0) return;

    e.latency_buf.push_back(ms);
    if (e.latency_buf.size() > window)
        e.latency_buf.pop_front();

    // Running mean
    size_t n     = e.latency_buf.size();
    double delta = ms - e.health.avg_latency_ms;
    e.health.avg_latency_ms += delta / static_cast<double>(n);
    e.health.last_latency_ms = ms;

    // p99 from sorted copy of the window
    std::vector<double> sorted(e.latency_buf.begin(), e.latency_buf.end());
    std::sort(sorted.begin(), sorted.end());
    size_t idx = static_cast<size_t>(0.99 * static_cast<double>(n - 1));
    e.health.p99_latency_ms = sorted[idx];
}

} // anonymous namespace

// ============================================================================
// Impl
// ============================================================================

struct ModelServingEngine::Impl {
    ModelServingConfig                                        config;
    mutable std::shared_mutex                                 mu;
    std::unordered_map<std::string, std::unique_ptr<Entry>>   registry;

    explicit Impl(ModelServingConfig cfg) : config(std::move(cfg)) {}
};

// ============================================================================
// ModelServingEngine – construction / destruction
// ============================================================================

ModelServingEngine::ModelServingEngine(ModelServingConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

ModelServingEngine::~ModelServingEngine() = default;

// ============================================================================
// registerModel
// ============================================================================

void ModelServingEngine::registerModel(const std::string& name,
                                        const std::string& version,
                                        AutoMLModel        model) {
    if (name.empty())    throw std::invalid_argument("model name must not be empty");
    if (version.empty()) throw std::invalid_argument("model version must not be empty");

    std::unique_lock lock(impl_->mu);

    if (impl_->registry.size() >= impl_->config.max_models)
        throw std::runtime_error(
            "ModelServingEngine: registry is full (max_models=" +
            std::to_string(impl_->config.max_models) + ")");

    std::string key = makeModelKey(name, version);
    if (impl_->registry.count(key))
        throw std::runtime_error(
            "ModelServingEngine: model already registered: " + key);

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

    auto e = std::make_unique<Entry>();
    e->model  = std::move(model);
    e->info   = std::move(info);
    e->health = std::move(health);

    impl_->registry.emplace(std::move(key), std::move(e));
}

// ============================================================================
// unregisterModel
// ============================================================================

bool ModelServingEngine::unregisterModel(const std::string& name,
                                          const std::string& version) {
    std::unique_lock lock(impl_->mu);
    return impl_->registry.erase(makeModelKey(name, version)) > 0;
}

// ============================================================================
// predict (single record)
// ============================================================================

std::string ModelServingEngine::predict(const std::string& name,
                                         const std::string& version,
                                         const DataPoint&   point) const {
    std::shared_lock lock(impl_->mu);

    auto it = impl_->registry.find(makeModelKey(name, version));
    if (it == impl_->registry.end())
        throw std::out_of_range("ModelServingEngine: model not found: " +
                                makeModelKey(name, version));

    Entry& e = *it->second;
    auto   t0 = std::chrono::steady_clock::now();
    auto   result = e.model.predictOne(point);
    double ms = elapsedMs(t0);

    // Update health metrics under the per-entry mutex so concurrent
    // inference threads do not race on the counters / latency buffer.
    {
        std::lock_guard<std::mutex> hlock(e.health_mu);
        ++e.health.total_predictions;
        e.health.last_used_ms = nowMs();
        if (impl_->config.track_latency)
            recordLatency(e, ms, impl_->config.latency_window);
    }

    return result;
}

// ============================================================================
// predictBatch
// ============================================================================

std::vector<std::string> ModelServingEngine::predictBatch(
    const std::string&            name,
    const std::string&            version,
    const std::vector<DataPoint>& data) const {

    if (data.size() > impl_->config.max_batch_size)
        throw std::invalid_argument(
            "ModelServingEngine: batch size " + std::to_string(data.size()) +
            " exceeds max_batch_size=" +
            std::to_string(impl_->config.max_batch_size));

    std::shared_lock lock(impl_->mu);

    auto it = impl_->registry.find(makeModelKey(name, version));
    if (it == impl_->registry.end())
        throw std::out_of_range("ModelServingEngine: model not found: " +
                                makeModelKey(name, version));

    Entry& e  = *it->second;
    auto   t0 = std::chrono::steady_clock::now();
    auto   results = e.model.predict(data);
    double ms = elapsedMs(t0);

    {
        std::lock_guard<std::mutex> hlock(e.health_mu);
        ++e.health.total_batch_calls;
        e.health.total_batch_records += data.size();
        e.health.last_used_ms = nowMs();
        if (impl_->config.track_latency)
            recordLatency(e, ms, impl_->config.latency_window);
    }

    return results;
}

// ============================================================================
// predictProba
// ============================================================================

std::vector<std::map<std::string, double>> ModelServingEngine::predictProba(
    const std::string&            name,
    const std::string&            version,
    const std::vector<DataPoint>& data) const {

    if (data.size() > impl_->config.max_batch_size)
        throw std::invalid_argument(
            "ModelServingEngine: batch size " + std::to_string(data.size()) +
            " exceeds max_batch_size=" +
            std::to_string(impl_->config.max_batch_size));

    std::shared_lock lock(impl_->mu);

    auto it = impl_->registry.find(makeModelKey(name, version));
    if (it == impl_->registry.end())
        throw std::out_of_range("ModelServingEngine: model not found: " +
                                makeModelKey(name, version));

    Entry& e  = *it->second;
    auto   t0 = std::chrono::steady_clock::now();

    std::vector<std::map<std::string, double>> out;

    if (e.info.task == AutoMLTask::CLASSIFICATION) {
        out = e.model.predictProba(data);
    } else {
        // Regression: return a single-entry map {"value" → prediction} per point
        auto preds = e.model.predict(data);
        out.reserve(preds.size());
        for (const auto& p : preds) {
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
        if (impl_->config.track_latency)
            recordLatency(e, ms, impl_->config.latency_window);
    }

    return out;
}

// ============================================================================
// listModels
// ============================================================================

std::vector<ModelInfo> ModelServingEngine::listModels() const {
    std::shared_lock lock(impl_->mu);
    std::vector<ModelInfo> out;
    out.reserve(impl_->registry.size());
    for (const auto& [k, e] : impl_->registry)
        out.push_back(e->info);
    return out;
}

// ============================================================================
// modelInfo
// ============================================================================

std::optional<ModelInfo> ModelServingEngine::modelInfo(
    const std::string& name,
    const std::string& version) const {

    std::shared_lock lock(impl_->mu);
    auto it = impl_->registry.find(makeModelKey(name, version));
    if (it == impl_->registry.end()) return std::nullopt;
    return it->second->info;
}

// ============================================================================
// healthMetrics
// ============================================================================

std::optional<ModelHealthMetrics> ModelServingEngine::healthMetrics(
    const std::string& name,
    const std::string& version) const {

    std::shared_lock lock(impl_->mu);
    auto it = impl_->registry.find(makeModelKey(name, version));
    if (it == impl_->registry.end()) return std::nullopt;
    // Take a consistent snapshot of health metrics under the per-entry lock.
    std::lock_guard<std::mutex> hlock(it->second->health_mu);
    return it->second->health;
}

// ============================================================================
// isRegistered
// ============================================================================

bool ModelServingEngine::isRegistered(const std::string& name,
                                       const std::string& version) const {
    std::shared_lock lock(impl_->mu);
    return impl_->registry.count(makeModelKey(name, version)) > 0;
}

// ============================================================================
// serializeModel
// ============================================================================

std::string ModelServingEngine::serializeModel(const std::string& name,
                                                const std::string& version) const {
    std::shared_lock lock(impl_->mu);
    auto it = impl_->registry.find(makeModelKey(name, version));
    if (it == impl_->registry.end())
        throw std::out_of_range("ModelServingEngine: model not found: " +
                                makeModelKey(name, version));
    return it->second->model.serialize();
}

// ============================================================================
// loadModel
// ============================================================================

void ModelServingEngine::loadModel(const std::string& name,
                                    const std::string& version,
                                    const std::string& serialized_data) {
    auto model = AutoMLModel::deserialize(serialized_data);
    registerModel(name, version, std::move(model));
}

} // namespace analytics
} // namespace themisdb
