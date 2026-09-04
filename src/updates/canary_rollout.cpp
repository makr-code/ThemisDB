/**
 * @file canary_rollout.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/canary_rollout.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <functional>
#include <stdexcept>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// CanaryConfig helpers
// ---------------------------------------------------------------------------

CanaryConfig CanaryConfig::withDefaultStages(const std::string& version,
                                              const std::string& node_id) {
    CanaryConfig cfg;
    cfg.version = version;
    cfg.node_id = node_id;
    cfg.error_rate_threshold = 0.05;
    cfg.min_sample_count = 20;
    cfg.stages = {
        {0.01, std::chrono::seconds{3600}},   //  1 % – 1 hour observation
        {0.05, std::chrono::seconds{7200}},   //  5 % – 2 hours
        {0.25, std::chrono::seconds{21600}},  // 25 % – 6 hours
        {1.00, std::chrono::seconds{0}},      //100 % – no observation (final)
    };
    return cfg;
}

// ---------------------------------------------------------------------------
// CanaryRollout – construction
// ---------------------------------------------------------------------------

CanaryRollout::CanaryRollout(std::shared_ptr<HotReloadEngine> engine,
                             const CanaryConfig& config)
    : engine_(std::move(engine))
    , config_(config) {

    if (!engine_) {
        throw std::invalid_argument("CanaryRollout: engine must not be null");
    }
    if (config_.stages.empty()) {
        throw std::invalid_argument("CanaryRollout: stages must not be empty");
    }
    if (config_.node_id.empty()) {
        throw std::invalid_argument("CanaryRollout: node_id must not be empty");
    }
    if (config_.version.empty()) {
        throw std::invalid_argument("CanaryRollout: version must not be empty");
    }

    LOG_INFO("CanaryRollout initialised: version={} node={} stages={}",
             config_.version, config_.node_id,static_cast<int>(config_.stages.size()));
}

// ---------------------------------------------------------------------------
// Node membership
// ---------------------------------------------------------------------------

double CanaryRollout::computeNodeHash() const {
    // Deterministic hash in [0, 1) based on version + node_id.
    // Using std::hash<std::string> gives the same result on all nodes for the
    // same inputs – no inter-node communication required.
    const std::string key = config_.version + ":" + config_.node_id;
    const std::size_t h = std::hash<std::string>{}(key);
    // Map to (0, 1] – avoid exact 0 so 0% always excludes the node.
    return static_cast<double>(h % 100000u + 1u) / 100000.0;
}

bool CanaryRollout::isNodeInStage([[maybe_unused]] size_t stage_index) const {
    if (stage_index >= config_.stages.size()) {
        return false;
    }
    const double node_hash = computeNodeHash();
    return node_hash <= config_.stages[stage_index].percentage;
}

bool CanaryRollout::isNodeInCurrentStage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isNodeInStage(current_stage_);
}

// ---------------------------------------------------------------------------
// Stage management
// ---------------------------------------------------------------------------

size_t CanaryRollout::currentStage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_stage_;
}

bool CanaryRollout::advanceStage() {
    StageCompleteCallback cb;
    double pct = 0.0;
    size_t completed_stage = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_rolled_back_ || is_complete_) {
            return false;
        }
        if (current_stage_ + 1 >= config_.stages.size()) {
            is_complete_ = true;
            LOG_INFO("CanaryRollout: all stages complete for version {}",
                     config_.version);
            return false;
        }

        completed_stage = current_stage_;
        pct = config_.stages[current_stage_].percentage;

        current_stage_++;
        success_count_ = 0;
        error_count_ = 0;

        cb = stage_complete_cb_;
    }

    LOG_INFO("CanaryRollout: advanced to stage {} ({:.0f}% of nodes), version {}",
             completed_stage + 1,
             config_.stages[completed_stage + 1].percentage * 100.0,
             config_.version);

    if (cb) {
        try {
            cb(completed_stage, pct);
        } catch (const std::exception& e) {
            LOG_WARN("CanaryRollout: stage-complete callback threw: {}", e.what());
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Update application
// ---------------------------------------------------------------------------

ReloadResult CanaryRollout::applyIfIncluded() {
    ReloadResult skipped;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (is_rolled_back_) {
            skipped.error_message = "Canary rollout was rolled back";
            return skipped;
        }
        if (is_complete_) {
            skipped.error_message = "Canary rollout already complete";
            return skipped;
        }
        if (is_applied_) {
            // Idempotency guard: return the already-stored rollback_id so the
            // caller can still use it for manual rollback if needed.
            skipped.success = true;
            skipped.rollback_id = rollback_id_;
            skipped.error_message = "Update already applied on this node";
            LOG_DEBUG("CanaryRollout: apply called again – already applied (version {})",
                      config_.version);
            return skipped;
        }
        if (!isNodeInStage(current_stage_)) {
            skipped.error_message =
                "Node not included in canary stage " +
                std::to_string(current_stage_) + " (" +
                std::to_string(
                    static_cast<int>(config_.stages[current_stage_].percentage * 100)) +
                "% rollout)";
            LOG_DEBUG("CanaryRollout: {}", skipped.error_message);
            return skipped;
        }
    }

    LOG_INFO("CanaryRollout: applying version {} on node {}",
             config_.version, config_.node_id);

    ReloadResult result = engine_->applyHotReload(config_.version);

    if (result.success) {
        std::lock_guard<std::mutex> lock(mutex_);
        rollback_id_ = result.rollback_id;
        is_applied_ = true;
        LOG_INFO("CanaryRollout: update applied; rollback_id={}", rollback_id_);
    } else {
        LOG_ERROR("CanaryRollout: applyHotReload failed: {}", result.error_message);
    }

    return result;
}

bool CanaryRollout::rollback(const std::string& reason) {
    std::string rid = {};
    RollbackCallback cb;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (is_rolled_back_) {
            LOG_WARN("CanaryRollout: already rolled back");
            return false;
        }

        rid = rollback_id_;
        rollback_reason_ = reason.empty() ? "manual rollback" : reason;
        is_rolled_back_ = true;
        cb = rollback_cb_;
    }

    LOG_WARN("CanaryRollout: rolling back version {} – reason: {}",
             config_.version, reason);

    bool ok = true;
    if (!rid.empty()) {
        ok = engine_->rollback(rid);
    } else {
        LOG_WARN("CanaryRollout: no rollback_id available; update may not have been applied");
    }

    if (cb) {
        try {
            cb(rollback_reason_);
        } catch (const std::exception& e) {
            LOG_WARN("CanaryRollout: rollback callback threw: {}", e.what());
        }
    }

    return ok;
}

// ---------------------------------------------------------------------------
// Health tracking
// ---------------------------------------------------------------------------

void CanaryRollout::reportSuccess() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++success_count_;
    }
}

void CanaryRollout::reportError() {
    bool trigger_rollback = false;
    std::string reason = {};

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++error_count_;

        if (!is_rolled_back_ && !is_complete_) {
            const size_t total = success_count_ + error_count_;
            if (total >= config_.min_sample_count) {
                const double rate =
                    static_cast<double>(error_count_) / static_cast<double>(total);
                if (rate > config_.error_rate_threshold) {
                    trigger_rollback = true;
                    reason = "error rate " +
                             std::to_string(static_cast<int>(rate * 100)) +
                             "% exceeds threshold " +
                             std::to_string(static_cast<int>(
                                 config_.error_rate_threshold * 100)) +
                             "%";
                }
            }
        }
    }

    if (trigger_rollback) {
        LOG_WARN("CanaryRollout: auto-rollback triggered – {}", reason);
        rollback(reason);
    }
}

double CanaryRollout::errorRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t total = success_count_ + error_count_;
    if (total == 0) {
        return 0.0;
    }
    return static_cast<double>(error_count_) / static_cast<double>(total);
}

bool CanaryRollout::shouldRollback() const {
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t total = success_count_ + error_count_;
    if (total < config_.min_sample_count) {
        return false;
    }
    const double rate =
        static_cast<double>(error_count_) / static_cast<double>(total);
    return rate > config_.error_rate_threshold;
}

// ---------------------------------------------------------------------------
// Status & callbacks
// ---------------------------------------------------------------------------

CanaryStatus CanaryRollout::status() const {
    std::lock_guard<std::mutex> lock(mutex_);

    CanaryStatus s;
    s.current_stage = current_stage_;
    s.total_stages = config_.stages.size();
    s.current_percentage =
        current_stage_ <static_cast<int>(config_.stages.size())
            ? config_.stages[current_stage_].percentage
            : 1.0;
    s.this_node_included = isNodeInStage(current_stage_);
    s.is_complete = is_complete_;
    s.is_rolled_back = is_rolled_back_;
    s.version = config_.version;
    s.rollback_reason = rollback_reason_;
    s.rollback_id = rollback_id_;

    const size_t total = success_count_ + error_count_;
    s.sample_count = total;
    s.observed_error_rate =
        total > 0
            ? static_cast<double>(error_count_) / static_cast<double>(total)
            : 0.0;

    return s;
}

void CanaryRollout::setStageCompleteCallback([[maybe_unused]] StageCompleteCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    stage_complete_cb_ = std::move(cb);
}

void CanaryRollout::setRollbackCallback([[maybe_unused]] RollbackCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    rollback_cb_ = std::move(cb);
}

// ===========================================================================
// CanaryDeployment
// ===========================================================================

CanaryDeployment::CanaryDeployment() = default;

CanaryDeployment::CanaryDeployment(CanaryDeployment&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    version_ = std::move(other.version_);
    node_id_ = std::move(other.node_id_);
    stages_ = std::move(other.stages_);
    error_rate_threshold_ = other.error_rate_threshold_;
    latency_threshold_us_ = other.latency_threshold_us_;
    engine_ = std::move(other.engine_);
    rollout_ = std::move(other.rollout_);
    stage_complete_cb_ = std::move(other.stage_complete_cb_);
    rollback_cb_ = std::move(other.rollback_cb_);
    latency_samples_us_ = std::move(other.latency_samples_us_);
    memory_bytes_ = other.memory_bytes_;
    cpu_fraction_ = other.cpu_fraction_;
    disk_io_bytes_per_sec_ = other.disk_io_bytes_per_sec_;
    custom_metrics_ = std::move(other.custom_metrics_);
    ab_testing_enabled_ = other.ab_testing_enabled_;
    ab_config_ = std::move(other.ab_config_);
}

CanaryDeployment& CanaryDeployment::operator=(CanaryDeployment&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    std::scoped_lock lock(mutex_, other.mutex_);
    version_ = std::move(other.version_);
    node_id_ = std::move(other.node_id_);
    stages_ = std::move(other.stages_);
    error_rate_threshold_ = other.error_rate_threshold_;
    latency_threshold_us_ = other.latency_threshold_us_;
    engine_ = std::move(other.engine_);
    rollout_ = std::move(other.rollout_);
    stage_complete_cb_ = std::move(other.stage_complete_cb_);
    rollback_cb_ = std::move(other.rollback_cb_);
    latency_samples_us_ = std::move(other.latency_samples_us_);
    memory_bytes_ = other.memory_bytes_;
    cpu_fraction_ = other.cpu_fraction_;
    disk_io_bytes_per_sec_ = other.disk_io_bytes_per_sec_;
    custom_metrics_ = std::move(other.custom_metrics_);
    ab_testing_enabled_ = other.ab_testing_enabled_;
    ab_config_ = std::move(other.ab_config_);

    return *this;
}

// ---------------------------------------------------------------------------
// Builder API
// ---------------------------------------------------------------------------

void CanaryDeployment::setVersion(const std::string& version) {
    std::lock_guard<std::mutex> lock(mutex_);
    version_ = version;
}

void CanaryDeployment::setStages(std::vector<CanaryDeploymentStage> stages) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (size_t i = 0; i <static_cast<int>(stages.size()); ++i) {
        stages[i].stage_number = i;
    }
    stages_ = std::move(stages);
}

void CanaryDeployment::setErrorRateThreshold([[maybe_unused]] double threshold) {
    std::lock_guard<std::mutex> lock(mutex_);
    error_rate_threshold_ = threshold;
}

void CanaryDeployment::setLatencyThreshold(std::chrono::milliseconds p99_limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    latency_threshold_us_ =
        std::chrono::duration_cast<std::chrono::microseconds>(p99_limit);
}

void CanaryDeployment::setEngine(std::shared_ptr<HotReloadEngine> engine) {
    std::lock_guard<std::mutex> lock(mutex_);
    engine_ = std::move(engine);
}

void CanaryDeployment::setNodeId(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    node_id_ = node_id;
}

// ---------------------------------------------------------------------------
// Deployment
// ---------------------------------------------------------------------------

ReloadResult CanaryDeployment::deploy() {
    std::unique_ptr<CanaryRollout> rollout;
    std::vector<CanaryDeploymentStage> stages_copy;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (version_.empty()) {
            throw std::invalid_argument("CanaryDeployment: version must be set before deploy()");
        }
        if (node_id_.empty()) {
            throw std::invalid_argument("CanaryDeployment: node_id must be set before deploy()");
        }
        if (!engine_) {
            throw std::invalid_argument("CanaryDeployment: engine must be set before deploy()");
        }
        if (stages_.empty()) {
            throw std::invalid_argument("CanaryDeployment: stages must be set before deploy()");
        }

        // Convert CanaryDeploymentStage → CanaryConfig::stages
        CanaryConfig cfg;
        cfg.version = version_;
        cfg.node_id = node_id_;
        cfg.error_rate_threshold = error_rate_threshold_;
        cfg.min_sample_count = 20;
        for (const auto& s : stages_) {
            CanaryStage cs;
            cs.percentage = static_cast<double>(s.percentage) / 100.0;
            cs.observation_duration = s.duration;
            cfg.stages.push_back(cs);
        }

        auto new_rollout = std::make_unique<CanaryRollout>(engine_, cfg);
        stages_copy = stages_;

        // Wire stage-complete callback: always reads the current CanaryDeployment
        // callback so callbacks registered after deploy() also fire.
        // Note: CanaryRollout calls these callbacks OUTSIDE its own lock, so it
        // is safe to acquire mutex_ here.
        new_rollout->setStageCompleteCallback(
            [this](size_t completed_stage, double /*pct*/) {
                StageCompleteCallback cb;
                CanaryDeploymentStage stage_info;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    cb = stage_complete_cb_;
                    if (static_cast<int>(stages_.size()) > completed_stage) {
                        stage_info = stages_[completed_stage];
                    }
                }
                if (cb) {
                   try { 
                       cb(stage_info); 
                   } catch (...) {
                       // Error Code: 7488 - Never let stage callbacks crash the rollout
                       // Log and silently ignore to ensure deployment continuity
                       LOG_WARN([[maybe_unused]] "CanaryRollout: stage complete callback threw exception; silently caught");
                   }
               }
           });

        // Wire rollback callback: same dynamic-read approach.
        new_rollout->setRollbackCallback(
            [this](const std::string& reason) {
                RollbackCallback cb;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    cb = rollback_cb_;
                }
                if (cb) {
                    try { 
                        cb(reason); 
                    } catch (...) {
                        // Error Code: 7489 - Never let rollback callbacks crash the rollout
                        // Log and silently ignore to ensure rollout can proceed
                        LOG_WARN([[maybe_unused]] "CanaryRollout: rollback callback threw exception; silently caught");
                    }
                }
            });

        rollout_.reset();
        rollout = std::move(new_rollout);
    }

    // Apply outside the lock so callbacks can call back into this object.
    ReloadResult result = rollout->applyIfIncluded();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        rollout_ = std::move(rollout);
    }

    return result;
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

