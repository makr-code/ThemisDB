#include <gtest/gtest.h>
#include "graph/graph_resource_pool.h"
#include <atomic>
#include <chrono>
#include <future>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themis::graph;

// ============================================================================
// Helper: mock "connection" type
// ============================================================================
struct MockConnection {
    explicit MockConnection(int id) : id(id) {}
    int id;
};

// ============================================================================
// P3-03 Group 1: GraphConnectionPool – construction and basic acquire
// ============================================================================

TEST(ConnectionPoolTest, ConstructionPreWarmsPool) {
    int counter = 0;
    GraphConnectionPool<MockConnection> pool(3, [&] {
        return std::make_shared<MockConnection>(counter++);
    });
    EXPECT_EQ(3u, pool.capacity());
    EXPECT_EQ(3u, pool.available());
}

TEST(ConnectionPoolTest, AcquireDecreasesAvailable) {
    GraphConnectionPool<MockConnection> pool(3, [] {
        return std::make_shared<MockConnection>(0);
    });
    auto h = pool.acquire();
    EXPECT_EQ(2u, pool.available());
}

TEST(ConnectionPoolTest, ScopedHandleReleasesOnDestruction) {
    GraphConnectionPool<MockConnection> pool(2, [] {
        return std::make_shared<MockConnection>(0);
    });
    {
        auto h = pool.acquire();
        EXPECT_EQ(1u, pool.available());
    }
    EXPECT_EQ(2u, pool.available());
}

TEST(ConnectionPoolTest, AcquireCountIncrements) {
    GraphConnectionPool<MockConnection> pool(4, [] {
        return std::make_shared<MockConnection>(0);
    });
    pool.acquire();
    pool.acquire();
    EXPECT_EQ(2u, pool.acquiredCount());
}

TEST(ConnectionPoolTest, ResourceAccessViaGet) {
    int id_counter = 1;
    GraphConnectionPool<MockConnection> pool(1, [&] {
        return std::make_shared<MockConnection>(id_counter++);
    });
    auto h = pool.acquire();
    EXPECT_EQ(1, h.get().id);
}

TEST(ConnectionPoolTest, TryAcquireSucceedsWhenAvailable) {
    GraphConnectionPool<MockConnection> pool(2, [] {
        return std::make_shared<MockConnection>(0);
    });
    auto h = pool.tryAcquire();
    EXPECT_TRUE(h.has_value());
}

TEST(ConnectionPoolTest, TryAcquireFailsWhenExhausted) {
    GraphConnectionPool<MockConnection> pool(1, [] {
        return std::make_shared<MockConnection>(0);
    });
    auto h1 = pool.acquire();
    auto h2 = pool.tryAcquire();
    EXPECT_FALSE(h2.has_value());
}

TEST(ConnectionPoolTest, ZeroPoolSizeThrows) {
    EXPECT_THROW(
        (GraphConnectionPool<MockConnection>(0, [] {
            return std::make_shared<MockConnection>(0);
        })),
        std::invalid_argument);
}

TEST(ConnectionPoolTest, ScopedHandleBoolOperator) {
    GraphConnectionPool<MockConnection> pool(1, [] {
        return std::make_shared<MockConnection>(0);
    });
    auto h = pool.acquire();
    EXPECT_TRUE(static_cast<bool>(h));
}

TEST(ConnectionPoolTest, MoveConstructedHandlePreservesResource) {
    GraphConnectionPool<MockConnection> pool(2, [] {
        return std::make_shared<MockConnection>(42);
    });
    auto h1 = pool.acquire();
    auto h2 = std::move(h1);
    EXPECT_TRUE(static_cast<bool>(h2));
    EXPECT_EQ(42, h2.get().id);
}

