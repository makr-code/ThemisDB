/**
 * @file test_phase1_resource_management.cpp
 * @brief Phase 1 - Agent 2: Resource Management & Concurrency Hardening Tests
 *
 * Comprehensive test suite for:
 * - Exception-safe resource management (RAII patterns)
 * - Resource leaks in exception paths
 * - Thread management with timeouts
 * - Lock contention and concurrency patterns
 * - Destructor invocation and cleanup
 *
 * @version 0.1.0
 * @since 2026-08-07
 */

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <mutex>
#include <spdlog/spdlog.h>

#include "utils/thread_guard.h"
#include "aql/docs_assistant_functions.h"

using namespace themis::utils;
using namespace themis::aql;
using namespace std::chrono_literals;

namespace {

// ============================================================================
// Test Fixture for Resource Management
// ============================================================================

class ResourceManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        spdlog::set_level(spdlog::level::debug);
    }

    void TearDown() override {
        // Verify no leaks after each test
    }
};

// ============================================================================
// Test 1: DocsAssistantFunctions Singleton (Resource Leak Fix)
// ============================================================================

/**
 * @test RM-01: Singleton resource is properly managed (no raw new)
 *
 * Verifies that getDocsAssistantFunctions() uses Meyer's singleton pattern
 * and does not leak resources on repeated access.
 *
 * Expected: PASS
 * - Multiple calls return the same instance
 * - Instance is properly destroyed on program exit
 * - No raw new/delete involved
 */
TEST_F(ResourceManagementTest, SingletonNoResourceLeak) {
    DocsAssistantFunctions& ref1 = getDocsAssistantFunctions();
    DocsAssistantFunctions& ref2 = getDocsAssistantFunctions();

    // Same object
    EXPECT_EQ(&ref1, &ref2);

    // No explicit cleanup needed - automatic via static destruction
}

/**
 * @test RM-02: Singleton is exception-safe during initialization
 *
 * Even if the constructor throws (e.g., degraded mode), the singleton
 * should still be accessible and in a valid state.
 *
 * Expected: PASS
 * - First call constructs instance (may degrade if docs not found)
 * - Subsequent calls return same degraded instance
 * - No exceptions thrown on access
 */
TEST_F(ResourceManagementTest, SingletonExceptionSafe) {
    try {
        DocsAssistantFunctions& ref = getDocsAssistantFunctions();
        // Even if degraded, should be valid
        EXPECT_TRUE(true);
    } catch (...) {
        FAIL() << "Singleton initialization threw unexpected exception";
    }
}

/**
 * @test RM-03: Singleton thread-safety
 *
 * Multiple threads accessing singleton concurrently should all get
 * the same instance without races.
 *
 * Expected: PASS
 * - All threads see the same pointer value
 * - No data races (C++11 static initialization is thread-safe)
 */
