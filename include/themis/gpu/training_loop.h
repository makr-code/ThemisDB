/**
 * @file training_loop.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief GPU training loop coordinator.
 *
 * Orchestrates the training lifecycle for GPU-backed ML workloads:
 * - Mini-batch iteration
 * - Forward / backward pass via a caller-supplied `LossFn`
 * - Loss tracking, step counting, early stopping
 * - Periodic checkpoint callbacks
 * - Per-epoch statistics
 *
 * Like `GPULauncher`, the actual GPU computation is supplied as a `LossFn`
 * callback so the coordinator is testable without real hardware.  When
 * THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP are defined the `LossFn` body
 * would invoke a CUDA/ROCm kernel and return the device-side loss value.
 *
 * Thread safety: public methods are protected by an internal mutex.
 */
class GPUTrainingLoop {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------
    using Batch        = std::vector<std::vector<float>>;
    /// Forward + backward pass returning the scalar loss for this batch.
    using LossFn       = std::function<double(const Batch&)>;
    /// Called every checkpoint_interval steps with (step, loss).
    using CheckpointFn = std::function<void(size_t step, double loss)>;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    struct Config {
        size_t max_steps            = 1000;
        size_t checkpoint_interval  = 100;  ///< 0 = disabled
        size_t log_interval         = 10;
        /// Stop early when loss drops below this value (0.0 = disabled).
        double early_stop_loss      = 0.0;
        bool   shuffle_batches      = false;
    };

    // -----------------------------------------------------------------------
    // Per-step record
    // -----------------------------------------------------------------------
    struct StepRecord {
        size_t step    = 0;
        double loss    = 0.0;
        bool   gpu_run = false;  ///< true when LossFn indicates GPU was used
    };

    // -----------------------------------------------------------------------
    // Per-epoch summary
    // -----------------------------------------------------------------------
    struct EpochStats {
        size_t epoch      = 0;
        size_t steps      = 0;
        double avg_loss   = 0.0;
        double min_loss   = 0.0;
        double max_loss   = 0.0;
        size_t gpu_steps  = 0;
        size_t cpu_steps  = 0;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    GPUTrainingLoop();
    explicit GPUTrainingLoop(const Config& config);

    // -----------------------------------------------------------------------
    // Execution
    // -----------------------------------------------------------------------

    /**
     * @brief Run one epoch over @p batches using @p loss_fn.
     *
     * Iterates batches in order (or shuffled when Config::shuffle_batches is
     * true), calls @p loss_fn for each batch, tracks loss history, fires
     * @p checkpoint at every `checkpoint_interval` steps, and stops early
     * when Config::early_stop_loss is configured and the loss crosses the
     * threshold.
     *
     * @return EpochStats summarising the completed epoch.
     */
    EpochStats run(const std::vector<Batch>& batches,
                   LossFn                    loss_fn,
                   CheckpointFn              checkpoint = nullptr);

    // -----------------------------------------------------------------------
    // Introspection
    // -----------------------------------------------------------------------
    size_t                  currentStep()    const;
    double                  lastLoss()       const;
    const std::vector<StepRecord>& history() const;
    EpochStats              lastEpochStats() const;
    bool                    isStopped()      const;

    /// Reset step counter, loss history, and stopped flag.
    void reset();

private:
    Config                  config_;
    mutable std::mutex      mutex_;
    size_t                  step_       = 0;
    double                  last_loss_  = 0.0;
    bool                    stopped_    = false;
    std::vector<StepRecord> history_;
    EpochStats              last_epoch_;
    size_t                  epoch_num_  = 0;
};

} // namespace gpu
} // namespace themis
