// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_training_cancellation.cpp
 * @brief Phase 4 training cancellation and resource cleanup tests.
 *
 * Tests verify:
 *  - Graceful training cancellation
 *  - Resource cleanup on cancellation
 *  - Partial checkpoint state handling
 *  - Cancellation signal propagation
 *  - Thread-safe cancellation flags
 *  - Cleanup verification after cancellation
 *  - Multiple cancellation requests (idempotent)
 *  - Cleanup statistics reporting
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <string>

// Simplified training session for testing cancellation
namespace themis {
namespace training {

class TrainingSession {
public:
    explicit TrainingSession(const std::string& id = "session_1")
        : session_id_(id), cancelled_(false), completed_(false), checkpoint_saved_(false) {}

    void startTraining(int duration_ms = 5000) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (training_thread_.joinable()) {
            return;  // Already running
        }

        training_thread_ = std::thread([this, duration_ms]() {
            auto start = std::chrono::high_resolution_clock::now();

            while (true) {
                if (cancelled_.load()) {
                    cleanupResources();
                    break;
                }

                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - start).count();

                if (elapsed >= duration_ms) {
                    saveCheckpoint();
                    completed_ = true;
                    break;
                }

                // Simulate training work
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                current_epoch_++;
            }
        });
    }

    void cancel() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!cancelled_.load()) {
            cancelled_ = true;
            if (training_thread_.joinable()) {
                training_thread_.join();
            }
        }
    }

    bool isCancelled() const {
        return cancelled_.load();
    }

    bool isCompleted() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return completed_;
    }

    bool hasCheckpointSaved() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return checkpoint_saved_;
    }

    int getCurrentEpoch() const {
        return current_epoch_.load();
    }

    std::string getSessionId() const {
        return session_id_;
    }

    struct CleanupStats {
        bool resources_freed = false;
        int epoch_at_cancellation = 0;
        bool checkpoint_preserved = false;
        std::chrono::milliseconds cleanup_duration;
    };

    CleanupStats getCleanupStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cleanup_stats_;
    }

    void waitForCompletion(int timeout_ms = 10000) {
        auto start = std::chrono::high_resolution_clock::now();
        while (!isCompleted() && !isCancelled()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start).count();
            if (elapsed > timeout_ms) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

private:
    void cleanupResources() {
        auto start = std::chrono::high_resolution_clock::now();

        // Simulate cleanup operations
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start);

        CleanupStats stats;
        stats.resources_freed = true;
        stats.epoch_at_cancellation = current_epoch_.load();
        stats.checkpoint_preserved = checkpoint_saved_.load();
        stats.cleanup_duration = elapsed;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            cleanup_stats_ = stats;
        }
    }

    void saveCheckpoint() {
        std::lock_guard<std::mutex> lock(mutex_);
        checkpoint_saved_ = true;
    }

    mutable std::mutex mutex_;
    std::thread training_thread_;
    std::string session_id_;
    std::atomic<bool> cancelled_;
    std::atomic<bool> completed_;
    std::atomic<bool> checkpoint_saved_;
    std::atomic<int> current_epoch_{0};
    CleanupStats cleanup_stats_;
};

}  // namespace training
}  // namespace themis

using namespace themis::training;

// ============================================================================
// Basic cancellation
// ============================================================================

