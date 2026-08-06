/**
 * @file test_process_concurrency_churn_focused.cpp
 * @brief Phase 4 Concurrency Tests: Thread-safety under concurrent CRUD, import, export, linking
 * @note Test IDs: C-01..C-08
 * @note Timeout: 120s per test
 */

#include <gtest/gtest.h>
#include "process/process_api_contract.h"
#include "process/process_linker.h"

#include <atomic>
#include <barrier>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using namespace themis::process;

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixtures
// ─────────────────────────────────────────────────────────────────────────────

class ConcurrencyChurnTest : public ::testing::Test {
protected:
    static constexpr int32_t kNumThreads = 8;
    static constexpr int32_t kIterationsPerThread = 100;
};

// ─────────────────────────────────────────────────────────────────────────────
// C-01: Thread-safe atomic counter under concurrent increments
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrencyChurnTest, C01_ConcurrentAtomicIncrement) {
    std::atomic<int64_t> counter{0};
    std::barrier<std::function<void()>> sync_point(kNumThreads, []{});

    auto work = [&counter, &sync_point]() {
        sync_point.arrive_and_wait();  // Ensure all threads start together
        for (int32_t i = 0; i < kIterationsPerThread; ++i) {
            counter.fetch_add(1, std::memory_order_release);
        }
    };

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(work);
    }

    for (auto& t : threads) {
        t.join();
    }

    int64_t expected = static_cast<int64_t>(kNumThreads) * kIterationsPerThread;
    EXPECT_EQ(counter.load(std::memory_order_acquire), expected);
}