void CanaryDeployment::onStageComplete([[maybe_unused]] StageCompleteCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    stage_complete_cb_ = std::move(cb);
}

void CanaryDeployment::onRollback([[maybe_unused]] RollbackCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    rollback_cb_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// Health / metric reporting
// ---------------------------------------------------------------------------

void CanaryDeployment::reportSuccess() {
    // Read rollout pointer without holding the lock during the call, since
    // CanaryRollout callbacks acquire mutex_ (deadlock risk if held here).
    CanaryRollout* r = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        r = rollout_.get();
    }
    if (r) {
      r->reportSuccess();
    }
}

void CanaryDeployment::reportError() {
    // Same rationale: release mutex_ before calling into CanaryRollout, which
    // may auto-rollback and invoke a callback that acquires mutex_.
    CanaryRollout* r = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        r = rollout_.get();
    }
    if (r) {
      r->reportError();
    }
}

void CanaryDeployment::reportLatency(std::chrono::microseconds latency) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (static_cast<int>(latency_samples_us_.size()) > = kMaxLatencySamples) {
            latency_samples_us_.pop_front();  // O(1) for deque
        }
        latency_samples_us_.push_back(latency.count());
    }
    // Check threshold outside the lock.
    checkLatencyThreshold();
}

void CanaryDeployment::reportMemoryUsage([[maybe_unused]] double bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    memory_bytes_ = bytes;
}

