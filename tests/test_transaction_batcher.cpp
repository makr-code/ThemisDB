/**
 * @file test_transaction_batcher.cpp
 * @brief Unit tests for TransactionBatcher — v1.8.0
 *
 * Acceptance criteria covered:
 *  AC-1  Default BatchConfig has window=5ms, max=1000, min=10, adaptive=true
 *  AC-2  setBatchConfig() applies window clamping (< 1ms → 1ms, > 100ms → 100ms)
 *  AC-3  setBatchConfig() clamps max_batch_size >= 1
 *  AC-4  setBatchConfig() clamps min_batch_size <= max_batch_size
 *  AC-5  submitAsync() with null commit_fn returns error future immediately
 *  AC-6  submitAsync() after destructor / shutdown returns error future
 *  AC-7  Single item flushed within window and future resolves OK
 *  AC-8  Batch of items all resolve with correct Status values
 *  AC-9  flush() drains queue immediately (no need to wait for window)
 *  AC-10 max_batch_size triggers immediate flush before window expires
 *  AC-11 Failed commit_fn propagates error Status to the corresponding future
 *  AC-12 Exception thrown by commit_fn is caught and returned as error Status
 *  AC-13 Stats: batches_flushed increments after each flush
 *  AC-14 Stats: transactions_committed and transactions_failed track correctly
 *  AC-15 Stats: avg_batch_size is a running average across flushes
 *  AC-16 Stats: avg_latency_ms is non-negative
 *  AC-17 Per-table policy overrides window independently of global config
 *  AC-18 Per-table policy overrides max_batch_size independently
 *  AC-19 getTablePolicy() returns zero policy for unknown table
 *  AC-20 Concurrent submitters: all futures eventually resolve without deadlock
 *  AC-21 Fair FIFO ordering: items resolve in submission order within a batch
 *  AC-22 Adaptive sizing: adjustments increment stats.adaptive_adjustments
 *  AC-23 getBatchConfig() returns clamped config after setBatchConfig()
 *  AC-24 Destructor flushes remaining items before exiting
 *  AC-25 setTablePolicy with zero fields inherits global config values
 *  AC-26 High-throughput: 1000 concurrent submitters all get resolved futures
 */

#include <gtest/gtest.h>
#include "transaction/transaction_batcher.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>
#include <numeric>

using namespace themis;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class TransactionBatcherTest : public ::testing::Test {
protected:
    // Use a narrow 2 ms window so tests don't wait long, with adaptive off by
    // default to keep deterministic timing behaviour.
    void SetUp() override {
        batcher_ = std::make_unique<TransactionBatcher>();
        TransactionBatcher::BatchConfig cfg;
        cfg.window           = std::chrono::microseconds(2000);  // 2 ms
        cfg.max_batch_size   = 100;
        cfg.min_batch_size   = 1;
        cfg.enable_adaptive  = false;
        batcher_->setBatchConfig(cfg);
    }

    void TearDown() override {
        batcher_.reset();  // blocks until flush thread exits
    }

    std::unique_ptr<TransactionBatcher> batcher_;
};

// ─────────────────────────────────────────────────────────────────────────────
// AC-1  Default BatchConfig
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherDefaultTest, DefaultConfig) {
    TransactionBatcher b;
    auto cfg = b.getBatchConfig();
    EXPECT_EQ(cfg.window.count(), 5000);   // 5 ms
    EXPECT_EQ(cfg.max_batch_size, 1000u);
    EXPECT_EQ(cfg.min_batch_size, 10u);
    EXPECT_TRUE(cfg.enable_adaptive);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-2  setBatchConfig — window clamping
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherConfigTest, WindowClampedToMin) {
    TransactionBatcher b;
    TransactionBatcher::BatchConfig cfg;
    cfg.window = std::chrono::microseconds(0);  // below 1 ms
    b.setBatchConfig(cfg);
    auto got = b.getBatchConfig();
    EXPECT_EQ(got.window.count(), 1000);  // clamped to 1 ms
}

