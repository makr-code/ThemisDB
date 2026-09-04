// Test: BackendRegistry Runtime Startup (capability-driven selection)
//
// Validates the initializeRuntime() API added to BackendRegistry that performs
// capability-driven backend selection at server startup.  Specifically it checks:
//  1. isRuntimeInitialized() is false before initializeRuntime() is called
//  2. After initializeRuntime() the flag is true
//  3. getSelectedVectorBackend() / getSelectedGraphBackend() / getSelectedGeoBackend()
//     return the same pointers as the corresponding selectXBackendFor() calls
//  4. The default requirements select CPU backends on a system without GPU hardware
//  5. Custom capability requirements are honoured (impossible reqs → nullptr)
//  6. A second call to initializeRuntime() re-runs selection and updates cached pointers
//  7. defaultVectorRequirements() / defaultGraphRequirements() / defaultGeoRequirements()
//     produce the expected constraint fields
//
// These tests run on any platform (no GPU required).

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::acceleration;

// =============================================================================
// Default requirement helpers
// =============================================================================

TEST(BackendRegistryStartup, DefaultVectorRequirements_NeedsVectorOps) {
    const auto reqs = BackendRegistry::defaultVectorRequirements();
    EXPECT_TRUE(reqs.needsVectorOps);
    EXPECT_FALSE(reqs.needsGraphOps);
    EXPECT_FALSE(reqs.needsGeoOps);
}

TEST(BackendRegistryStartup, DefaultVectorRequirements_RequiresFP32) {
    const auto reqs = BackendRegistry::defaultVectorRequirements();
    EXPECT_TRUE(hasPrecision(reqs.requiredPrecisions, PrecisionMode::FP32));
}

TEST(BackendRegistryStartup, DefaultVectorRequirements_RequiresAllANNMetrics) {
    const auto reqs = BackendRegistry::defaultVectorRequirements();
    EXPECT_NE(reqs.requiredMetrics & metricBit(DistanceMetric::L2),            0u);
    EXPECT_NE(reqs.requiredMetrics & metricBit(DistanceMetric::COSINE),        0u);
    EXPECT_NE(reqs.requiredMetrics & metricBit(DistanceMetric::INNER_PRODUCT), 0u);
}

TEST(BackendRegistryStartup, DefaultGraphRequirements_NeedsGraphOps) {
    const auto reqs = BackendRegistry::defaultGraphRequirements();
    EXPECT_FALSE(reqs.needsVectorOps);
    EXPECT_TRUE(reqs.needsGraphOps);
    EXPECT_FALSE(reqs.needsGeoOps);
}

TEST(BackendRegistryStartup, DefaultGeoRequirements_NeedsGeoOps) {
    const auto reqs = BackendRegistry::defaultGeoRequirements();
    EXPECT_FALSE(reqs.needsVectorOps);
    EXPECT_FALSE(reqs.needsGraphOps);
    EXPECT_TRUE(reqs.needsGeoOps);
}

TEST(BackendRegistryStartup, DefaultGeoRequirements_RequiresFP32) {
    const auto reqs = BackendRegistry::defaultGeoRequirements();
    EXPECT_TRUE(hasPrecision(reqs.requiredPrecisions, PrecisionMode::FP32));
}

// =============================================================================
// initializeRuntime() lifecycle
// =============================================================================

// Fresh (non-initialized) state: the singleton always registers CPU backends
// but initializeRuntime() has not been called on a freshly constructed instance.
// Because BackendRegistry is a singleton that persists across tests we cannot
// reliably test the "before first call" state in a separate test.  Instead we
// verify the post-call invariants.

TEST(BackendRegistryStartup, IsRuntimeInitializedAfterCall) {
    BackendRegistry::instance().initializeRuntime();
    EXPECT_TRUE(BackendRegistry::instance().isRuntimeInitialized());
}

TEST(BackendRegistryStartup, GetSelectedVectorBackend_NotNullAfterInit) {
    BackendRegistry::instance().initializeRuntime();
    EXPECT_NE(BackendRegistry::instance().getSelectedVectorBackend(), nullptr);
}

TEST(BackendRegistryStartup, GetSelectedGraphBackend_NotNullAfterInit) {
    BackendRegistry::instance().initializeRuntime();
    EXPECT_NE(BackendRegistry::instance().getSelectedGraphBackend(), nullptr);
}

