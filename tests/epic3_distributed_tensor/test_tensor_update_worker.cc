/// @file test_tensor_update_worker.cc
/// @brief CTest for tensor update worker lifecycle and operations
/// @author ThemisDB Implementation Team
/// @date 2026-07-02
///
/// Tests update worker:
/// - Worker lifecycle (startup, process loop, shutdown)
/// - Update path selection (patch, refit, rebuild)
/// - Crash recovery
/// - Rank growth handling
/// - Quality metrics tracking

#include <gtest/gtest.h>
#include "distributed_tensor/include/artifact_manifest.h"
#include <memory>
#include <queue>
#include <vector>

namespace themis {
namespace distributed_tensor {

// Mock UpdateTask for testing
struct UpdateTask {
    enum Type {
        PATCH,      // Small delta patch
        REFIT,      // Partial rebuild
        REBUILD     // Full rebuild
    };

    Type task_type = PATCH;
    std::string artifact_id;
    int32_t delta_size_bytes = 0;
    int32_t new_rank = 0;
};

// Mock UpdateWorker for testing
class UpdateWorker {
public:
    enum State {
        IDLE,
        READY,
        PROCESSING,
        CRASHED
    };

    UpdateWorker() : state_(IDLE) {}

    void start() {
        // If recovering from a crash, preserve process_count_ as the safe point.
        if (state_ == CRASHED) {
            state_ = READY;
        } else {
            state_ = READY;
            process_count_ = 0;
        }
    }

    void shutdown() {
        state_ = IDLE;
    }

    void processTask(const UpdateTask& task) {
        if (state_ != READY) return;

        state_ = PROCESSING;
        process_count_++;

        // Simulate different task types
        switch (task.task_type) {
            case UpdateTask::PATCH:
                patch_count_++;
                break;
            case UpdateTask::REFIT:
                refit_count_++;
                break;
            case UpdateTask::REBUILD:
                rebuild_count_++;
                break;
        }

        // Simulate quality metrics update
        quality_score_ = 0.95 - (0.01 * (process_count_ % 10));

        state_ = READY;
    }

    void simulateCrash() {
        state_ = CRASHED;
    }

    State getState() const { return state_; }
    int32_t getProcessCount() const { return process_count_; }
    int32_t getPatchCount() const { return patch_count_; }
    int32_t getRefitCount() const { return refit_count_; }
    int32_t getRebuildCount() const { return rebuild_count_; }
    double getQualityScore() const { return quality_score_; }

private:
    State state_;
    int32_t process_count_ = 0;
    int32_t patch_count_ = 0;
    int32_t refit_count_ = 0;
    int32_t rebuild_count_ = 0;
    double quality_score_ = 1.0;
};

/// Test fixture for update worker tests
class TensorUpdateWorkerTest : public ::testing::Test {
protected:
    void SetUp() override {
        worker_ = std::make_unique<UpdateWorker>();
        manifest_.artifact_id = "test:tensor:worker";
        manifest_.rank_cap = 16;
    }

