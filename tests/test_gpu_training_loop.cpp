#include "themis/gpu/training_loop.h"
#include <gtest/gtest.h>
#include <vector>

using namespace themis::gpu;
using Batch = GPUTrainingLoop::Batch;

// ============================================================================
// Helpers
// ============================================================================

// Create N identical single-sample batches.
static std::vector<Batch> makeBatches(size_t n, size_t samples = 1) {
    Batch sample(samples, std::vector<float>(4, 1.0f));
    return std::vector<Batch>(n, sample);
}

// Constant loss function.
static GPUTrainingLoop::LossFn constLoss(double v) {
    return [v](const Batch&) { return v; };
}

// Loss that decrements each call.
static GPUTrainingLoop::LossFn decrementingLoss(double start, double step) {
    double cur = start;
    return [cur, step](const Batch&) mutable {
        double v = cur;
        cur -= step;
        return v;
    };
}

// ============================================================================
// Fixture
// ============================================================================

class GPUTrainingLoopTest : public ::testing::Test {};

// ============================================================================
// Construction
// ============================================================================

TEST_F(GPUTrainingLoopTest, DefaultConstructor) {
    GPUTrainingLoop loop;
    EXPECT_EQ(loop.currentStep(), 0u);
    EXPECT_EQ(loop.lastLoss(), 0.0);
    EXPECT_FALSE(loop.isStopped());
    EXPECT_TRUE(loop.history().empty());
}

TEST_F(GPUTrainingLoopTest, ConfigConstructor) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps = 50;
    cfg.checkpoint_interval = 5;
    GPUTrainingLoop loop(cfg);
    EXPECT_EQ(loop.currentStep(), 0u);
}

// ============================================================================
// Basic run
// ============================================================================

TEST_F(GPUTrainingLoopTest, RunSingleBatch) {
    GPUTrainingLoop loop;
    auto batches = makeBatches(1);
    auto stats = loop.run(batches, constLoss(0.5));
    EXPECT_EQ(stats.steps, 1u);
    EXPECT_NEAR(stats.avg_loss, 0.5, 1e-9);
}

TEST_F(GPUTrainingLoopTest, RunMultipleBatches) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps = 100;
    GPUTrainingLoop loop(cfg);
    auto batches = makeBatches(10);
    auto stats = loop.run(batches, constLoss(1.0));
    EXPECT_EQ(stats.steps, 10u);
    EXPECT_NEAR(stats.avg_loss, 1.0, 1e-9);
}

TEST_F(GPUTrainingLoopTest, StepCounterIncrements) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps = 100;
    GPUTrainingLoop loop(cfg);
    loop.run(makeBatches(5), constLoss(0.1));
    EXPECT_EQ(loop.currentStep(), 5u);
    loop.run(makeBatches(3), constLoss(0.1));
    EXPECT_EQ(loop.currentStep(), 8u);
}

TEST_F(GPUTrainingLoopTest, HistoryRecorded) {
    GPUTrainingLoop loop;
    loop.run(makeBatches(4), constLoss(0.25));
    ASSERT_EQ(loop.history().size(), 4u);
    for (const auto& rec : loop.history()) {
        EXPECT_NEAR(rec.loss, 0.25, 1e-9);
    }
}

// ============================================================================
// max_steps cap
// ============================================================================

TEST_F(GPUTrainingLoopTest, MaxStopsAtCap) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps = 3;
    GPUTrainingLoop loop(cfg);
    auto stats = loop.run(makeBatches(10), constLoss(1.0));
    EXPECT_EQ(stats.steps, 3u);
    EXPECT_TRUE(loop.isStopped());
}

// ============================================================================
// Early stopping
// ============================================================================

TEST_F(GPUTrainingLoopTest, EarlyStopOnLossThreshold) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps       = 100;
    cfg.early_stop_loss = 0.5;
    GPUTrainingLoop loop(cfg);

    // Loss starts at 1.0 and decrements by 0.1 per step → crosses 0.5 at step 6
    auto stats = loop.run(makeBatches(20), decrementingLoss(1.0, 0.1));
    EXPECT_LT(stats.steps, 20u);
    EXPECT_TRUE(loop.isStopped());
}

// ============================================================================
// Checkpoint callback
// ============================================================================

