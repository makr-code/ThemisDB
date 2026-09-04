// Test: BackendRegistry Thread-Safety
//
// Validates that BackendRegistry is safe to use from multiple threads
// concurrently.  Specifically it checks that 16 reader threads calling
// getBestVectorBackend() concurrently while a background writer calls
// registerBackend() produce no crashes and no data races.
//
// All public methods that read shared state hold a shared_lock on
// registryMutex_; registerBackend() holds a unique_lock.  These tests
// exercise that invariant directly.
//
// NOTE: For deterministic data-race detection run this suite locally under
// ThreadSanitizer (-fsanitize=thread).  The CI build uses ASan+UBSan only.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include <atomic>
#include <thread>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// Minimal in-process stub backend
//
// Replaces autoDetect() in writer threads so that the write-contention path
// (registerBackend() unique_lock vs. reader shared_lock) is exercised without
// triggering any plugin directory scans or stderr noise.
// =============================================================================

namespace {

struct MinimalCpuVectorStub final : public IVectorBackend {
    const char*        name()        const noexcept override { return "MinimalCpuVectorStub"; }
    BackendType        type()        const noexcept override { return BackendType::CPU; }
    bool               isAvailable() const noexcept override { return true; }
    bool               initialize()        override { return true; }
    void               shutdown()          override {}

    BackendCapabilities getCapabilities() const override {
        BackendCapabilities c;
        c.supportsVectorOps   = true;
        c.supportsBatchProcessing = true;
        c.supportedPrecisions = PrecisionMode::FP32;
        c.supportedMetrics    = metricBit(DistanceMetric::L2)
                              | metricBit(DistanceMetric::COSINE)
                              | metricBit(DistanceMetric::INNER_PRODUCT);
        c.deviceName = "stub-cpu";
        return c;
    }

    std::vector<float> computeDistances(
        const float*, size_t, size_t, const float*, size_t, bool) override { return {}; }

    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float*, size_t, size_t, const float*, size_t, size_t, bool) override { return {}; }
};

} // namespace

// =============================================================================
// 16 reader threads + 1 lightweight registerBackend() writer
// =============================================================================

// 16 threads all call getBestVectorBackend() concurrently while a background
// thread repeatedly calls registerBackend() with a cheap stub.
// registerBackend() acquires a unique_lock on registryMutex_;
// getBestVectorBackend() acquires a shared_lock — this is the write/read
// contention the mutex must serialise.
TEST(BackendRegistryThreadSafety, ConcurrentGetBestVector_WhileRegister_NoCrash) {
    // Seed the registry with a known stub before calling initializeRuntime() so
    // this test is self-contained and order-independent regardless of which
    // other tests ran first or whether autoDetect() found any real backends.
    BackendRegistry::instance().registerBackend(
        std::make_unique<MinimalCpuVectorStub>());
    BackendRegistry::instance().initializeRuntime();
    // Confirm the stub is visible before spawning threads.
    ASSERT_NE(BackendRegistry::instance().getBestVectorBackend(), nullptr)
        << "Registry must expose MinimalCpuVectorStub before concurrency phase";

    constexpr int kReaders = 16;
    std::atomic<bool> running{true};
    std::atomic<int>  ok{0};
    // Start barrier: writer waits until all readers have entered their loop.
    std::atomic<int>  readersReady{0};

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&running, &ok, &readersReady]() {
            readersReady.fetch_add(1, std::memory_order_release);
            while (running.load(std::memory_order_relaxed)) {
                auto* b = BackendRegistry::instance().getBestVectorBackend();
                if (b != nullptr) {
                    ok.fetch_add(1, std::memory_order_relaxed);
                }
                std::this_thread::yield();
            }
        });
    }

    // Writer: register a lightweight stub 5× while readers are active.
    // Uses registerBackend() directly — no plugin scanning, no I/O.
    std::thread writer([&running, &readersReady]() {
        // Wait until all readers are running to maximise contention.
        while (readersReady.load(std::memory_order_acquire) < kReaders) {
            std::this_thread::yield();
        }
        for (int i = 0; i < 5; ++i) {
            BackendRegistry::instance().registerBackend(
                std::make_unique<MinimalCpuVectorStub>());
            std::this_thread::yield();
        }
        running.store(false, std::memory_order_release);
    });

    writer.join();
    for (auto& t : readers) {
      t.join();
    }

    // CPU backend is always present; at least one read must have succeeded.
    EXPECT_GT(ok.load(), 0);
}