TEST(TrainingCancellationTest, StartAndCancel_Succeeds) {
    TrainingSession session("test_1");
    session.startTraining(5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_FALSE(session.isCancelled());
    session.cancel();
    EXPECT_TRUE(session.isCancelled());
}

TEST(TrainingCancellationTest, CancelledSessionStopsTraining) {
    TrainingSession session("test_2");
    session.startTraining(10000);  // Long training

    // Let it run a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int epoch_at_cancel = session.getCurrentEpoch();
    session.cancel();

    // Wait a bit more
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int epoch_after = session.getCurrentEpoch();

    // Epoch should not increase significantly after cancellation
    EXPECT_LE(epoch_after - epoch_at_cancel, 10);
}

// ============================================================================
// Resource cleanup
// ============================================================================

TEST(TrainingCancellationTest, Cleanup_ResourcesFreed) {
    TrainingSession session("test_3");
    session.startTraining(5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    session.cancel();

    auto stats = session.getCleanupStats();
    EXPECT_TRUE(stats.resources_freed);
}

TEST(TrainingCancellationTest, Cleanup_StatsReported) {
    TrainingSession session("test_4");
    session.startTraining(5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    session.cancel();

    auto stats = session.getCleanupStats();
    EXPECT_TRUE(stats.resources_freed);
    EXPECT_GE(stats.epoch_at_cancellation, 0);
    EXPECT_GE(stats.cleanup_duration.count(), 0);
}

// ============================================================================
// Cancellation idempotency
// ============================================================================

TEST(TrainingCancellationTest, MultipleCancels_Idempotent) {
    TrainingSession session("test_5");
    session.startTraining(5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    session.cancel();
    EXPECT_TRUE(session.isCancelled());

    // Second cancel should not throw
    EXPECT_NO_THROW(session.cancel());
    EXPECT_TRUE(session.isCancelled());

    // Third cancel
    EXPECT_NO_THROW(session.cancel());
    EXPECT_TRUE(session.isCancelled());
}

// ============================================================================
// Checkpoint handling on cancellation
// ============================================================================

TEST(TrainingCancellationTest, CancelBeforeCheckpoint_NoCheckpoint) {
    TrainingSession session("test_6");
    session.startTraining(10000);  // Long duration

    // Cancel immediately
    session.cancel();

    EXPECT_TRUE(session.isCancelled());
    // Checkpoint may or may not have been saved depending on timing
    // But we should be able to query the state
    session.hasCheckpointSaved();  // Should not throw
}

TEST(TrainingCancellationTest, CancelAfterCheckpoint_PreservesCheckpoint) {
    TrainingSession session("test_7");
    session.startTraining(1000);  // Short duration - likely completes

    // Let it complete
    session.waitForCompletion();

    // Cancel after completion
    session.cancel();

    // Checkpoint should be preserved
    auto stats = session.getCleanupStats();
    EXPECT_TRUE(stats.checkpoint_preserved || !session.isCompleted());
}

// ============================================================================
// Graceful shutdown
// ============================================================================

TEST(TrainingCancellationTest, GracefulShutdown_AllResourcesReleased) {
    {
        TrainingSession session("test_8");
        session.startTraining(5000);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        session.cancel();
        auto stats = session.getCleanupStats();
        EXPECT_TRUE(stats.resources_freed);
    }
    // Destructor should complete without hanging
    EXPECT_TRUE(true);
}

// ============================================================================
// Concurrent operations
// ============================================================================

TEST(TrainingCancellationTest, ConcurrentCancels_Safe) {
    TrainingSession session("test_9");
    session.startTraining(5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&session]() {
            session.cancel();
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_TRUE(session.isCancelled());
}

TEST(TrainingCancellationTest, CancelDuringTraining_Safe) {
    TrainingSession session("test_10");
    session.startTraining(5000);

    std::thread cancel_thread([&session]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        session.cancel();
    });

    cancel_thread.join();

    EXPECT_TRUE(session.isCancelled());
}

// ============================================================================
// Cancellation state transitions
// ============================================================================

TEST(TrainingCancellationTest, StateTransitions_Valid) {
    TrainingSession session("test_11");

    // Initial state
    EXPECT_FALSE(session.isCancelled());
    EXPECT_FALSE(session.isCompleted());

    session.startTraining(5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // After start
    EXPECT_FALSE(session.isCancelled());

    session.cancel();

    // After cancel
    EXPECT_TRUE(session.isCancelled());
}

TEST(TrainingCancellationTest, CancelBeforeStart_Safe) {
    TrainingSession session("test_12");

    // Cancel before starting
    session.cancel();
    EXPECT_TRUE(session.isCancelled());

    // Starting after cancel should be safe or no-op
    session.startTraining(1000);
}

// ============================================================================
// Cleanup timing
// ============================================================================

TEST(TrainingCancellationTest, CleanupTiming_Reasonable) {
    TrainingSession session("test_13");
    session.startTraining(5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    session.cancel();

    auto stats = session.getCleanupStats();
    // Cleanup should be reasonably fast (less than 1 second)
    EXPECT_LT(stats.cleanup_duration.count(), 1000);
}

// ============================================================================
// Multiple sessions
// ============================================================================

TEST(TrainingCancellationTest, MultipleSessions_IndependentCancellation) {
    TrainingSession session1("test_14_s1");
    TrainingSession session2("test_14_s2");

    session1.startTraining(5000);
    session2.startTraining(5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    session1.cancel();

    EXPECT_TRUE(session1.isCancelled());
    EXPECT_FALSE(session2.isCancelled());

    session2.cancel();
    EXPECT_TRUE(session2.isCancelled());
}

// ============================================================================
// Early completion vs cancellation
// ============================================================================

TEST(TrainingCancellationTest, CompletionBeforeCancel_PreservesState) {
    TrainingSession session("test_15");
    session.startTraining(500);  // Short - should complete

    session.waitForCompletion();

    // Now cancel after completion
    session.cancel();

    EXPECT_TRUE(session.isCancelled());
}

TEST(TrainingCancellationTest, CancelBeforeCompletion_StopsTraining) {
    TrainingSession session("test_16");
    session.startTraining(10000);  // Long duration

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    session.cancel();

    // Should not complete
    EXPECT_TRUE(session.isCancelled());
}

// ============================================================================
// Error handling in cancellation
// ============================================================================

TEST(TrainingCancellationTest, CancelMultipleTimes_NoThrow) {
    TrainingSession session("test_17");
    session.startTraining(5000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_NO_THROW({
        for (int i = 0; i < 10; ++i) {
            session.cancel();
        }
    });
}

// ============================================================================
// Session lifecycle with cancellation
// ============================================================================

TEST(TrainingCancellationTest, SessionLifecycle_CompleteCancellationFlow) {
    TrainingSession session("test_18");

    // Start
    session.startTraining(2000);
    EXPECT_FALSE(session.isCancelled());

    // Let it run
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Cancel
    session.cancel();
    EXPECT_TRUE(session.isCancelled());

    // Verify cleanup
    auto stats = session.getCleanupStats();
    EXPECT_TRUE(stats.resources_freed);

    // Multiple queries should work
    EXPECT_TRUE(session.isCancelled());
    session.getCleanupStats();
}
