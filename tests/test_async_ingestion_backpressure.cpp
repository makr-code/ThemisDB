/**
 * @file test_async_ingestion_backpressure.cpp
 * @brief Unit / integration tests for back-pressure in
 *        AsyncIngestionWorker::submitStream() and
 *        AsyncIngestionWorker::ingestStream() (CON-005).
 *
 * Verifies:
 *  1. ingestStream() returns a std::future<std::string> resolving to the
 *     primary ContentId on success.
 *  2. ingestStream() future propagates exceptions from failed jobs.
 *  3. submitStream() blocks the caller when queue depth >= max_queue_depth
 *     and unblocks when a worker dequeues a job.
 *  4. ingestStream() also blocks under back-pressure.
 *  5. Worker shutdown unblocks blocked submitters and delivers exceptions
 *     to pending futures.
 *  6. max_queue_depth config field defaults and is independent of
 *     max_queue_size.
 */

#include <gtest/gtest.h>
#include "content/async_ingestion_worker.h"
#include "content/content_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>

using namespace themis;
using namespace themis::content;
using namespace std::chrono_literals;

namespace {

// ============================================================================
// TestDatabase helper (mirrors the pattern in test_huggingface_plugin.cpp)
// ============================================================================

class TestDatabase {
public:
    TestDatabase() {
        std::random_device rd;
        path_ = std::filesystem::temp_directory_path() /
                ("themis_bp_test_" + std::to_string(rd()) + "_" +
                 std::to_string(std::chrono::steady_clock::now()
                                    .time_since_epoch()
                                    .count()));
        std::filesystem::create_directories(path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = path_.string();
        cfg.enable_wal = true;
        storage_ = std::make_shared<RocksDBWrapper>(cfg);
        if (!storage_->open()) {
            throw std::runtime_error("Failed to open test RocksDB");
        }

        vector_index_    = std::make_shared<VectorIndexManager>(*storage_);
        graph_index_     = std::make_shared<GraphIndexManager>(*storage_);
        secondary_index_ = std::make_shared<SecondaryIndexManager>(*storage_);

        content_manager_ = std::make_shared<ContentManager>(
            storage_, vector_index_, graph_index_, secondary_index_);
    }

    ~TestDatabase() {
        storage_->close();
        std::filesystem::remove_all(path_);
    }

    std::shared_ptr<ContentManager> getContentManager() { return content_manager_; }

private:
    std::filesystem::path path_;
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<ContentManager> content_manager_;
};

// ============================================================================
// Fixture
// ============================================================================

class AsyncIngestionBackpressureTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<TestDatabase>();
    }

    void TearDown() override {
        db_.reset();
    }

    /**
     * @brief Build a worker with a configurable max_queue_depth.
     */
    std::unique_ptr<AsyncIngestionWorker> makeWorker(
        size_t max_queue_depth = 2,
        size_t worker_threads  = 1
    ) {
        AsyncIngestionConfig cfg;
        cfg.worker_thread_count = worker_threads;
        cfg.max_queue_size      = max_queue_depth + 100; // Hard limit >> soft limit
        cfg.max_queue_depth     = max_queue_depth;
        cfg.enable_auto_cleanup = false;
        cfg.verbose_logging     = false;
        return std::make_unique<AsyncIngestionWorker>(db_->getContentManager(), cfg);
    }

    std::unique_ptr<TestDatabase> db_;
};

} // anonymous namespace

// ============================================================================
// Test 1 – ingestStream() returns a future that resolves to a ContentId
// ============================================================================

TEST_F(AsyncIngestionBackpressureTest, IngestStream_ReturnsFutureWithContentId) {
    auto worker = makeWorker(/*max_queue_depth=*/4, /*threads=*/1);

    // Register a fast custom handler so no real ContentManager I/O is needed
    std::atomic<int> handled{0};
    worker->registerJobHandler(IngestionJobType::STREAM_FILE, [&](IngestionJob& job) {
        ++handled;
        job.content_ids.push_back("test_content_id_" + std::to_string(handled.load()));
        job.processed_items = 1;
        job.progress = 1.0f;
    });

    worker->start();

    std::istringstream ss("hello world");
    auto fut = worker->ingestStream(ss, "hello.txt");

    ASSERT_TRUE(fut.valid());

    // Wait up to 5 s for the future to resolve
    ASSERT_EQ(fut.wait_for(5s), std::future_status::ready);

    std::string content_id = fut.get();
    EXPECT_FALSE(content_id.empty());
    EXPECT_EQ(content_id, "test_content_id_1");

    worker->stop(true);
}