// =============================================================================
// 16 reader threads on getBestVectorBackend() after initializeRuntime()
// =============================================================================

// Regression: plain concurrent reads must not race on selectedVectorBackend_
// (which is protected by registryMutex_).
TEST(BackendRegistryThreadSafety, ConcurrentGetBestVectorBackend_NoCrash) {
    // Seed the registry with a known stub so this test runs correctly even
    // when it is the first test executed in the process.
    BackendRegistry::instance().registerBackend(
        std::make_unique<MinimalCpuVectorStub>());
    BackendRegistry::instance().initializeRuntime();
    ASSERT_NE(BackendRegistry::instance().getBestVectorBackend(), nullptr)
        << "Registry must expose MinimalCpuVectorStub before concurrency phase";

    constexpr int kThreads = 16;
    std::vector<std::thread> threads;
    std::atomic<int> ok{0};
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&ok]() {
            auto* b = BackendRegistry::instance().getBestVectorBackend();
            if (b != nullptr) {
                ok.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_GT(ok.load(), 0);
}

// =============================================================================
// isRuntimeInitialized() concurrent reads
// =============================================================================

// isRuntimeInitialized() uses std::atomic<bool> with acquire/release ordering;
// concurrent reads from multiple threads must never observe torn state.
TEST(BackendRegistryThreadSafety, IsRuntimeInitialized_ConcurrentReads_NoCrash) {
    // Call initializeRuntime() unconditionally so this test is self-contained;
    // runtimeInitialized_ is set to true with memory_order_release.
    BackendRegistry::instance().registerBackend(
        std::make_unique<MinimalCpuVectorStub>());
    BackendRegistry::instance().initializeRuntime();
    ASSERT_NE(BackendRegistry::instance().getBestVectorBackend(), nullptr)
        << "Registry must expose MinimalCpuVectorStub before concurrency phase";

    constexpr int kThreads = 16;
    std::vector<std::thread> threads;
    std::atomic<int> true_count{0};
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&true_count]() {
            if (BackendRegistry::instance().isRuntimeInitialized()) {
                true_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(true_count.load(), kThreads);
}

// =============================================================================
// Mixed: getSelectedVectorBackend() + registerBackend() writer
// =============================================================================

// getSelectedVectorBackend() reads selectedVectorBackend_ under a shared lock
// while registerBackend() holds an exclusive lock to modify backends_.
// This exercises the exclusive/shared lock interaction on registryMutex_
// without invoking autoDetect()'s plugin directory scan.
TEST(BackendRegistryThreadSafety, ConcurrentGetSelectedVector_WhileRegister_NoCrash) {
    // Seed the registry with a known stub and run initializeRuntime() so that
    // selectedVectorBackend_ is non-null regardless of test execution order.
    BackendRegistry::instance().registerBackend(
        std::make_unique<MinimalCpuVectorStub>());
    BackendRegistry::instance().initializeRuntime();
    ASSERT_NE(BackendRegistry::instance().getBestVectorBackend(), nullptr)
        << "Registry must expose MinimalCpuVectorStub before concurrency phase";

    constexpr int kReaders = 8;
    std::atomic<bool> running{true};
    std::atomic<int>  readersReady{0};

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&running, &readersReady]() {
            readersReady.fetch_add(1, std::memory_order_release);
            while (running.load(std::memory_order_relaxed)) {
                (void)BackendRegistry::instance().getSelectedVectorBackend();
                (void)BackendRegistry::instance().getSelectedGraphBackend();
                (void)BackendRegistry::instance().getSelectedGeoBackend();
                std::this_thread::yield();
            }
        });
    }

    std::thread writer([&running, &readersReady]() {
        while (readersReady.load(std::memory_order_acquire) < kReaders) {
            std::this_thread::yield();
        }
        for (int i = 0; i < 5; ++i) {
            BackendRegistry::instance().registerBackend(
                std::make_unique<MinimalCpuVectorStub>());
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