void CanaryDeployment::reportCpuUsage([[maybe_unused]] double fraction) {
    std::lock_guard<std::mutex> lock(mutex_);
    cpu_fraction_ = fraction;
}

void CanaryDeployment::reportDiskIO([[maybe_unused]] double bytes_per_sec) {
    std::lock_guard<std::mutex> lock(mutex_);
    disk_io_bytes_per_sec_ = bytes_per_sec;
}

void CanaryDeployment::recordCustomMetric(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    custom_metrics_[name] = value;
}

// ---------------------------------------------------------------------------
// A/B testing and traffic splitting
// ---------------------------------------------------------------------------

void CanaryDeployment::enableABTesting(const ABTestConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    ab_testing_enabled_ = true;
    ab_config_ = config;
}

bool CanaryDeployment::isCanaryRequest(const std::string& request_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ab_testing_enabled_) {
        return false;
    }
    const std::string key = request_id + ab_config_.experiment_id;
    const std::size_t h = std::hash<std::string>{}(key);
    const double frac = static_cast<double>(h % 100000u) / 100000.0;
    return frac < ab_config_.canary_fraction;
}

bool CanaryDeployment::isControlRequest(const std::string& request_id) const {
    return !isCanaryRequest(request_id);
}

bool CanaryDeployment::isNodeInCanaryGroup() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!rollout_) {
        return false;
    }
    return rollout_->isNodeInCurrentStage();
}

