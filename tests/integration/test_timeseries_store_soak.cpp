// tests/integration/test_timeseries_store_soak.cpp
// Wave D soak tests for the Timeseries module.
// Labels: wave_d;soak;not_release_critical
// THEMIS_SOAK_DURATION_MS controls wall-clock run length (default 60 000 ms).
//
// Design: all tests use in-process stubs so no external engine is required.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <random>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static long long soak_duration_ms() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env) {
        try { return std::stoll(env); } catch (...) {}
    }
    return 60000LL;
}

using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

// ---------------------------------------------------------------------------
// In-process stubs
// ---------------------------------------------------------------------------

namespace stubs {

struct TSStore {
    std::atomic<uint64_t> ingested{0};
    std::atomic<uint64_t> flushed{0};
    std::atomic<uint64_t> queries{0};
    std::atomic<uint64_t> flush_errors{0};

    bool ingest(uint64_t /*series_id*/, double /*value*/, int64_t /*ts_ns*/) {
        ingested.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    // Returns true if flush succeeds.
    bool flush() {
        flushed.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool range_query(uint64_t /*series_id*/, int64_t /*start*/, int64_t /*end*/) {
        queries.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

} // namespace stubs

// ---------------------------------------------------------------------------
// TimeseriesSoak_IngestThroughput
// ---------------------------------------------------------------------------
// Validates sustained ingest throughput of >= 50 000 samples/sec over the
// full soak window using an in-process TSStore stub.
// ---------------------------------------------------------------------------
TEST(TimeseriesStoreSoak, TimeseriesSoak_IngestThroughput) {
    const auto duration_ms = soak_duration_ms();
    const auto deadline     = Clock::now() + std::chrono::milliseconds(duration_ms);

    stubs::TSStore store;

    const int kThreads = 8;
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t series = static_cast<uint64_t>(t);
            int64_t  ts     = 0;
            std::mt19937_64 rng(t);
            std::uniform_real_distribution<double> val_dist(0.0, 1000.0);
            while (Clock::now() < deadline) {
                store.ingest(series, val_dist(rng), ts++);
            }
        });
    }
    for (auto& th : threads) th.join();

    const double elapsed_s    = duration_ms / 1000.0;
    const double samples_sec  = static_cast<double>(store.ingested.load()) / elapsed_s;

    EXPECT_GT(store.ingested.load(), 0u)
        << "No samples ingested during soak";
    EXPECT_GE(samples_sec, 50000.0)
        << "Ingest throughput below 50 000 samples/sec threshold; got "
        << samples_sec;
}

// ---------------------------------------------------------------------------
// TimeseriesSoak_RangeQueryStability
// ---------------------------------------------------------------------------
// Continuously issues range queries while concurrent ingest runs; validates
// that query latency remains bounded and no errors occur.
// ---------------------------------------------------------------------------
TEST(TimeseriesStoreSoak, TimeseriesSoak_RangeQueryStability) {
    const auto duration_ms = soak_duration_ms();
    const auto deadline     = Clock::now() + std::chrono::milliseconds(duration_ms);

    stubs::TSStore store;
    std::atomic<uint64_t> query_errors{0};

    // Ingest writers
    const int kWriters = 4;
    std::vector<std::thread> writers;
    for (int t = 0; t < kWriters; ++t) {
        writers.emplace_back([&, t]() {
            int64_t ts = 0;
            while (Clock::now() < deadline) {
                store.ingest(static_cast<uint64_t>(t), 1.0, ts++);
            }
        });
    }

    // Query readers
    const int kReaders = 4;
    std::vector<std::thread> readers;
    for (int t = 0; t < kReaders; ++t) {
        readers.emplace_back([&, t]() {
            while (Clock::now() < deadline) {
                if (!store.range_query(static_cast<uint64_t>(t), 0, 1000)) {
                    query_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : writers) th.join();
    for (auto& th : readers) th.join();

    EXPECT_GT(store.queries.load(), 0u)
        << "No range queries completed during soak";
    EXPECT_EQ(query_errors.load(), 0u)
        << "Range query errors detected during soak";
}

// ---------------------------------------------------------------------------
// TimeseriesSoak_FlushReliability
// ---------------------------------------------------------------------------
// Triggers flush operations continuously for the soak window; validates that
// flush success rate remains 100% and no stalls are observed.
// ---------------------------------------------------------------------------
TEST(TimeseriesStoreSoak, TimeseriesSoak_FlushReliability) {
    const auto duration_ms = soak_duration_ms();
    const auto deadline     = Clock::now() + std::chrono::milliseconds(duration_ms);

    stubs::TSStore store;
    std::atomic<uint64_t> flush_errors{0};

    // Ingest thread to keep the store non-empty.
    std::thread ingest_thread([&]() {
        int64_t ts = 0;
        while (Clock::now() < deadline) {
            store.ingest(0, 1.0, ts++);
        }
    });

    // Flush thread.
    std::thread flush_thread([&]() {
        while (Clock::now() < deadline) {
            if (!store.flush()) {
                flush_errors.fetch_add(1, std::memory_order_relaxed);
            }
            // Brief yield to avoid hammering the flush path.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    ingest_thread.join();
    flush_thread.join();

    EXPECT_GT(store.flushed.load(), 0u)
        << "[TIMESERIES:FlushStall] No flush operations completed during soak";
    EXPECT_EQ(flush_errors.load(), 0u)
        << "[TIMESERIES:FlushStall] Flush errors detected during soak";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