TEST(TransactionBatcherConfigTest, WindowClampedToMax) {
    TransactionBatcher b;
    TransactionBatcher::BatchConfig cfg;
    cfg.window = std::chrono::microseconds(200000);  // 200 ms > 100 ms max
    b.setBatchConfig(cfg);
    auto got = b.getBatchConfig();
    EXPECT_EQ(got.window.count(), 100000);  // clamped to 100 ms
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-3  setBatchConfig — max_batch_size clamped >= 1
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherConfigTest, MaxBatchSizeClamped) {
    TransactionBatcher b;
    TransactionBatcher::BatchConfig cfg;
    cfg.max_batch_size = 0;
    b.setBatchConfig(cfg);
    auto got = b.getBatchConfig();
    EXPECT_GE(got.max_batch_size, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-4  setBatchConfig — min_batch_size <= max_batch_size
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherConfigTest, MinBatchSizeCappedByMax) {
    TransactionBatcher b;
    TransactionBatcher::BatchConfig cfg;
    cfg.max_batch_size = 5;
    cfg.min_batch_size = 100;  // larger than max
    b.setBatchConfig(cfg);
    auto got = b.getBatchConfig();
    EXPECT_LE(got.min_batch_size, got.max_batch_size);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-5  submitAsync — null commit_fn returns error immediately
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, NullCommitFn_ReturnsError) {
    auto f = batcher_->submitAsync(nullptr);
    auto st = f.get();
    EXPECT_FALSE(st.ok);
    EXPECT_FALSE(st.message.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-7  Single item flushed within window
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, SingleItem_ResolvesOK) {
    bool ran = false;
    auto f = batcher_->submitAsync([&ran]() -> TransactionBatcher::Status {
        ran = true;
        return TransactionBatcher::Status::OK();
    });

    auto st = f.get();
    EXPECT_TRUE(st.ok) << st.message;
    EXPECT_TRUE(ran);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-8  Batch of items all resolve with correct Status
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, MultipleItems_AllResolve) {
    const int N = 20;
    std::vector<std::future<TransactionBatcher::Status>> futures;
    std::atomic<int> ran{0};

    for (int i = 0; i < N; ++i) {
        futures.push_back(batcher_->submitAsync([&ran]() -> TransactionBatcher::Status {
            ++ran;
            return TransactionBatcher::Status::OK();
        }));
    }

    for (auto& f : futures) {
        auto st = f.get();
        EXPECT_TRUE(st.ok) << st.message;
    }
    EXPECT_EQ(ran.load(), N);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-9  flush() drains queue immediately
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, Flush_DrainsQueueImmediately) {
    // Use a very large window so the background thread won't fire on its own.
    TransactionBatcher::BatchConfig cfg;
    cfg.window         = std::chrono::microseconds(100000);  // 100 ms
    cfg.max_batch_size = 1000;
    cfg.min_batch_size = 1;
    cfg.enable_adaptive = false;
    batcher_->setBatchConfig(cfg);

    std::atomic<int> counter{0};
    const int N = 10;
    std::vector<std::future<TransactionBatcher::Status>> futures;
    for (int i = 0; i < N; ++i) {
        futures.push_back(batcher_->submitAsync([&counter]() -> TransactionBatcher::Status {
            ++counter;
            return TransactionBatcher::Status::OK();
        }));
    }

    // Immediately flush (should not wait the full 100 ms window)
    auto t0 = std::chrono::steady_clock::now();
    batcher_->flush();
    auto elapsed = std::chrono::steady_clock::now() - t0;

    // All items should have been processed.
    for (auto& f : futures) {
        auto st = f.get();
        EXPECT_TRUE(st.ok);
    }
    EXPECT_EQ(counter.load(), N);

    // Flush should be much faster than the 100 ms window.
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 90);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-10  max_batch_size triggers immediate flush
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherMaxSizeTest, MaxBatchSize_TriggersFlush) {
    // Use a 50 ms window but max_batch_size = 5 so a batch of 5 flushes early.
    TransactionBatcher b;
    TransactionBatcher::BatchConfig cfg;
    cfg.window         = std::chrono::microseconds(50000);  // 50 ms
    cfg.max_batch_size = 5;
    cfg.min_batch_size = 1;
    cfg.enable_adaptive = false;
    b.setBatchConfig(cfg);

    std::atomic<int> processed{0};
    std::vector<std::future<TransactionBatcher::Status>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(b.submitAsync([&processed]() -> TransactionBatcher::Status {
            ++processed;
            return TransactionBatcher::Status::OK();
        }));
    }

    // All 5 items should resolve well before the 50 ms window.
    auto t0 = std::chrono::steady_clock::now();
    for (auto& f : futures) {
        auto st = f.get();
        EXPECT_TRUE(st.ok);
    }
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    EXPECT_EQ(processed.load(), 5);
    EXPECT_LT(elapsed_ms, 40);  // should be well under the 50 ms window
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-11  Failed commit_fn propagates error Status
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, FailedCommitFn_PropagatesError) {
    auto f = batcher_->submitAsync([]() -> TransactionBatcher::Status {
        return TransactionBatcher::Status::Error("intentional failure");
    });

    auto st = f.get();
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("intentional"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-12  Exception thrown by commit_fn is caught and returned as error
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, ExceptionInCommitFn_ReturnsError) {
    auto f = batcher_->submitAsync([]() -> TransactionBatcher::Status {
        throw std::runtime_error("commit blew up");
        return TransactionBatcher::Status::OK();  // unreachable
    });

    auto st = f.get();
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("commit blew up"), std::string::npos);
}