// ============================================================================
// Test 2 – ingestStream() future propagates exceptions from a failed job
// ============================================================================

TEST_F(AsyncIngestionBackpressureTest, IngestStream_FuturePropagatesException) {
    auto worker = makeWorker(4, 1);

    worker->registerJobHandler(IngestionJobType::STREAM_FILE, [](IngestionJob& /*job*/) {
        throw std::runtime_error("simulated ingestion failure");
    });

    worker->start();

    std::istringstream ss("data");
    auto fut = worker->ingestStream(ss, "data.txt");

    ASSERT_TRUE(fut.valid());
    ASSERT_EQ(fut.wait_for(5s), std::future_status::ready);

    EXPECT_THROW(fut.get(), std::exception);

    worker->stop(true);
}

// ============================================================================
// Test 3 – submitStream() blocks when queue depth == max_queue_depth
// ============================================================================

TEST_F(AsyncIngestionBackpressureTest, SubmitStream_BlocksAtMaxQueueDepth) {
    // Worker processes each job slowly so the queue fills up
    auto worker = makeWorker(/*max_queue_depth=*/1, /*threads=*/1);

    worker->registerJobHandler(IngestionJobType::STREAM_FILE, [](IngestionJob& job) {
        std::this_thread::sleep_for(60ms);
        job.content_ids.push_back("id");
        job.processed_items = 1;
        job.progress = 1.0f;
    });

    worker->start();

    std::istringstream s1("data1");
    std::istringstream s2("data2");

    // First submit fills the queue (queue depth == 1 == max_queue_depth)
    auto id1 = worker->submitStream(s1, "file1.txt");
    EXPECT_FALSE(id1.empty());

    // Second submit must block because the queue is at capacity
    std::atomic<bool> submitted{false};
    auto submitter = std::thread([&] {
        auto id2 = worker->submitStream(s2, "file2.txt");
        (void)id2;
        submitted.store(true);
    });

    // After 30 ms the worker should NOT yet have dequeued the first job
    std::this_thread::sleep_for(30ms);
    EXPECT_FALSE(submitted.load())
        << "submitStream() must block while queue is at max_queue_depth";

    // After the worker processes the first job (~60 ms total), it should unblock
    submitter.join();
    EXPECT_TRUE(submitted.load());

    worker->stop(true);
}

// ============================================================================
// Test 4 – ingestStream() also blocks under back-pressure
// ============================================================================

TEST_F(AsyncIngestionBackpressureTest, IngestStream_BlocksAtMaxQueueDepth) {
    auto worker = makeWorker(/*max_queue_depth=*/1, /*threads=*/1);

    std::atomic<int> seq{0};
    worker->registerJobHandler(IngestionJobType::STREAM_FILE, [&](IngestionJob& job) {
        std::this_thread::sleep_for(60ms);
        job.content_ids.push_back("id_" + std::to_string(++seq));
        job.processed_items = 1;
        job.progress = 1.0f;
    });

    worker->start();

    std::istringstream s1("a");
    std::istringstream s2("b");

    // Fill the queue with the first job
    auto fut1 = worker->ingestStream(s1, "a.txt");

    std::atomic<bool> submitted{false};
    std::future<std::string> fut2;

    auto submitter = std::thread([&] {
        fut2 = worker->ingestStream(s2, "b.txt");
        submitted.store(true);
    });

    std::this_thread::sleep_for(30ms);
    EXPECT_FALSE(submitted.load())
        << "ingestStream() must block while queue is at max_queue_depth";

    submitter.join();
    EXPECT_TRUE(submitted.load());

    // Both futures must resolve successfully
    ASSERT_EQ(fut1.wait_for(5s), std::future_status::ready);
    EXPECT_NO_THROW(fut1.get());

    ASSERT_EQ(fut2.wait_for(5s), std::future_status::ready);
    EXPECT_NO_THROW(fut2.get());

    worker->stop(true);
}

// ============================================================================
// Test 5 – Worker shutdown unblocks blocked submitter and delivers exception
// ============================================================================