TEST_F(GPUTrainingLoopTest, CheckpointCalledAtInterval) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps           = 100;
    cfg.checkpoint_interval = 3;
    GPUTrainingLoop loop(cfg);

    std::vector<size_t> checkpoint_steps;
    auto cp = [&](size_t step, double) {
        checkpoint_steps.push_back(step);
    };

    loop.run(makeBatches(9), constLoss(1.0), cp);
    // Steps 3, 6, 9 should have triggered checkpoints
    ASSERT_EQ(checkpoint_steps.size(), 3u);
    EXPECT_EQ(checkpoint_steps[0], 3u);
    EXPECT_EQ(checkpoint_steps[1], 6u);
    EXPECT_EQ(checkpoint_steps[2], 9u);
}

TEST_F(GPUTrainingLoopTest, CheckpointDisabledWhenZero) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps           = 100;
    cfg.checkpoint_interval = 0;
    GPUTrainingLoop loop(cfg);

    int cp_count = 0;
    loop.run(makeBatches(5), constLoss(1.0),
             [&](size_t, double) { ++cp_count; });
    EXPECT_EQ(cp_count, 0);
}

// ============================================================================
// Epoch stats
// ============================================================================

TEST_F(GPUTrainingLoopTest, EpochStatsMinMax) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps = 100;
    GPUTrainingLoop loop(cfg);

    double losses[] = {0.8, 0.3, 0.5};
    size_t idx = 0;
    auto loss_fn = [&](const Batch&) { return losses[idx++ % 3]; };

    auto stats = loop.run(makeBatches(3), loss_fn);
    EXPECT_NEAR(stats.min_loss, 0.3, 1e-9);
    EXPECT_NEAR(stats.max_loss, 0.8, 1e-9);
    EXPECT_NEAR(stats.avg_loss, (0.8 + 0.3 + 0.5) / 3.0, 1e-9);
}

TEST_F(GPUTrainingLoopTest, LastEpochStatsAccessible) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps = 100;
    GPUTrainingLoop loop(cfg);
    loop.run(makeBatches(4), constLoss(2.0));
    auto es = loop.lastEpochStats();
    EXPECT_EQ(es.steps, 4u);
    EXPECT_NEAR(es.avg_loss, 2.0, 1e-9);
}

// ============================================================================
// Multi-epoch (sequential run calls)
// ============================================================================

TEST_F(GPUTrainingLoopTest, MultipleEpochsAccumulateHistory) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps = 1000;
    GPUTrainingLoop loop(cfg);
    loop.run(makeBatches(5), constLoss(1.0));
    loop.run(makeBatches(5), constLoss(0.5));
    EXPECT_EQ(loop.history().size(), 10u);
    EXPECT_EQ(loop.currentStep(), 10u);
}

// ============================================================================
// reset
// ============================================================================

TEST_F(GPUTrainingLoopTest, ResetClearsState) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps = 5;
    GPUTrainingLoop loop(cfg);
    loop.run(makeBatches(5), constLoss(1.0));
    EXPECT_TRUE(loop.isStopped());

    loop.reset();
    EXPECT_EQ(loop.currentStep(), 0u);
    EXPECT_EQ(loop.lastLoss(), 0.0);
    EXPECT_FALSE(loop.isStopped());
    EXPECT_TRUE(loop.history().empty());
}

TEST_F(GPUTrainingLoopTest, ResetAllowsReuse) {
    GPUTrainingLoop::Config cfg;
    cfg.max_steps = 3;
    GPUTrainingLoop loop(cfg);
    loop.run(makeBatches(5), constLoss(0.9));
    loop.reset();
    auto stats = loop.run(makeBatches(2), constLoss(0.1));
    EXPECT_EQ(stats.steps, 2u);
    EXPECT_EQ(loop.currentStep(), 2u);
}

// ============================================================================
// Error handling
// ============================================================================

TEST_F(GPUTrainingLoopTest, EmptyBatchesThrows) {
    GPUTrainingLoop loop;
    EXPECT_THROW(loop.run({}, constLoss(1.0)), std::invalid_argument);
}

TEST_F(GPUTrainingLoopTest, NullLossFnThrows) {
    GPUTrainingLoop loop;
    EXPECT_THROW(loop.run(makeBatches(1), nullptr), std::invalid_argument);
}