// ---------------------------------------------------------------------------
// Status and metrics
// ---------------------------------------------------------------------------

LatencyStats CanaryDeployment::computeLatencyStats() const {
    // Caller must hold mutex_.
    LatencyStats stats = {};
    if (latency_samples_us_.empty()) {
        return stats;
    }

    // Sort a copy of the deque to compute percentiles.
    std::vector<int64_t> sorted(latency_samples_us_.begin(), latency_samples_us_.end());
    std::sort(sorted.begin(), sorted.end());

    stats.sample_count = sorted.size();
    const size_t n = sorted.size();

    auto percentile = [&]([[maybe_unused]] double p) -> std::chrono::microseconds {
        // Nearest-rank method: index = ceil(p/100 * n) - 1 (0-based).
        // Guard against underflow from size_t subtraction: ensure n >= 1 (checked above).
        const auto rank = static_cast<size_t>(
            std::ceil(p / 100.0 * static_cast<double>(n)));
        // rank is in [1, n]; convert to 0-based and clamp.
        const size_t idx = (rank > 0 ? rank - 1 : 0);
        const size_t clamped = std::min(idx, n - 1);
        return std::chrono::microseconds{sorted[clamped]};
    };

    stats.p50 = percentile(50.0);
    stats.p95 = percentile(95.0);
    stats.p99 = percentile(99.0);
    return stats;
}