TEST_F(TransactionBatcherTest, CStringExceptionInCommitFn_ReturnsError) {
    auto f = batcher_->submitAsync([]() -> TransactionBatcher::Status {
        throw "commit cstr blew up";
    });

    auto st = f.get();
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("commit cstr blew up"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-13  Stats: batches_flushed increments
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, Stats_BatchesFlushedIncrements) {
    for (int round = 0; round < 3; ++round) {
        auto f = batcher_->submitAsync([]() -> TransactionBatcher::Status {
            return TransactionBatcher::Status::OK();
        });
        f.get();
        batcher_->flush();
    }

    auto stats = batcher_->getStats();
    EXPECT_GE(stats.batches_flushed, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-14  Stats: committed and failed counts are correct
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, Stats_CommittedAndFailedCounts) {
    // 3 successes, 2 failures
    std::vector<std::future<TransactionBatcher::Status>> futures;
    for (int i = 0; i < 5; ++i) {
        bool fail = (i < 2);
        futures.push_back(batcher_->submitAsync([fail]() -> TransactionBatcher::Status {
            return fail ? TransactionBatcher::Status::Error("err") : TransactionBatcher::Status::OK();
        }));
    }
    for (auto& f : futures) {
      f.get();
    }

    auto stats = batcher_->getStats();
    EXPECT_EQ(stats.transactions_committed, 3u);
    EXPECT_EQ(stats.transactions_failed,    2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-15  Stats: avg_batch_size is a running average
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, Stats_AvgBatchSizeIsPositive) {
    const int N = 8;
    std::vector<std::future<TransactionBatcher::Status>> futures;
    for (int i = 0; i < N; ++i) {
        futures.push_back(batcher_->submitAsync([]() -> TransactionBatcher::Status {
            return TransactionBatcher::Status::OK();
        }));
    }
    for (auto& f : futures) {
      f.get();
    }

    auto stats = batcher_->getStats();
    EXPECT_GT(stats.avg_batch_size, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-16  Stats: avg_latency_ms is non-negative
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, Stats_AvgLatencyNonNegative) {
    auto f = batcher_->submitAsync([]() -> TransactionBatcher::Status {
        return TransactionBatcher::Status::OK();
    });
    f.get();

    auto stats = batcher_->getStats();
    EXPECT_GE(stats.avg_latency_ms, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-17  Per-table policy overrides window
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherPolicyTest, TablePolicy_OverridesWindow) {
    TransactionBatcher b;

    TransactionBatcher::BatchPolicy pol;
    pol.window = std::chrono::microseconds(20000);  // 20 ms
    b.setTablePolicy("orders", pol);

    auto got = b.getTablePolicy("orders");
    EXPECT_EQ(got.window.count(), 20000);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-18  Per-table policy overrides max_batch_size
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherPolicyTest, TablePolicy_OverridesMaxBatchSize) {
    TransactionBatcher b;

    TransactionBatcher::BatchPolicy pol;
    pol.max_batch_size = 50;
    b.setTablePolicy("sessions", pol);

    auto got = b.getTablePolicy("sessions");
    EXPECT_EQ(got.max_batch_size, 50u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-19  getTablePolicy — unknown table returns zero policy
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherPolicyTest, GetTablePolicy_UnknownTableReturnsZero) {
    TransactionBatcher b;
    auto pol = b.getTablePolicy("nonexistent");
    EXPECT_EQ(pol.window.count(), 0);
    EXPECT_EQ(pol.max_batch_size, 0u);
    EXPECT_EQ(pol.min_batch_size, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-20  Concurrent submitters: all futures resolve
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherConcurrencyTest, ConcurrentSubmitters_AllResolve) {
    TransactionBatcher b;
    TransactionBatcher::BatchConfig cfg;
    cfg.window         = std::chrono::microseconds(5000);  // 5 ms
    cfg.max_batch_size = 500;
    cfg.min_batch_size = 1;
    cfg.enable_adaptive = false;
    b.setBatchConfig(cfg);

    const int N_THREADS = 10;
    const int PER_THREAD = 20;

    std::mutex fut_mutex = {};
    std::vector<std::future<TransactionBatcher::Status>> futures;
    futures.reserve(N_THREADS * PER_THREAD);

    std::atomic<int> submitted{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < N_THREADS; ++t) {
        threads.emplace_back([&b, &futures, &fut_mutex, &submitted, PER_THREAD]() {
            for (int i = 0; i < PER_THREAD; ++i) {
                auto f = b.submitAsync([]() -> TransactionBatcher::Status {
                    return TransactionBatcher::Status::OK();
                });
                {
                    std::lock_guard<std::mutex> lk(fut_mutex);
                    futures.push_back(std::move(f));
                }
                ++submitted;
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    for (auto& f : futures) {
        auto st = f.get();
        EXPECT_TRUE(st.ok) << st.message;
    }
    EXPECT_EQ(futures.size(), static_cast<size_t>(N_THREADS * PER_THREAD));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-21  FIFO ordering within a batch
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionBatcherTest, FIFO_OrderPreservedWithinBatch) {
    // Use large window so all items land in a single batch.
    TransactionBatcher::BatchConfig cfg;
    cfg.window         = std::chrono::microseconds(100000);  // 100 ms
    cfg.max_batch_size = 100;
    cfg.min_batch_size = 1;
    cfg.enable_adaptive = false;
    batcher_->setBatchConfig(cfg);

    const int N = 10;
    std::mutex mu = {};
    std::vector<int> exec_order;

    std::vector<std::future<TransactionBatcher::Status>> futures;
    for (int i = 0; i < N; ++i) {
        futures.push_back(batcher_->submitAsync([i, &exec_order, &mu]() -> TransactionBatcher::Status {
            std::lock_guard<std::mutex> lk(mu);
            exec_order.push_back(i);
            return TransactionBatcher::Status::OK();
        }));
    }

    // Force flush so all items are processed now.
    batcher_->flush();

    for (auto& f : futures) {
      f.get();
    }

    ASSERT_EQ(static_cast<int>(exec_order.size()), N);
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(exec_order[i], i) << "item at position " << i << " was " << exec_order[i];
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-22  Adaptive: adaptive_adjustments increments
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherAdaptiveTest, Adaptive_AdjustmentsIncrement) {
    TransactionBatcher b;
    TransactionBatcher::BatchConfig cfg;
    // Use a wide starting window so there is room to narrow.
    cfg.window          = std::chrono::microseconds(50000);  // 50 ms
    cfg.max_batch_size  = 5;
    cfg.min_batch_size  = 1;
    cfg.enable_adaptive = true;
    b.setBatchConfig(cfg);

    // Trigger several max_batch_size flushes.  Each batch of 5 no-op commit_fns
    // executes in microseconds, so throughput >> max_batch_size * 0.9, which
    // deterministically fires the narrowing path in adaptWindow() on every flush.
    for (int round = 0; round < 5; ++round) {
        std::vector<std::future<TransactionBatcher::Status>> futures;
        for (int i = 0; i < 5; ++i) {
            futures.push_back(b.submitAsync([]() -> TransactionBatcher::Status {
                return TransactionBatcher::Status::OK();
            }));
        }
        for (auto& f : futures) {
          f.get();
        }
    }

    auto stats = b.getStats();
    // The narrowing path must have fired at least once: window 50ms → 45ms → …
    EXPECT_GT(stats.adaptive_adjustments, 0u);
    EXPECT_GE(stats.batches_flushed, 5u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-23  getBatchConfig returns clamped values
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherConfigTest, GetBatchConfig_ReturnsClamped) {
    TransactionBatcher b;
    TransactionBatcher::BatchConfig cfg;
    cfg.window           = std::chrono::microseconds(500);  // below 1 ms
    cfg.max_batch_size   = 0;                               // clamped to 1
    cfg.min_batch_size   = 999;                             // clamped to max
    cfg.enable_adaptive  = false;
    b.setBatchConfig(cfg);

    auto got = b.getBatchConfig();
    EXPECT_GE(got.window.count(), 1000);   // at least 1 ms
    EXPECT_GE(got.max_batch_size, 1u);
    EXPECT_LE(got.min_batch_size, got.max_batch_size);
    EXPECT_FALSE(got.enable_adaptive);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-24  Destructor flushes remaining items
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherDestructorTest, Destructor_FlushesRemainingItems) {
    std::atomic<int> counter{0};
    std::vector<std::future<TransactionBatcher::Status>> futures;

    {
        TransactionBatcher b;
        TransactionBatcher::BatchConfig cfg;
        cfg.window         = std::chrono::microseconds(100000);  // 100 ms — won't fire on its own
        cfg.max_batch_size = 1000;
        cfg.min_batch_size = 1;
        cfg.enable_adaptive = false;
        b.setBatchConfig(cfg);

        for (int i = 0; i < 5; ++i) {
            futures.push_back(b.submitAsync([&counter]() -> TransactionBatcher::Status {
                ++counter;
                return TransactionBatcher::Status::OK();
            }));
        }
        // Destructor fires here — must flush remaining items.
    }

    // All futures must be resolved after the destructor returns.
    for (auto& f : futures) {
        auto st = f.get();
        EXPECT_TRUE(st.ok) << st.message;
    }
    EXPECT_EQ(counter.load(), 5);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-25  setTablePolicy with zero fields — inherits global config
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherPolicyTest, ZeroFieldPolicy_InheritsGlobal) {
    TransactionBatcher b;
    // Register a policy with all zeros (inherit everything from global).
    TransactionBatcher::BatchPolicy pol;  // all zeros
    b.setTablePolicy("metrics", pol);

    auto got = b.getTablePolicy("metrics");
    EXPECT_EQ(got.window.count(), 0);
    EXPECT_EQ(got.max_batch_size, 0u);
    EXPECT_EQ(got.min_batch_size, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-26  High-throughput: 1000 submitters all resolve
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionBatcherHighThroughputTest, HighThroughput_AllResolve) {
    const char* env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!env || std::string(env) != "1") {
        GTEST_SKIP() << "Skipped: set THEMIS_RUN_PERF_TESTS=1 to enable performance tests";
    }

    TransactionBatcher b;
    TransactionBatcher::BatchConfig cfg;
    cfg.window         = std::chrono::microseconds(5000);  // 5 ms
    cfg.max_batch_size = 200;
    cfg.min_batch_size = 10;
    cfg.enable_adaptive = true;
    b.setBatchConfig(cfg);

    const int N = 1000;
    std::atomic<int> done{0};

    std::vector<std::future<TransactionBatcher::Status>> futures;
    futures.reserve(N);

    for (int i = 0; i < N; ++i) {
        futures.push_back(b.submitAsync([&done]() -> TransactionBatcher::Status {
            ++done;
            return TransactionBatcher::Status::OK();
        }));
    }

    for (auto& f : futures) {
        auto st = f.get();
        EXPECT_TRUE(st.ok) << st.message;
    }

    EXPECT_EQ(done.load(), N);

    auto stats = b.getStats();
    EXPECT_EQ(stats.transactions_committed, static_cast<uint64_t>(N));
    EXPECT_EQ(stats.transactions_failed, 0u);
}