TEST(ConnectionPoolTest, ConcurrentAcquireAndRelease) {
    GraphConnectionPool<MockConnection> pool(4, [] {
        return std::make_shared<MockConnection>(0);
    });
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads = {};

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&] {
            auto h = pool.acquire();
            if (h) {
                std::this_thread::sleep_for(std::chrono::milliseconds{2});
                ++success_count;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(8, success_count.load());
    EXPECT_EQ(4u, pool.available());
}

// ============================================================================
// P3-03 Group 2: GraphThreadPool – task submission and execution
// ============================================================================

TEST(ThreadPoolTest, ZeroThreadsThrows) {
    EXPECT_THROW(GraphThreadPool pool(0), std::invalid_argument);
}

TEST(ThreadPoolTest, ConstructWithMultipleThreads) {
    GraphThreadPool pool(4);
    EXPECT_EQ(4u, pool.threadCount());
}

TEST(ThreadPoolTest, SubmitSingleTask) {
    GraphThreadPool pool(2);
    auto f = pool.submit([] { return 42; });
    EXPECT_EQ(42, f.get());
}

TEST(ThreadPoolTest, SubmitMultipleTasks) {
    GraphThreadPool pool(4);
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 8; ++i) {
        futures.push_back(pool.submit([i] { return i * i; }));
    }
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(i * i, futures[i].get());
    }
}

TEST(ThreadPoolTest, CompletedCountMatchesSubmitted) {
    GraphThreadPool pool(2);
    const int N = 10;
    std::vector<std::future<void>> futs;
    for (int i = 0; i < N; ++i) {
        futs.push_back(pool.submit([] { /* no-op */ }));
    }
    for (auto& f : futs) {
      f.get();
    }
    EXPECT_EQ(static_cast<uint64_t>(N), pool.completedCount());
}

TEST(ThreadPoolTest, QueuedCountMatchesSubmitted) {
    GraphThreadPool pool(1);
    std::atomic<bool> gate{false};
    // Block the worker
    auto blocker = pool.submit([&] {
        while (!gate.load()) {
          std::this_thread::yield();
        }
    });
    // Submit 5 more tasks
    for (int i = 0; i < 5; ++i) pool.submit([] {});
    EXPECT_GE(pool.queuedCount(), 1u);
    gate.store(true);
    blocker.get();
}

TEST(ThreadPoolTest, ShutdownPreventsNewSubmit) {
    GraphThreadPool pool(1);
    pool.shutdown();
    EXPECT_TRUE(pool.isStopped());
    EXPECT_THROW(pool.submit([] {}), std::runtime_error);
}

TEST(ThreadPoolTest, ConcurrentSubmission) {
    GraphThreadPool pool(4);
    std::atomic<int> counter{0};
    std::vector<std::thread> threads = {};

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&] {
            auto f = pool.submit([&] { ++counter; });
            f.get();
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(8, counter.load());
}

TEST(ThreadPoolTest, TaskExceptionPropagates) {
    GraphThreadPool pool(2);
    auto f = pool.submit([]() -> int {
        throw std::runtime_error("expected_failure");
        return 0;
    });
    EXPECT_THROW(f.get(), std::runtime_error);
}

// ============================================================================
// P3-03 Group 3: GraphBufferPool – allocation and recycling
// ============================================================================

TEST(BufferPoolTest, ZeroDimensionsThrow) {
    EXPECT_THROW(GraphBufferPool(0, 1024), std::invalid_argument);
    EXPECT_THROW(GraphBufferPool(4, 0), std::invalid_argument);
}

TEST(BufferPoolTest, AvailableEqualsPoolSizeInitially) {
    GraphBufferPool pool(4, 1024);
    EXPECT_EQ(4u, pool.available());
}

TEST(BufferPoolTest, AcquireDecreasesAvailable) {
    GraphBufferPool pool(3, 512);
    auto b = pool.acquire();
    EXPECT_EQ(2u, pool.available());
}

TEST(BufferPoolTest, ScopedBufferReleasesOnDestruction) {
    GraphBufferPool pool(2, 256);
    {
        auto b = pool.acquire();
        EXPECT_EQ(1u, pool.available());
    }
    EXPECT_EQ(2u, pool.available());
}

