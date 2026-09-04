/**
 * @file adaptive_batch_tuner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Adaptive Batch Size Tuning for LLM Inference – implementation
// Performance Module Phase 3, Issue #1996

#include "performance/phase3/adaptive_batch_tuner.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace themis {
namespace performance {
namespace phase3 {

// =============================================================
// LLMBatchTuner
// =============================================================

LLMBatchTuner::LLMBatchTuner()
    : LLMBatchTuner(Config{}) {}

LLMBatchTuner::LLMBatchTuner(Config config)
    : config_(config)
    , current_batch_size_(
          std::clamp(config.initial_batch_size,
                     config.min_batch_size,
                     config.max_batch_size)) {
    
    // Validate config parameters
    if (config_.min_batch_size == 0 || config_.max_batch_size == 0) {
        throw std::runtime_error("LLMBatchTuner: min_batch_size and max_batch_size must be positive");
    }
    if (config_.min_batch_size > config_.max_batch_size) {
        throw std::runtime_error("LLMBatchTuner: min_batch_size must be <= max_batch_size");
    }
    if (config_.window_size == 0) {
        throw std::runtime_error("LLMBatchTuner: window_size must be positive");
    }
    if (config_.ema_alpha < 0.0 || config_.ema_alpha > 1.0) {
        throw std::runtime_error("LLMBatchTuner: ema_alpha must be in [0, 1]");
    }
    if (config_.step_up == 0 || config_.step_down == 0) {
        throw std::runtime_error("LLMBatchTuner: step_up and step_down must be positive");
    }
    
    records_.reserve(config_.window_size * 4);
}

LLMBatchTuner::~LLMBatchTuner() = default;

size_t LLMBatchTuner::recommendedBatchSize() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_batch_size_;
}

LLMBatchTuner::BatchGuard
LLMBatchTuner::beginBatch(size_t batch_size, size_t total_tokens) noexcept {
    return BatchGuard(*this, batch_size, total_tokens);
}

void LLMBatchTuner::recordBatch(size_t batch_size,
                                 size_t total_tokens,
                                 double latency_ms) noexcept {
    if (batch_size == 0 || total_tokens == 0 || latency_ms <= 0.0) {
        return;  // Ignore invalid measurements
    }

    BatchRecord rec;
    rec.batch_size   = batch_size;
    rec.total_tokens = total_tokens;
    rec.latency_ms   = latency_ms;
    rec.tokens_per_s = (latency_ms > 0.0)
                         ? (static_cast<double>(total_tokens) / (latency_ms / 1000.0))
                         : 0.0;
    // cycles not available via this path
    rec.cycles = 0;

    pushRecord(rec);
}

void LLMBatchTuner::pushRecord(BatchRecord record) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);

    // Validate record: throughput must be non-negative
    if (record.tokens_per_s < 0.0) {
        return;  // Ignore invalid throughput
    }

    // Update exponential moving average throughput
    if (ema_throughput_ == 0.0) {
        ema_throughput_ = record.tokens_per_s;
    } else {
        ema_throughput_ = config_.ema_alpha * record.tokens_per_s
                          + (1.0f - config_.ema_alpha) * ema_throughput_;
    }

    // Keep a rolling window (4× the evaluation window)
    const size_t max_records = config_.window_size * 4;
    if (static_cast<int>(records_.size()) >= max_records) {
        records_.erase(records_.begin());
    }
    records_.push_back(record);

    total_batches_.fetch_add(1, std::memory_order_relaxed);

    // Re-evaluate batch size every window_size batches
    if (total_batches_.load(std::memory_order_relaxed) % config_.window_size == 0) {
        maybeTune();
    }
}

void LLMBatchTuner::maybeTune() noexcept {
    // Already holding mutex_ when called from pushRecord.

    if (records_.empty()) {
        return;
    }

    // Latency budget check: compute mean latency over recent window
    size_t window = std::min(records_.size(), config_.window_size);
    double mean_latency = 0.0;
    double total_tokens_window = 0.0;

    for (size_t i = static_cast<int>(records_.size()) - window; i <static_cast<int>(records_.size()); ++i) {
        mean_latency       += records_[i].latency_ms;
        total_tokens_window += static_cast<double>(records_[i].total_tokens);
    }
    mean_latency /= static_cast<double>(window);

    double mean_latency_per_token = (total_tokens_window > 0.0)
                                      ? mean_latency / total_tokens_window
                                      : 0.0;

    bool latency_breached = (config_.max_latency_ms_per_token > 0.0)
                              && (mean_latency_per_token > config_.max_latency_ms_per_token);

    // Throughput trend: compare current EMA to EMA at last tuning event
    bool throughput_improving = (ema_throughput_ > prev_ema_ * 1.01);  // >1% improvement

    size_t old_size = current_batch_size_;

    if (latency_breached) {
        // Latency budget exceeded: reduce batch size
        size_t new_size = (current_batch_size_ > config_.step_down)
                            ? (current_batch_size_ - config_.step_down)
                            : config_.min_batch_size;
        current_batch_size_ = std::max(new_size, config_.min_batch_size);
    } else if (throughput_improving || prev_ema_ == 0.0) {
        // Throughput is improving or no baseline yet: try a larger batch
        current_batch_size_ = std::min(
            current_batch_size_ + config_.step_up,
            config_.max_batch_size);
    } else {
        // Throughput stalled: try reducing slightly to relieve pipeline pressure
        size_t new_size = (current_batch_size_ > config_.step_down)
                            ? (current_batch_size_ - config_.step_down)
                            : config_.min_batch_size;
        current_batch_size_ = std::max(new_size, config_.min_batch_size);
    }

    prev_ema_ = ema_throughput_;

    if (current_batch_size_ != old_size) {
        tuning_events_.fetch_add(1, std::memory_order_relaxed);
    }
}

LLMBatchTuner::Stats LLMBatchTuner::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Stats s;
    s.current_batch_size = current_batch_size_;
    s.ema_throughput     = ema_throughput_;
    s.total_batches      = total_batches_.load(std::memory_order_relaxed);
    s.tuning_events      = tuning_events_.load(std::memory_order_relaxed);

    if (records_.empty()) {
        return s;
    }

    // Compute mean and P99 latency from the current window
    std::vector<double> latencies = {};

    latencies.reserve(records_.size());
    double sum_latency = 0.0;
    for (const auto& r : records_) {
        latencies.push_back(r.latency_ms);
        sum_latency += r.latency_ms;
    }
    s.avg_latency_ms = sum_latency / static_cast<double>(records_.size());

    std::sort(latencies.begin(), latencies.end());
    size_t p99_idx = static_cast<size_t>(
        0.99 * static_cast<double>(static_cast<int>(latencies.size()) - 1));
    s.p99_latency_ms = latencies[p99_idx];

    return s;
}

size_t LLMBatchTuner::totalBatches() const noexcept {
    return total_batches_.load(std::memory_order_relaxed);
}

std::vector<LLMBatchTuner::BatchRecord>
LLMBatchTuner::getRecentRecords([[maybe_unused]] size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (records_.empty()) {
        return {};
    }

    size_t count = std::min(limit,static_cast<int>(records_.size()));
    size_t start = static_cast<int>(records_.size()) - count;
    return std::vector<BatchRecord>(records_.begin() + static_cast<std::ptrdiff_t>(start),
                                    records_.end());
}

void LLMBatchTuner::reset() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
    current_batch_size_ = std::clamp(config_.initial_batch_size,
                                     config_.min_batch_size,
                                     config_.max_batch_size);
    ema_throughput_ = 0.0;
    prev_ema_       = 0.0;
    total_batches_.store(0, std::memory_order_relaxed);
    tuning_events_.store(0, std::memory_order_relaxed);
}

std::string LLMBatchTuner::summary() const {
    auto s = getStats();
    std::ostringstream oss = {};
    oss << std::fixed << std::setprecision(1);
    oss << "LLMBatchTuner{"
        << "batch=" << s.current_batch_size
        << " ema_tps=" << s.ema_throughput
        << " avg_lat=" << s.avg_latency_ms << "ms"
        << " p99_lat=" << s.p99_latency_ms << "ms"
        << " batches=" << s.total_batches
        << " tune_events=" << s.tuning_events
        << "}";
    return oss.str();
}

// =============================================================
// BatchGuard
// =============================================================

LLMBatchTuner::BatchGuard::BatchGuard(LLMBatchTuner& tuner,
                                       size_t         batch_size,
                                       size_t         total_tokens) noexcept
    : tuner_(&tuner)
    , batch_size_(batch_size)
    , total_tokens_(total_tokens)
    , start_cycles_(HardwareCycleCounter::rdtscp())
    , start_wall_(std::chrono::steady_clock::now())
    , ended_(false)
{}

LLMBatchTuner::BatchGuard::BatchGuard(BatchGuard&& other) noexcept
    : tuner_(other.tuner_)
    , batch_size_(other.batch_size_)
    , total_tokens_(other.total_tokens_)
    , start_cycles_(other.start_cycles_)
    , start_wall_(other.start_wall_)
    , ended_(other.ended_)
{
    other.ended_ = true;
}

LLMBatchTuner::BatchGuard::~BatchGuard() noexcept {
    if (!ended_) {
        end();
    }
}

void LLMBatchTuner::BatchGuard::end() noexcept {
    if (ended_ || tuner_ == nullptr) {
        return;
    }
    ended_ = true;

    if (batch_size_ == 0 || total_tokens_ == 0) {
        return;
    }

    uint64_t end_cycles  = HardwareCycleCounter::rdtscp();
    auto     end_wall    = std::chrono::steady_clock::now();

    uint64_t cycles_elapsed = (end_cycles >= start_cycles_)
                                ? (end_cycles - start_cycles_)
                                : 0;

    double latency_ms = std::chrono::duration<double, std::milli>(
                            end_wall - start_wall_).count();

    BatchRecord rec;
    rec.batch_size   = batch_size_;
    rec.total_tokens = total_tokens_;
    rec.cycles       = cycles_elapsed;
    rec.latency_ms   = latency_ms;
    rec.tokens_per_s = (latency_ms > 0.0)
                         ? (static_cast<double>(total_tokens_) / (latency_ms / 1000.0))
                         : 0.0;

    tuner_->pushRecord(rec);
}


} // namespace phase3
} // namespace performance
} // namespace themis