TEST(BackendRegistryStartup, GetSelectedGeoBackend_NotNullAfterInit) {
    BackendRegistry::instance().initializeRuntime();
    EXPECT_NE(BackendRegistry::instance().getSelectedGeoBackend(), nullptr);
}

// =============================================================================
// Selection results on a machine without GPU hardware (CPU fallback)
// =============================================================================

TEST(BackendRegistryStartup, SelectedVectorBackend_IsCPU_WhenNoGPU) {
    BackendRegistry::instance().initializeRuntime();
    auto* vb = BackendRegistry::instance().getSelectedVectorBackend();
    ASSERT_NE(vb, nullptr);
    EXPECT_EQ(vb->type(), BackendType::CPU);
}

TEST(BackendRegistryStartup, SelectedGraphBackend_IsCPU_WhenNoGPU) {
    BackendRegistry::instance().initializeRuntime();
    auto* gb = BackendRegistry::instance().getSelectedGraphBackend();
    ASSERT_NE(gb, nullptr);
    EXPECT_EQ(gb->type(), BackendType::CPU);
}

TEST(BackendRegistryStartup, SelectedGeoBackend_IsCPU_WhenNoGPU) {
    BackendRegistry::instance().initializeRuntime();
    auto* geo = BackendRegistry::instance().getSelectedGeoBackend();
    ASSERT_NE(geo, nullptr);
    EXPECT_EQ(geo->type(), BackendType::CPU);
}

// The startup-selected vector backend must satisfy the default requirements.
TEST(BackendRegistryStartup, SelectedVectorBackend_SatisfiesDefaultRequirements) {
    BackendRegistry::instance().initializeRuntime();
    auto* vb = BackendRegistry::instance().getSelectedVectorBackend();
    ASSERT_NE(vb, nullptr);
    EXPECT_TRUE(BackendRegistry::satisfies(
        vb->getCapabilities(),
        BackendRegistry::defaultVectorRequirements()));
}

TEST(BackendRegistryStartup, SelectedGraphBackend_SatisfiesDefaultRequirements) {
    BackendRegistry::instance().initializeRuntime();
    auto* gb = BackendRegistry::instance().getSelectedGraphBackend();
    ASSERT_NE(gb, nullptr);
    EXPECT_TRUE(BackendRegistry::satisfies(
        gb->getCapabilities(),
        BackendRegistry::defaultGraphRequirements()));
}

TEST(BackendRegistryStartup, SelectedGeoBackend_SatisfiesDefaultRequirements) {
    BackendRegistry::instance().initializeRuntime();
    auto* geo = BackendRegistry::instance().getSelectedGeoBackend();
    ASSERT_NE(geo, nullptr);
    EXPECT_TRUE(BackendRegistry::satisfies(
        geo->getCapabilities(),
        BackendRegistry::defaultGeoRequirements()));
}

// =============================================================================
// Custom capability requirements
// =============================================================================

// Impossible requirements (async + vector) yield nullptr when only CPU backends exist.
TEST(BackendRegistryStartup, InitRuntime_ImpossibleVectorReqs_SelectedIsNull) {
    BackendRegistry::CapabilityRequirements impossible;
    impossible.needsVectorOps = true;
    impossible.needsAsync     = true; // CPU backends don't expose async

    BackendRegistry::instance().initializeRuntime(
        impossible,
        BackendRegistry::defaultGraphRequirements(),
        BackendRegistry::defaultGeoRequirements());

    EXPECT_EQ(BackendRegistry::instance().getSelectedVectorBackend(), nullptr);
    // Graph and geo should still resolve.
    EXPECT_NE(BackendRegistry::instance().getSelectedGraphBackend(), nullptr);
    EXPECT_NE(BackendRegistry::instance().getSelectedGeoBackend(),   nullptr);
}

// FP16-only requirement returns nullptr when only CPU (FP32) backends exist.
TEST(BackendRegistryStartup, InitRuntime_FP16VectorReqs_SelectedIsNull_WhenNoGPU) {
    BackendRegistry::CapabilityRequirements fp16reqs;
    fp16reqs.needsVectorOps     = true;
    fp16reqs.requiredPrecisions = PrecisionMode::FP16;

    BackendRegistry::instance().initializeRuntime(
        fp16reqs,
        BackendRegistry::defaultGraphRequirements(),
        BackendRegistry::defaultGeoRequirements());

    EXPECT_EQ(BackendRegistry::instance().getSelectedVectorBackend(), nullptr);
}