TEST_F(AsyncIngestionBackpressureTest, Shutdown_UnblocksBlockedIngestStream) {
    auto worker = makeWorker(/*max_queue_depth=*/1, /*threads=*/1);

    worker->registerJobHandler(IngestionJobType::STREAM_FILE, [](IngestionJob& job) {
        std::this_thread::sleep_for(200ms);
        job.content_ids.push_back("id");
        job.processed_items = 1;
        job.progress = 1.0f;
    });

    worker->start();

    std::istringstream s1("x");
    std::istringstream s2("y");

    // Fill the queue
    auto fut1 = worker->ingestStream(s1, "x.txt");

    std::atomic<bool> submitter_done{false};
    auto submitter = std::thread([&] {
        try {
            // This will block waiting for queue space
            auto fut2 = worker->ingestStream(s2, "y.txt");
            // If we get here, the future may resolve or carry an exception
            try {
                fut2.wait_for(5s);
                (void)fut2.get();
            } catch (...) {}
        } catch (const std::exception&) {
            // Expected: worker shut down while we were blocked
        }
        submitter_done.store(true);
    });

    // Give submitter time to reach the blocking wait
    std::this_thread::sleep_for(30ms);

    // Force-stop the worker
    worker->stop(false);

    submitter.join();
    EXPECT_TRUE(submitter_done.load());

    // fut1: either resolved or carries an exception – must not hang
    if (fut1.valid()) {
        try { (void)fut1.get(); } catch (...) {}
    }
}

// ============================================================================
// Test 6 – Config: max_queue_depth field and defaults
// ============================================================================

TEST(AsyncIngestionBackpressureConfigTest, Config_MaxQueueDepthField) {
    AsyncIngestionConfig cfg;
    cfg.max_queue_depth = 42;
    cfg.max_queue_size  = 1000;

    EXPECT_EQ(cfg.max_queue_depth, 42u);
    EXPECT_EQ(cfg.max_queue_size,  1000u);
}

TEST(AsyncIngestionBackpressureConfigTest, Config_DefaultMaxQueueDepthMatchesMaxQueueSize) {
    AsyncIngestionConfig default_cfg;
    // Both default to 1000 per the updated header
    EXPECT_EQ(default_cfg.max_queue_depth, 1000u);
    EXPECT_EQ(default_cfg.max_queue_size,  1000u);
}

// ============================================================================
// Test 7 – getStatistics() exposes max_queue_depth
// ============================================================================

TEST_F(AsyncIngestionBackpressureTest, Statistics_ExposesMaxQueueDepth) {
    auto worker = makeWorker(/*max_queue_depth=*/37, /*threads=*/1);
    worker->start();

    auto stats = worker->getStatistics();
    EXPECT_EQ(stats["max_queue_depth"].get<size_t>(), 37u);
    EXPECT_TRUE(stats.contains("max_queue_size"));

    worker->stop(true);
}

// ============================================================================
// Test 8 – stop(true) drains all queued ingestStream() futures before exit
// ============================================================================

TEST_F(AsyncIngestionBackpressureTest, StopGraceful_DrainsPendingFutures) {
    // Single slow worker; we enqueue 3 jobs but worker starts slowly.
    // stop(true) must not exit until all queued jobs complete.
    auto worker = makeWorker(/*max_queue_depth=*/10, /*threads=*/1);

    std::atomic<int> processed{0};
    worker->registerJobHandler(IngestionJobType::STREAM_FILE, [&](IngestionJob& job) {
        std::this_thread::sleep_for(20ms);
        ++processed;
        job.content_ids.push_back("id_" + std::to_string(processed.load()));
        job.processed_items = 1;
        job.progress = 1.0f;
    });

    worker->start();

    std::istringstream s1("a"), s2("b"), s3("c");
    auto fut1 = worker->ingestStream(s1, "a.txt");
    auto fut2 = worker->ingestStream(s2, "b.txt");
    auto fut3 = worker->ingestStream(s3, "c.txt");

    // Graceful stop – must process all 3 jobs
    worker->stop(true);

    // All futures must be ready now (not hanging)
    ASSERT_EQ(fut1.wait_for(0s), std::future_status::ready);
    ASSERT_EQ(fut2.wait_for(0s), std::future_status::ready);
    ASSERT_EQ(fut3.wait_for(0s), std::future_status::ready);

    EXPECT_NO_THROW(fut1.get());
    EXPECT_NO_THROW(fut2.get());
    EXPECT_NO_THROW(fut3.get());

    EXPECT_EQ(processed.load(), 3);
}