CanaryMetricsSnapshot CanaryDeployment::getMetricsSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    CanaryMetricsSnapshot snap;
    snap.latency = computeLatencyStats();
    snap.memory_bytes = memory_bytes_;
    snap.cpu_fraction = cpu_fraction_;
    snap.disk_io_bytes_per_sec = disk_io_bytes_per_sec_;
    snap.custom_metrics = custom_metrics_;

    if (rollout_) {
        auto s = rollout_->status();
        snap.error_rate    = s.observed_error_rate;
        snap.error_count   = static_cast<size_t>(
            static_cast<double>(s.sample_count) * s.observed_error_rate + 0.5);
        snap.success_count = s.sample_count > snap.error_count
                             ? s.sample_count - snap.error_count
                             : 0;
    }

    if (latency_threshold_us_.count() > 0 && snap.latency.sample_count > 0) {
        snap.latency_threshold_exceeded =
            snap.latency.p99 > latency_threshold_us_;
    }

    return snap;
}

CanaryStatus CanaryDeployment::status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!rollout_) {
        CanaryStatus s;
        s.version = version_;
        return s;
    }
    return rollout_->status();
}

bool CanaryDeployment::advanceStage() {
    CanaryRollout* r = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        r = rollout_.get();
    }
    if (!r) {
      return false;
    }
    return r->advanceStage();
}

bool CanaryDeployment::rollback(const std::string& reason) {
    CanaryRollout* r = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        r = rollout_.get();
    }
    if (!r) {
      return false;
    }
    return r->rollback(reason);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void CanaryDeployment::checkLatencyThreshold() {
    LatencyStats stats;
    std::chrono::microseconds threshold{0};
    bool should_rollback = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (latency_threshold_us_.count() == 0 || !rollout_) {
            return;
        }
        stats = computeLatencyStats();
        threshold = latency_threshold_us_;
        if (stats.sample_count > 0 && stats.p99 > threshold) {
            should_rollback = !rollout_->status().is_rolled_back &&
                              !rollout_->status().is_complete;
        }
    }

    if (should_rollback) {
        const std::string reason =
            "p99 latency " +
            std::to_string(stats.p99.count()) +
            "us exceeds threshold " +
            std::to_string(threshold.count()) + "us";
        LOG_WARN("CanaryDeployment: auto-rollback on latency – {}", reason);
        rollback(reason);
    }
}

} // namespace updates
} // namespace themis