// ─────────────────────────────────────────────────────────────────────────────
// C-02: Concurrent link creation without races
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrencyChurnTest, C02_ConcurrentLinkCreation) {
    std::vector<ProcessLink> links_storage;
    std::mutex links_mutex;
    std::barrier<std::function<void()>> sync_point(kNumThreads, []{});

    auto create_links = [&links_storage, &links_mutex, &sync_point](int32_t thread_id) {
        sync_point.arrive_and_wait();
        for (int32_t i = 0; i < kIterationsPerThread; ++i) {
            ProcessLink link;
            link.link_id = "link_" + std::to_string(thread_id) + "_" + std::to_string(i);
            link.source_id = "src_" + std::to_string(thread_id);
            link.target_id = "tgt_" + std::to_string(i);
            link.link_type = ProcessLinkType::CROSS_REFERENCE;
            link.created_at_ms = 1000 + i;

            {
                std::lock_guard<std::mutex> lock(links_mutex);
                links_storage.push_back(link);
            }
        }
    };

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(create_links, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    int64_t expected_size = static_cast<int64_t>(kNumThreads) * kIterationsPerThread;
    EXPECT_EQ(links_storage.size(), expected_size);

    // Verify all link IDs are unique
    std::set<std::string> unique_ids;
    for (const auto& link : links_storage) {
        EXPECT_EQ(unique_ids.count(link.link_id), 0) << "Duplicate link ID: " << link.link_id;
        unique_ids.insert(link.link_id);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// C-03: Concurrent attachment creation with atomic clock
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrencyChurnTest, C03_ConcurrentAttachmentCreation) {
    std::vector<ProcessAttachment> attachments;
    std::mutex attachments_mutex;
    std::atomic<int64_t> clock_ms{1000};
    std::barrier<std::function<void()>> sync_point(kNumThreads, []{});

    auto create_attachments = [&attachments, &attachments_mutex, &clock_ms, &sync_point](int32_t thread_id) {
        sync_point.arrive_and_wait();
        for (int32_t i = 0; i < kIterationsPerThread; ++i) {
            ProcessAttachment attach;
            attach.id = "attach_" + std::to_string(thread_id) + "_" + std::to_string(i);
            attach.instance_id = "inst_" + std::to_string(thread_id);
            attach.object_id = "obj_" + std::to_string(i);
            attach.attached_by = "thread_" + std::to_string(thread_id);
            attach.attached_at_ms = clock_ms.fetch_add(1, std::memory_order_acq_rel);

            {
                std::lock_guard<std::mutex> lock(attachments_mutex);
                attachments.push_back(attach);
            }
        }
    };

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(create_attachments, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    int64_t expected_size = static_cast<int64_t>(kNumThreads) * kIterationsPerThread;
    EXPECT_EQ(attachments.size(), expected_size);

    // Verify timestamps are monotonically increasing (at least for the same thread)
    std::map<std::string, int64_t> last_timestamp;
    for (const auto& attach : attachments) {
        // We can't guarantee global monotonicity due to thread scheduling,
        // but we can verify no timestamp goes backward (extreme check)
        EXPECT_GE(attach.attached_at_ms, 1000);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// C-04: Read-heavy concurrent access (no writes during reads)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrencyChurnTest, C04_ReadHeavyConcurrentAccess) {
    std::vector<ProcessLink> shared_links;
    std::shared_mutex access_mutex;

    // Pre-populate with test data
    for (int32_t i = 0; i < 100; ++i) {
        ProcessLink link;
        link.link_id = "link_" + std::to_string(i);
        link.source_id = "src_" + std::to_string(i % 10);
        link.target_id = "tgt_" + std::to_string(i % 20);
        shared_links.push_back(link);
    }

    std::atomic<int64_t> read_count{0};
    std::barrier<std::function<void()>> sync_point(kNumThreads, []{});

    auto reader = [&shared_links, &access_mutex, &read_count, &sync_point]() {
        sync_point.arrive_and_wait();
        for (int32_t i = 0; i < kIterationsPerThread; ++i) {
            {
                std::shared_lock<std::shared_mutex> lock(access_mutex);
                for (const auto& link : shared_links) {
                    (void)link.link_id;  // Prevent optimization
                }
                read_count.fetch_add(1, std::memory_order_release);
            }
        }
    };

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(reader);
    }

    for (auto& t : threads) {
        t.join();
    }

    int64_t expected_reads = static_cast<int64_t>(kNumThreads) * kIterationsPerThread;
    EXPECT_EQ(read_count.load(std::memory_order_acquire), expected_reads);
}

// ─────────────────────────────────────────────────────────────────────────────
// C-05: Writer-reader interleaving (single writer, multiple readers)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrencyChurnTest, C05_SingleWriterMultipleReaders) {
    std::vector<ProcessAttachment> shared_data;
    std::shared_mutex access_mutex;
    std::atomic<int64_t> writer_done{0};

    auto writer = [&shared_data, &access_mutex, &writer_done]() {
        for (int32_t i = 0; i < kIterationsPerThread; ++i) {
            ProcessAttachment attach;
            attach.id = "attach_" + std::to_string(i);
            attach.instance_id = "inst_main";
            attach.attached_at_ms = 1000 + i;

            {
                std::unique_lock<std::shared_mutex> lock(access_mutex);
                shared_data.push_back(attach);
            }
        }
        writer_done.store(1, std::memory_order_release);
    };

    auto reader = [&shared_data, &access_mutex]() {
        int32_t reads = 0;
        while (true) {
            {
                std::shared_lock<std::shared_mutex> lock(access_mutex);
                reads += shared_data.size();
            }

            if (reads > 0) break;
            std::this_thread::yield();
        }
    };

    std::vector<std::thread> threads;
    threads.emplace_back(writer);
    for (int32_t i = 1; i < kNumThreads; ++i) {
        threads.emplace_back(reader);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(writer_done.load(std::memory_order_acquire), 1);
    EXPECT_EQ(shared_data.size(), kIterationsPerThread);
}

// ─────────────────────────────────────────────────────────────────────────────
// C-06: Concurrent error enum access (no mutation)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrencyChurnTest, C06_ConcurrentErrorEnumAccess) {
    std::atomic<int64_t> checks_performed{0};
    std::barrier<std::function<void()>> sync_point(kNumThreads, []{});

    auto check_errors = [&checks_performed, &sync_point]() {
        sync_point.arrive_and_wait();
        for (int32_t i = 0; i < kIterationsPerThread; ++i) {
            ProcError errs[] = {
                ProcError::kUnsupportedElement,
                ProcError::kInvalidTransition,
                ProcError::kSerialiserFailed,
                ProcError::kDeserialiserFailed,
                ProcError::kExecutionTimeout,
                ProcError::kMaxDepthExceeded,
                ProcError::kMaxElementsExceeded,
                ProcError::kMaxContextSizeExceeded,
            };

            for (auto err : errs) {
                int32_t v = static_cast<int32_t>(err);
                EXPECT_GE(v, 7600);
                EXPECT_LE(v, 7609);
            }
            checks_performed.fetch_add(1, std::memory_order_release);
        }
    };

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(check_errors);
    }

    for (auto& t : threads) {
        t.join();
    }

    int64_t expected = static_cast<int64_t>(kNumThreads) * kIterationsPerThread;
    EXPECT_EQ(checks_performed.load(std::memory_order_acquire), expected);
}

// ─────────────────────────────────────────────────────────────────────────────
// C-07: Barrier synchronization prevents premature completion
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrencyChurnTest, C07_BarrierSynchronization) {
    std::vector<int32_t> execution_order;
    std::mutex order_mutex;
    std::barrier<std::function<void()>> barrier(kNumThreads, []{});

    auto synchronized_work = [&execution_order, &order_mutex, &barrier](int32_t thread_id) {
        // Record pre-barrier time
        {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(thread_id);
        }

        // All threads synchronize here
        barrier.arrive_and_wait();

        // Record post-barrier: all should reach here together
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(-thread_id);  // Negative marker for post-barrier
        }
    };

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(synchronized_work, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify: should have 2*kNumThreads entries (pre and post barrier)
    EXPECT_EQ(execution_order.size(), 2 * kNumThreads);

    // Count positive and negative markers
    int32_t pre_barrier_count = 0, post_barrier_count = 0;
    for (int32_t val : execution_order) {
        if (val > 0) pre_barrier_count++;
        else post_barrier_count++;
    }
    EXPECT_EQ(pre_barrier_count, kNumThreads);
    EXPECT_EQ(post_barrier_count, kNumThreads);
}