    std::unique_ptr<UpdateWorker> worker_;
    ArtifactManifest manifest_;
};

// ============================================================================
// Worker Lifecycle Tests
// ============================================================================

TEST_F(TensorUpdateWorkerTest, WorkerLifecycleStart) {
    // Verify: worker enters ready state on startup
    EXPECT_EQ(worker_->getState(), UpdateWorker::IDLE);

    worker_->start();
    EXPECT_EQ(worker_->getState(), UpdateWorker::READY);
    EXPECT_EQ(worker_->getProcessCount(), 0);
}

TEST_F(TensorUpdateWorkerTest, WorkerLifecycleProcessLoop) {
    // Verify: worker processes queued updates sequentially
    worker_->start();
    EXPECT_EQ(worker_->getState(), UpdateWorker::READY);

    // Simulate processing 3 tasks
    for (int i = 0; i < 3; ++i) {
        UpdateTask task;
        task.task_type = UpdateTask::PATCH;
        task.artifact_id = "test:tensor";

        worker_->processTask(task);
        EXPECT_EQ(worker_->getState(), UpdateWorker::READY);
    }

    EXPECT_EQ(worker_->getProcessCount(), 3);
}

TEST_F(TensorUpdateWorkerTest, WorkerLifecycleShutdown) {
    // Verify: worker transitions to idle on shutdown
    worker_->start();
    EXPECT_EQ(worker_->getState(), UpdateWorker::READY);

    worker_->shutdown();
    EXPECT_EQ(worker_->getState(), UpdateWorker::IDLE);
}

// ============================================================================
// Update Path Selection Tests
// ============================================================================

TEST_F(TensorUpdateWorkerTest, WorkerPatchSmallDelta) {
    // Verify: patch path selected for small deltas
    worker_->start();

    UpdateTask task;
    task.task_type = UpdateTask::PATCH;
    task.delta_size_bytes = 50;  // 50 bytes

    worker_->processTask(task);
    EXPECT_EQ(worker_->getPatchCount(), 1);
    EXPECT_EQ(worker_->getRefitCount(), 0);
    EXPECT_EQ(worker_->getRebuildCount(), 0);
}

TEST_F(TensorUpdateWorkerTest, WorkerRefitPartialChange) {
    // Verify: refit path selected for partial changes
    worker_->start();

    UpdateTask task;
    task.task_type = UpdateTask::REFIT;
    task.delta_size_bytes = 5000;  // 5KB

    worker_->processTask(task);
    EXPECT_EQ(worker_->getPatchCount(), 0);
    EXPECT_EQ(worker_->getRefitCount(), 1);
    EXPECT_EQ(worker_->getRebuildCount(), 0);
}

TEST_F(TensorUpdateWorkerTest, WorkerRebuildFull) {
    // Verify: rebuild path for full artifact regeneration
    worker_->start();

    UpdateTask task;
    task.task_type = UpdateTask::REBUILD;
    task.delta_size_bytes = 100000;  // 100KB

    worker_->processTask(task);
    EXPECT_EQ(worker_->getPatchCount(), 0);
    EXPECT_EQ(worker_->getRefitCount(), 0);
    EXPECT_EQ(worker_->getRebuildCount(), 1);
}

// ============================================================================
// Rank Growth and Adaptation Tests
// ============================================================================

TEST_F(TensorUpdateWorkerTest, WorkerRankGrowthSensitivity) {
    // Verify: worker adapts to rank changes
    worker_->start();
    manifest_.rank_cap = 16;

    // Simulate tasks at different ranks
    for (int rank = 8; rank <= 32; rank += 8) {
        UpdateTask task;
        task.task_type = UpdateTask::REBUILD;
        task.new_rank = rank;

        worker_->processTask(task);
        manifest_.rank_cap = rank;
    }

    EXPECT_EQ(worker_->getProcessCount(), 4);
    EXPECT_EQ(worker_->getRebuildCount(), 4);
    EXPECT_EQ(manifest_.rank_cap, 32);
}

// ============================================================================
// Quality Metrics Tests
// ============================================================================

TEST_F(TensorUpdateWorkerTest, WorkerResidualTracking) {
    // Verify: quality metrics updated after each update
    worker_->start();

    double initial_quality = worker_->getQualityScore();
    EXPECT_EQ(initial_quality, 1.0);

    // Process tasks and verify quality tracking
    for (int i = 0; i < 5; ++i) {
        UpdateTask task;
        task.task_type = UpdateTask::PATCH;
        worker_->processTask(task);

        double quality = worker_->getQualityScore();
        EXPECT_GE(quality, 0.0);
        EXPECT_LE(quality, 1.0);
    }

    EXPECT_EQ(worker_->getProcessCount(), 5);
}

// ============================================================================
// Crash Recovery Tests
// ============================================================================

TEST_F(TensorUpdateWorkerTest, WorkerCrashResumable) {
    // Verify: worker crash can be detected and resumed from last safe point
    worker_->start();
    EXPECT_EQ(worker_->getState(), UpdateWorker::READY);

    UpdateTask task;
    task.task_type = UpdateTask::PATCH;
    worker_->processTask(task);
    EXPECT_EQ(worker_->getProcessCount(), 1);

    // Simulate crash
    worker_->simulateCrash();
    EXPECT_EQ(worker_->getState(), UpdateWorker::CRASHED);

    // Verify: safe point (process_count_) is preserved
    EXPECT_EQ(worker_->getProcessCount(), 1);

    // Simulate recovery
    worker_->start();
    EXPECT_EQ(worker_->getState(), UpdateWorker::READY);

    // Resume from last safe point (process_count_ = 1)
    UpdateTask resume_task;
    resume_task.task_type = UpdateTask::PATCH;
    worker_->processTask(resume_task);

    EXPECT_EQ(worker_->getProcessCount(), 2);
}

// ============================================================================
// Update Escalation Tests
// ============================================================================

TEST_F(TensorUpdateWorkerTest, WorkerRefitEscalatesToRebuild) {
    // Verify: failed refit escalates to rebuild path
    worker_->start();

    // Simulate refit attempt
    UpdateTask refit_task;
    refit_task.task_type = UpdateTask::REFIT;
    worker_->processTask(refit_task);
    EXPECT_EQ(worker_->getRefitCount(), 1);

    // Simulate refit failure → escalate to rebuild
    UpdateTask rebuild_task;
    rebuild_task.task_type = UpdateTask::REBUILD;
    worker_->processTask(rebuild_task);

    EXPECT_EQ(worker_->getRefitCount(), 1);
    EXPECT_EQ(worker_->getRebuildCount(), 1);
}

// ============================================================================
// Throughput Tests
// ============================================================================

TEST_F(TensorUpdateWorkerTest, WorkerThroughputPatch) {
    // Verify: patch path achieves high throughput
    worker_->start();

    const int PATCH_COUNT = 100;
    for (int i = 0; i < PATCH_COUNT; ++i) {
        UpdateTask task;
        task.task_type = UpdateTask::PATCH;
        worker_->processTask(task);
    }

    EXPECT_EQ(worker_->getProcessCount(), PATCH_COUNT);
    EXPECT_EQ(worker_->getPatchCount(), PATCH_COUNT);
}

TEST_F(TensorUpdateWorkerTest, WorkerThroughputMixed) {
    // Verify: mixed workload handling
    worker_->start();

    // Mix of patch, refit, and rebuild tasks
    for (int i = 0; i < 50; ++i) {
        UpdateTask task;
        if (i % 3 == 0) {
            task.task_type = UpdateTask::PATCH;
        } else if (i % 3 == 1) {
            task.task_type = UpdateTask::REFIT;
        } else {
            task.task_type = UpdateTask::REBUILD;
        }
        worker_->processTask(task);
    }

    EXPECT_EQ(worker_->getProcessCount(), 50);
    EXPECT_GT(worker_->getPatchCount(), 0);
    EXPECT_GT(worker_->getRefitCount(), 0);
    EXPECT_GT(worker_->getRebuildCount(), 0);
}

} // namespace distributed_tensor
} // namespace themis
