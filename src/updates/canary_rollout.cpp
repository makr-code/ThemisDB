/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            canary_rollout.cpp                                 ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
             config_.version, config_.node_id, config_.stages.size());
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

bool CanaryRollout::isNodeInStage(size_t stage_index) const {
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
    std::string rid;
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
    std::string reason;

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
        current_stage_ < config_.stages.size()
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

void CanaryRollout::setStageCompleteCallback(StageCompleteCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    stage_complete_cb_ = std::move(cb);
}

void CanaryRollout::setRollbackCallback(RollbackCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    rollback_cb_ = std::move(cb);
}

} // namespace updates
} // namespace themis
