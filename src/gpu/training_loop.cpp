/**
 * @file training_loop.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "themis/gpu/training_loop.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GPUTrainingLoop::GPUTrainingLoop() : GPUTrainingLoop(Config{}) {}

GPUTrainingLoop::GPUTrainingLoop(const Config &config) : config_(config) {
    last_epoch_          = {};
    last_epoch_.min_loss = 0.0;
    last_epoch_.max_loss = 0.0;
}

// ---------------------------------------------------------------------------
// run
// ---------------------------------------------------------------------------

GPUTrainingLoop::EpochStats GPUTrainingLoop::run(const std::vector<Batch> &batches, LossFn loss_fn,
                                                 CheckpointFn checkpoint) {
    if (batches.empty() || !loss_fn) {
        throw std::invalid_argument("GPUTrainingLoop::run: batches and loss_fn must be non-empty");
    }

    // Build iteration order (optionally shuffled) ----------------------------
    std::vector<size_t> order(batches.size());
    std::iota(order.begin(), order.end(), 0);
    if (config_.shuffle_batches) {
        // Deterministic shuffle based on current step for reproducibility.
        for (size_t i = order.size(); i > 1; --i) {
            size_t j = (step_ + i) % i;
            std::swap(order[i - 1], order[j]);
        }
    }

    // Epoch state ------------------------------------------------------------
    double sum_loss    = 0.0;
    double min_loss    = std::numeric_limits<double>::max();
    double max_loss    = std::numeric_limits<double>::lowest();
    size_t gpu_steps   = 0;
    size_t cpu_steps   = 0;
    size_t local_steps = 0;

    std::lock_guard<std::mutex> lk(mutex_);
    ++epoch_num_;

    for (size_t idx : order) {
        if (stopped_) {
            break;
        }
        if (config_.max_steps > 0 && step_ >= config_.max_steps) {
            stopped_ = true;
            break;
        }

        double loss = loss_fn(batches[idx]);
        bool gpu    = (loss >= 0.0); // negative sentinel means CPU fallback

        StepRecord rec{step_, loss, gpu};
        history_.push_back(rec);

        ++step_;
        if (config_.max_steps > 0 && step_ >= config_.max_steps) {
            stopped_ = true;
        }
        ++local_steps;
        last_loss_ = loss;
        sum_loss += loss;
        if (loss < min_loss)
            min_loss = loss;
        if (loss > max_loss)
            max_loss = loss;

        if (gpu) {
            ++gpu_steps;
        } else {
            ++cpu_steps;
        }

        // Checkpoint ---------------------------------------------------------
        if (checkpoint && config_.checkpoint_interval > 0 && step_ % config_.checkpoint_interval == 0) {
            checkpoint(step_, loss);
        }

        // Early stopping -----------------------------------------------------
        if (config_.early_stop_loss > 0.0 && loss <= config_.early_stop_loss) {
            stopped_ = true;
            break;
        }
    }

    EpochStats es;
    es.epoch     = epoch_num_;
    es.steps     = local_steps;
    es.avg_loss  = local_steps > 0 ? sum_loss / static_cast<double>(local_steps) : 0.0;
    
    // Use safe floating-point comparisons: check if min/max were ever updated
    // from their sentinel values (std::numeric_limits<double>::max/lowest()).
    // Avoid exact equality checks with infinity values.
    if (std::isfinite(min_loss) && min_loss != std::numeric_limits<double>::max()) {
       es.min_loss = min_loss;
    } else {
       es.min_loss = 0.0;
    }
    
    if (std::isfinite(max_loss) && max_loss != std::numeric_limits<double>::lowest()) {
       es.max_loss = max_loss;
    } else {
       es.max_loss = 0.0;
    }
    
    es.gpu_steps = gpu_steps;
    es.cpu_steps = cpu_steps;

    last_epoch_ = es;
    return es;
}

// ---------------------------------------------------------------------------
// Introspection
// ---------------------------------------------------------------------------

size_t GPUTrainingLoop::currentStep() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return step_;
}

double GPUTrainingLoop::lastLoss() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return last_loss_;
}

const std::vector<GPUTrainingLoop::StepRecord> &GPUTrainingLoop::history() const {
    // NOTE: caller must ensure no concurrent run() call.
    return history_;
}

GPUTrainingLoop::EpochStats GPUTrainingLoop::lastEpochStats() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return last_epoch_;
}

bool GPUTrainingLoop::isStopped() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return stopped_;
}

void GPUTrainingLoop::reset() {
    std::lock_guard<std::mutex> lk(mutex_);
    step_      = 0;
    last_loss_ = 0.0;
    stopped_   = false;
    history_.clear();
    last_epoch_ = {};
    epoch_num_  = 0;
}

} // namespace gpu
} // namespace themis
