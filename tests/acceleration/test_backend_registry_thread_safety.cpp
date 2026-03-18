// Test: BackendRegistry Thread-Safety
//
// Validates that BackendRegistry is safe to use from multiple threads
// concurrently.  Specifically it checks that 16 reader threads calling
// getBestVectorBackend() concurrently while a background thread calls
// autoDetect() produce no crashes and no data races (verified under TSan).
//
// All public methods that read shared state hold a shared_lock on
// registryMutex_; autoDetect() ultimately calls registerBackend() which holds
// a unique_lock.  These tests exercise that invariant directly.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include <atomic>
#include <thread>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// 16 reader threads + 1 autoDetect() writer
// =============================================================================

// 16 threads all call getBestVectorBackend() concurrently while a background
// thread calls autoDetect() repeatedly.  No crash or data race expected.
// Under ThreadSanitizer (TSan) this test will fail if registryMutex_ is not
// properly held in all code paths.
TEST(BackendRegistryThreadSafety, ConcurrentGetBestVector_WhileAutoDetect_NoCrash) {
    // Ensure the registry is initialised with at least CPU backends before the
    // concurrent phase so readers have valid data to traverse.
    BackendRegistry::instance().initializeRuntime();

    constexpr int kReaders = 16;
    std::atomic<bool> running{true};
    std::atomic<int>  ok{0};

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&running, &ok]() {
            while (running.load(std::memory_order_relaxed)) {
                auto* b = BackendRegistry::instance().getBestVectorBackend();
                if (b != nullptr) {
                    ok.fetch_add(1, std::memory_order_relaxed);
                }
                std::this_thread::yield();
            }
        });
    }

    // Background writer: call autoDetect() several times while readers are
    // actively iterating backends_.  autoDetect() calls registerBackend()
    // which acquires a unique_lock — this is the write contention the mutex
    // must serialise.
    std::thread writer([&running]() {
        for (int i = 0; i < 5; ++i) {
            BackendRegistry::instance().autoDetect();
            std::this_thread::yield();
        }
        running.store(false, std::memory_order_release);
    });

    writer.join();
    for (auto& t : readers) t.join();

    // CPU backend is always present; at least one read must have succeeded.
    EXPECT_GT(ok.load(), 0);
}

// =============================================================================
// 16 reader threads on getBestVectorBackend() after initializeRuntime()
// =============================================================================

// Regression: plain concurrent reads must not race on selectedVectorBackend_
// (which is protected by registryMutex_).
TEST(BackendRegistryThreadSafety, ConcurrentGetBestVectorBackend_NoCrash) {
    BackendRegistry::instance().initializeRuntime();

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
    for (auto& t : threads) t.join();

    EXPECT_GT(ok.load(), 0);
}

// =============================================================================
// isRuntimeInitialized() concurrent reads
// =============================================================================

// isRuntimeInitialized() uses std::atomic<bool> with acquire/release ordering;
// concurrent reads from multiple threads must never observe torn state.
TEST(BackendRegistryThreadSafety, IsRuntimeInitialized_ConcurrentReads_NoCrash) {
    BackendRegistry::instance().initializeRuntime();

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
    for (auto& t : threads) t.join();

    EXPECT_EQ(true_count.load(), kThreads);
}

// =============================================================================
// Mixed: getSelectedVectorBackend() + autoDetect()
// =============================================================================

// getSelectedVectorBackend() reads selectedVectorBackend_ under a shared lock
// while autoDetect() → registerBackend() holds an exclusive lock to modify
// backends_.  This exercises the exclusive/shared lock interaction on
// registryMutex_.
TEST(BackendRegistryThreadSafety, ConcurrentGetSelectedVector_WhileAutoDetect_NoCrash) {
    BackendRegistry::instance().initializeRuntime();

    constexpr int kReaders = 8;
    std::atomic<bool> running{true};

    std::vector<std::thread> readers;
    readers.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back([&running]() {
            while (running.load(std::memory_order_relaxed)) {
                (void)BackendRegistry::instance().getSelectedVectorBackend();
                (void)BackendRegistry::instance().getSelectedGraphBackend();
                (void)BackendRegistry::instance().getSelectedGeoBackend();
                std::this_thread::yield();
            }
        });
    }

    std::thread writer([&running]() {
        for (int i = 0; i < 5; ++i) {
            BackendRegistry::instance().autoDetect();
            std::this_thread::yield();
        }
        running.store(false, std::memory_order_release);
    });

    writer.join();
    for (auto& t : readers) t.join();
    SUCCEED();
}