TEST(BufferPoolTest, BufferSizeCorrect) {
    GraphBufferPool pool(2, 4096);
    EXPECT_EQ(4096u, pool.bufferSize());
    auto b = pool.acquire();
    EXPECT_GE(b.capacity(), 4096u);
}

TEST(BufferPoolTest, TryAcquireSucceedsWhenAvailable) {
    GraphBufferPool pool(2, 128);
    auto b = pool.tryAcquire();
    EXPECT_TRUE(b.has_value());
}

TEST(BufferPoolTest, TryAcquireFailsWhenExhausted) {
    GraphBufferPool pool(1, 128);
    auto b1 = pool.acquire();
    auto b2 = pool.tryAcquire();
    EXPECT_FALSE(b2.has_value());
}

TEST(BufferPoolTest, AcquireCountIncrements) {
    GraphBufferPool pool(4, 64);
    pool.acquire();
    pool.acquire();
    EXPECT_EQ(2u, pool.acquiredCount());
}

TEST(BufferPoolTest, BufferZeroedOnReturn) {
    GraphBufferPool pool(1, 8);
    {
        auto b = pool.acquire();
        // Write non-zero data
        for (auto& byte : b.get()) {
          byte = 0xFF;
        }
    }
    // Re-acquire: buffer should be zero-filled
    auto b2 = pool.acquire();
    for (auto byte : b2.get()) {
        EXPECT_EQ(0, byte);
    }
}

TEST(BufferPoolTest, ConcurrentAcquireAndRelease) {
    GraphBufferPool pool(4, 256);
    std::atomic<int> success{0};
    std::vector<std::thread> threads = {};

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&] {
            auto b = pool.acquire();
            if (b) {
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
                ++success;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(8, success.load());
    EXPECT_EQ(4u, pool.available());
}

TEST(BufferPoolTest, ScopedBufferBoolOperator) {
    GraphBufferPool pool(1, 64);
    auto b = pool.acquire();
    EXPECT_TRUE(static_cast<bool>(b));
}

TEST(BufferPoolTest, MultipleSequentialAcquireAndRelease) {
    GraphBufferPool pool(2, 64);
    for (int i = 0; i < 10; ++i) {
        auto b = pool.acquire();
        EXPECT_TRUE(static_cast<bool>(b));
    }
    EXPECT_EQ(2u, pool.available());
    EXPECT_EQ(10u, pool.acquiredCount());
}

TEST(BufferPoolTest, AllBuffersExhaustedThenRefilled) {
    GraphBufferPool pool(3, 32);
    {
        auto b1 = pool.acquire();
        auto b2 = pool.acquire();
        auto b3 = pool.acquire();
        EXPECT_EQ(0u, pool.available());
    }
    // All returned after scope exit
    EXPECT_EQ(3u, pool.available());
}

TEST(ConnectionPoolTest, SequentialAcquireReleaseCycles) {
    GraphConnectionPool<MockConnection> pool(2, [] {
        return std::make_shared<MockConnection>(0);
    });
    for (int i = 0; i < 5; ++i) {
        auto h = pool.acquire();
        EXPECT_TRUE(static_cast<bool>(h));
    }
    EXPECT_EQ(5u, pool.acquiredCount());
    EXPECT_EQ(2u, pool.available());
}

TEST(ThreadPoolTest, SingleThreadHandlesManyTasks) {
    GraphThreadPool pool(1);
    std::atomic<int> sum{0};
    std::vector<std::future<void>> futs;
    for (int i = 0; i < 20; ++i) {
        futs.push_back(pool.submit([&, i] { sum.fetch_add(i); }));
    }
    for (auto& f : futs) {
      f.get();
    }
    // Sum of 0..19 = 190
    EXPECT_EQ(190, sum.load());
}

TEST(BufferPoolTest, MoveConstructedBufferPreservesValidity) {
    GraphBufferPool pool(2, 64);
    auto b1 = pool.acquire();
    auto b2 = std::move(b1);
    EXPECT_TRUE(static_cast<bool>(b2));
    EXPECT_EQ(64u, b2.capacity());
}