// ============================================================================
// Test 9 – Backpressure metrics: events_total and queue_depth_high_watermark
// ============================================================================

TEST_F(AsyncIngestionBackpressureTest, Statistics_BackpressureMetricsCountedOnOverload) {
    // Use max_queue_depth=2 with a gating handler so we can deterministically
    // reach capacity under queue+inflight accounting:
    //   - job1 is in flight (1)
    //   - job2 is queued (1)
    // => total load 2 == max_queue_depth, so job3 triggers back-pressure.
    auto worker = makeWorker(/*max_queue_depth=*/2, /*threads=*/1);

    // Gate that blocks the worker in the handler until we release it.
    // This ensures job1 is actively being processed (already dequeued) and
    // job2 is in the queue (at max_queue_depth=1) when we measure metrics.
    std::mutex       gate_mutex;
    std::condition_variable gate_cv;
    bool             gate_open = false;
    std::atomic<bool> job1_started{false};

    worker->registerJobHandler(IngestionJobType::STREAM_FILE, [&](IngestionJob& job) {
        job1_started.store(true);
        gate_cv.notify_all();
        // Block until test releases the gate
        std::unique_lock<std::mutex> lock(gate_mutex);
        gate_cv.wait(lock, [&] { return gate_open; });
        job.content_ids.push_back("id");
        job.processed_items = 1;
        job.progress = 1.0f;
    });

    worker->start();

    // Verify initial state: no back-pressure events yet
    {
        auto stats = worker->getStatistics();
        ASSERT_TRUE(stats.contains("backpressure"));
        EXPECT_EQ(stats["backpressure"]["events_total"].get<uint64_t>(), 0u);
        EXPECT_EQ(stats["backpressure"]["queue_depth_high_watermark"].get<uint64_t>(), 0u);
    }

    std::istringstream s1("data1");
    std::istringstream s2("data2");

    // First submitStream: queue is empty → no back-pressure event, enqueues immediately
    auto id1 = worker->submitStream(s1, "file1.txt");
    EXPECT_FALSE(id1.empty());

    // Wait until the worker has dequeued job1 and started processing it (holding the gate)
    {
        std::unique_lock<std::mutex> lock(gate_mutex);
        ASSERT_TRUE(gate_cv.wait_for(lock, 5s, [&] { return job1_started.load(); }))
            << "Worker should have started processing job1 within 5 s";
    }
    // Now: job1 is IN FLIGHT (gate blocking worker), queue is EMPTY.
    // Submit job2 — total load becomes 2 (1 inflight + 1 queued), still accepted
    // because the waiter checks before push and load was 1 < max_queue_depth=2.
    auto id2 = worker->submitStream(s2, "file2.txt");
    EXPECT_FALSE(id2.empty());

    // Now queue holds job2 while job1 remains inflight: total load is at capacity.
    // Submit job3 from a thread — this MUST trigger exactly one back-pressure event.
    std::istringstream s3("data3");
    auto submitter3 = std::async(std::launch::async, [&]() -> bool {
        try {
            auto id3 = worker->submitStream(s3, "file3.txt");
            return !id3.empty();
        } catch (const std::exception&) {
            // Possible if worker shuts down while waiting
            return false;
        }
    });

    // Give the submitter thread time to reach and enter the back-pressure wait
    std::this_thread::sleep_for(30ms);

    // While gate is closed the total load stays at capacity; submitter3 must be blocked
    EXPECT_EQ(submitter3.wait_for(0ms), std::future_status::timeout)
        << "job3 submitter should still be blocked under back-pressure";

    {
        auto stats = worker->getStatistics();
        EXPECT_GE(stats["backpressure"]["events_total"].get<uint64_t>(), 1u)
            << "At least one back-pressure event must be recorded when queue reaches max_queue_depth";
        EXPECT_GE(stats["backpressure"]["queue_depth_high_watermark"].get<uint64_t>(), 1u)
            << "queue_depth_high_watermark must reflect the peak queue depth observed";
    }

    // Release the gate so the worker can complete job1 and eventually unblock the submitter
    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        gate_open = true;
    }
    gate_cv.notify_all();

    ASSERT_EQ(submitter3.wait_for(5s), std::future_status::ready)
        << "job3 submitter should unblock after worker capacity is released";
    (void)submitter3.get();

    worker->stop(true);
}