// =============================================================================
// Re-initialization
// =============================================================================

TEST(BackendRegistryStartup, SecondInitializeRuntime_OverwritesCachedSelection) {
    // First call: impossible vector reqs → nullptr
    BackendRegistry::CapabilityRequirements impossible;
    impossible.needsVectorOps = true;
    impossible.needsAsync     = true;

    BackendRegistry::instance().initializeRuntime(
        impossible,
        BackendRegistry::defaultGraphRequirements(),
        BackendRegistry::defaultGeoRequirements());

    EXPECT_EQ(BackendRegistry::instance().getSelectedVectorBackend(), nullptr);

    // Second call: default reqs → CPU backend
    BackendRegistry::instance().initializeRuntime();
    EXPECT_NE(BackendRegistry::instance().getSelectedVectorBackend(), nullptr);
}

// =============================================================================
// Consistency with selectVectorBackendFor()
// =============================================================================

// getSelectedVectorBackend() must return the same pointer as a direct call to
// selectVectorBackendFor() with the same requirements.
TEST(BackendRegistryStartup, SelectedVectorBackend_ConsistentWithSelectAPI) {
    BackendRegistry::instance().initializeRuntime();

    auto* fromStartup = BackendRegistry::instance().getSelectedVectorBackend();
    auto* fromSelect  = BackendRegistry::instance().selectVectorBackendFor(
        BackendRegistry::defaultVectorRequirements());

    EXPECT_EQ(fromStartup, fromSelect);
}

TEST(BackendRegistryStartup, SelectedGraphBackend_ConsistentWithSelectAPI) {
    BackendRegistry::instance().initializeRuntime();

    auto* fromStartup = BackendRegistry::instance().getSelectedGraphBackend();
    auto* fromSelect  = BackendRegistry::instance().selectGraphBackendFor(
        BackendRegistry::defaultGraphRequirements());

    EXPECT_EQ(fromStartup, fromSelect);
}

TEST(BackendRegistryStartup, SelectedGeoBackend_ConsistentWithSelectAPI) {
    BackendRegistry::instance().initializeRuntime();

    auto* fromStartup = BackendRegistry::instance().getSelectedGeoBackend();
    auto* fromSelect  = BackendRegistry::instance().selectGeoBackendFor(
        BackendRegistry::defaultGeoRequirements());

    EXPECT_EQ(fromStartup, fromSelect);
}

// =============================================================================
// Thread-safety tests
// =============================================================================

// 16 threads all call getBestVectorBackend() concurrently.
// None should crash, return garbage, or trigger a data race under TSan.
TEST(BackendRegistryStartup, ConcurrentGetBestVectorBackend_NoCrash) {
    BackendRegistry::instance().initializeRuntime();

    constexpr int kThreads = 16;
    std::vector<std::thread> threads;
    std::atomic<int> ok{0};
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&ok]() {
            auto* b = BackendRegistry::instance().getBestVectorBackend();
            if (b != nullptr) {
              ++ok;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    // CPU backend is always registered, so at least 1 should have succeeded.
    EXPECT_GT(ok.load(), 0);
}

// 8 reader threads call getBestVectorBackend() while a writer thread calls
// getAvailableBackends().  No crash or data race expected.
TEST(BackendRegistryStartup, ConcurrentReadersAndAvailableBackends_NoCrash) {
    BackendRegistry::instance().initializeRuntime();

    constexpr int kReaders = 8;
    std::atomic<bool> running{true};
    std::vector<std::thread> readers;
    readers.reserve(kReaders);

    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&running]() {
            while (running.load(std::memory_order_relaxed)) {
                (void)BackendRegistry::instance().getBestVectorBackend();
            }
        });
    }

    // Background writer
    std::thread writer([&running]() {
        for (int i = 0; i < 20; ++i) {
            (void)BackendRegistry::instance().getAvailableBackends();
            std::this_thread::yield();
        }
        running.store(false, std::memory_order_release);
    });

    writer.join();
    for (auto& t : readers) {
      t.join();
    }
    SUCCEED();
}

// isRuntimeInitialized() must be thread-safe after initializeRuntime().
TEST(BackendRegistryStartup, IsRuntimeInitialized_ThreadSafe) {
    BackendRegistry::instance().initializeRuntime();

    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> true_count{0};
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&true_count]() {
            if (BackendRegistry::instance().isRuntimeInitialized()) {
              ++true_count;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(true_count.load(), kThreads);
}