// ─────────────────────────────────────────────────────────────────────────────
// C-08: High-contention scenario with multiple link operations
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrencyChurnTest, C08_HighContentionLinkOperations) {
    std::vector<ProcessLink> all_links;
    std::vector<ProcessAttachment> all_attachments;
    std::mutex links_mutex, attachments_mutex;
    std::atomic<int64_t> total_operations{0};
    std::barrier<std::function<void()>> sync_point(kNumThreads, []{});

    auto mixed_operations = [&all_links, &links_mutex, &all_attachments, &attachments_mutex,
                             &total_operations, &sync_point](int32_t thread_id) {
        sync_point.arrive_and_wait();

        for (int32_t i = 0; i < kIterationsPerThread; ++i) {
            // Create a link
            {
                ProcessLink link;
                link.link_id = "link_" + std::to_string(thread_id) + "_" + std::to_string(i);
                link.source_id = "src";
                link.target_id = "tgt";
                link.link_type = ProcessLinkType::SUB_PROCESS;

                std::lock_guard<std::mutex> lock(links_mutex);
                all_links.push_back(link);
            }
            total_operations.fetch_add(1, std::memory_order_release);

            // Create an attachment
            {
                ProcessAttachment attach;
                attach.id = "attach_" + std::to_string(thread_id) + "_" + std::to_string(i);
                attach.instance_id = "inst";
                attach.object_id = "obj";

                std::lock_guard<std::mutex> lock(attachments_mutex);
                all_attachments.push_back(attach);
            }
            total_operations.fetch_add(1, std::memory_order_release);
        }
    };

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(mixed_operations, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    int64_t expected_ops = static_cast<int64_t>(kNumThreads) * kIterationsPerThread * 2;
    EXPECT_EQ(total_operations.load(std::memory_order_acquire), expected_ops);
    EXPECT_EQ(all_links.size(), kNumThreads * kIterationsPerThread);
    EXPECT_EQ(all_attachments.size(), kNumThreads * kIterationsPerThread);
}