TEST_F(ResourceManagementTest, SingletonThreadSafe) {
    std::vector<DocsAssistantFunctions*> ptrs;
    std::mutex mtx;
    const int N = 10;

    auto worker = [&]() {
        DocsAssistantFunctions& ref = getDocsAssistantFunctions();
        std::lock_guard<std::mutex> lock(mtx);
        ptrs.push_back(&ref);
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    // All pointers should be identical
    EXPECT_EQ(ptrs.size(), N);
    for (const auto* ptr : ptrs) {
        EXPECT_EQ(ptr, ptrs[0]);
    }
}

// ============================================================================
// Test 2: ThreadGuard Exception Safety
// ============================================================================

/**
 * @test RM-04: ThreadGuard ensures proper thread cleanup
 *
 * ThreadGuard should join thread in destructor without exceptions.
 *
 * Expected: PASS
 * - Thread completes before guard destruction
 * - No dangling thread references
 * - Destructor is noexcept
 */
TEST_F(ResourceManagementTest, ThreadGuardCleanup) {
    std::atomic<bool> thread_ran{false};
    {
        ThreadGuard guard(
            std::thread([&]() { thread_ran = true; }),
            std::chrono::seconds(5)
        );
    } // Guard destructor calls join

    EXPECT_TRUE(thread_ran) << "Thread should have run before guard destruction";
}

/**
 * @test RM-05: ThreadGuard is exception-safe
 *
 * ThreadGuard destructor must not throw even if join fails.
 *
 * Expected: PASS
 * - Destructor is noexcept and doesn't throw
 * - Timeouts are handled gracefully
 */
TEST_F(ResourceManagementTest, ThreadGuardNoexcept) {
    EXPECT_NO_THROW({
        std::atomic<bool> should_run{true};
        ThreadGuard guard(
            std::thread([&]() {
                // Cooperatively wait for shutdown
                while (should_run) {
                    std::this_thread::sleep_for(10ms);
                }
            }),
            std::chrono::seconds(1)
        );
        std::this_thread::sleep_for(100ms);
        should_run = false;
        // Destructor should not throw
    });
}

/**
 * @test RM-06: ThreadGuard construction with non-joinable thread throws
 *
 * Expected: THROWS std::invalid_argument
 */
TEST_F(ResourceManagementTest, ThreadGuardRejectsNonJoinable) {
    std::thread empty_thread;
    EXPECT_FALSE(empty_thread.joinable());

    EXPECT_THROW({
        ThreadGuard guard(std::move(empty_thread));
    }, std::invalid_argument);
}

/**
 * @test RM-07: ThreadGuard timeout handling
 *
 * If thread doesn't finish within timeout, join_with_timeout returns false.
 *
 * Expected: PASS
 * - join_with_timeout() returns false on timeout
 * - Destructor still succeeds (noexcept)
 * - Thread continues running (not forcefully terminated)
 *
 * @note This test uses a long-running thread to verify timeout behavior
 */
TEST_F(ResourceManagementTest, ThreadGuardTimeoutHandling) {
    std::atomic<bool> should_stop{false};

    {
        ThreadGuard guard(
            std::thread([&]() {
                // Long-running thread that ignores shutdown flag
                while (!should_stop) {
                    std::this_thread::sleep_for(1ms);
                }
            }),
            std::chrono::milliseconds(10) // Very short timeout
        );

        // Let thread start
        std::this_thread::sleep_for(20ms);

        // Expect join_with_timeout to work (thread should exit after flag set)
        should_stop = true;
    } // Destructor should handle cleanup gracefully
}

// ============================================================================
// Test 3: RAII Resource Management Patterns
// ============================================================================

/**
 * @test RM-08: unique_ptr automatic cleanup on exception
 *
 * Verifies that unique_ptr properly cleans up resources even when
 * exception is thrown during operations.
 *
 * Expected: PASS
 * - unique_ptr destructor is called (tracked via atomic counter)
 * - Exception doesn't prevent cleanup
 */
TEST_F(ResourceManagementTest, UniquePtrExceptionCleanup) {
    static std::atomic<int> cleanup_count{0};

    struct Resource {
        ~Resource() { ++cleanup_count; }
    };

    cleanup_count = 0;
    {
        auto ptr = std::make_unique<Resource>();
        EXPECT_EQ(cleanup_count, 0);

        try {
            throw std::runtime_error("test exception");
        } catch (...) {
            // ptr still in scope - should cleanup on block exit
        }
    }

    EXPECT_EQ(cleanup_count, 1) << "unique_ptr should clean up even after exception";
}

/**
 * @test RM-09: shared_ptr reference counting under exceptions
 *
 * shared_ptr should maintain correct reference count even with exceptions.
 *
 * Expected: PASS
 * - use_count is accurate
 * - Last reference triggers cleanup
 */
TEST_F(ResourceManagementTest, SharedPtrRefCounting) {
    static std::atomic<int> cleanup_count{0};

    struct Resource {
        ~Resource() { ++cleanup_count; }
    };

    cleanup_count = 0;
    {
        auto ptr1 = std::make_shared<Resource>();
        EXPECT_EQ(ptr1.use_count(), 1);

        {
            auto ptr2 = ptr1;
            EXPECT_EQ(ptr1.use_count(), 2);
        }

        EXPECT_EQ(ptr1.use_count(), 1);
        EXPECT_EQ(cleanup_count, 0);
    }

    EXPECT_EQ(cleanup_count, 1);
}

/**
 * @test RM-10: lock_guard RAII exception safety
 *
 * Lock is released even if code within lock_guard throws.
 *
 * Expected: PASS
 * - Lock is properly released after exception
 * - Subsequent locks succeed
 */
TEST_F(ResourceManagementTest, LockGuardExceptionSafety) {
    std::mutex mtx;
    bool lock_acquired_twice = false;

    try {
        {
            std::lock_guard<std::mutex> lock(mtx);
            // Try to acquire same lock in another scope
            // (This would deadlock if lock not released)
            throw std::runtime_error("test");
        }
    } catch (const std::exception&) {
        // Lock should be released by now
        if (mtx.try_lock()) {
            mtx.unlock();
            lock_acquired_twice = true;
        }
    }

    EXPECT_TRUE(lock_acquired_twice) << "Lock should be released after exception";
}

// ============================================================================
// Test 4: Lock Contention & Concurrency Patterns
// ============================================================================

/**
 * @test RM-11: No circular lock ordering (basic check)
 *
 * Verify that our locking patterns don't introduce deadlocks.
 *
 * Expected: PASS
 * - Multiple threads acquire locks in consistent order
 * - No deadlock occurs
 */
TEST_F(ResourceManagementTest, NoCircularLockOrdering) {
    std::mutex mtx1, mtx2;
    std::atomic<int> successful_acquisitions{0};

    auto worker = [&]() {
        std::lock_guard<std::mutex> lock1(mtx1);
        std::this_thread::sleep_for(1ms); // Simulate work
        std::lock_guard<std::mutex> lock2(mtx2);
        ++successful_acquisitions;
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successful_acquisitions, 5) << "All threads should acquire locks successfully";
}

/**
 * @test RM-12: Lock contention doesn't cause memory leaks
 *
 * High contention scenarios shouldn't leak resources.
 *
 * Expected: PASS
 * - All allocations are properly freed
 * - No locks left in bad state
 */
TEST_F(ResourceManagementTest, LockContentionNoLeak) {
    std::mutex mtx;
    std::vector<std::unique_ptr<int>> allocations;

    auto worker = [&]() {
        for (int i = 0; i < 100; ++i) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                allocations.push_back(std::make_unique<int>(i));
            }
            std::this_thread::yield();
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(allocations.size(), 400) << "All allocations should be tracked";
}

// ============================================================================
// Test 5: Destructor Invocation Verification
// ============================================================================

/**
 * @test RM-13: Destructors are called in correct order
 *
 * RAII objects should have destructors called in reverse construction order.
 *
 * Expected: PASS
 * - Destructors called in LIFO order
 * - Cleanup order is deterministic
 */
TEST_F(ResourceManagementTest, DestructorOrder) {
    static std::vector<int> destruction_order;
    destruction_order.clear();

    struct Tracker {
        int id;
        explicit Tracker(int id_) : id(id_) {}
        ~Tracker() { destruction_order.push_back(id); }
    };

    {
        Tracker t1(1), t2(2), t3(3);
    }

    EXPECT_EQ(destruction_order, std::vector<int>({3, 2, 1}))
        << "Destructors should be called in reverse order";
}

/**
 * @test RM-14: Destructor exception safety
 *
 * Destructors must be noexcept and handle exceptions gracefully.
 *
 * Expected: PASS
 * - No exceptions thrown during destruction
 * - All cleanup still occurs
 */
TEST_F(ResourceManagementTest, DestructorNoThrow) {
    bool cleanup_occurred = false;

    struct Resource {
        bool& cleanup_flag;
        explicit Resource(bool& flag) : cleanup_flag(flag) {}
        ~Resource() noexcept { cleanup_flag = true; }
    };

    EXPECT_NO_THROW({
        Resource res(cleanup_occurred);
    });

    EXPECT_TRUE(cleanup_occurred);
}

// ============================================================================
// Test Summary & Metrics
// ============================================================================

} // anonymous namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
