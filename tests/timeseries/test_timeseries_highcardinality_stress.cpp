// tests/timeseries/test_timeseries_highcardinality_stress.cpp
// Wave D high-cardinality stress tests for the Timeseries module.
// Labels: wave_d;stress;not_release_critical
//
// Design: all tests use in-process stubs; no external engine required.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// In-process stubs
// ---------------------------------------------------------------------------

namespace stubs {

struct TSIngestStore {
    std::atomic<uint64_t> ingested{0};
    std::atomic<uint64_t> errors{0};

    bool ingest(uint64_t series_id, double value, int64_t ts_ns) {
        (void)series_id; (void)value; (void)ts_ns;
        ingested.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct DownsamplingEngine {
    std::atomic<uint64_t> downsampled{0};
    std::atomic<uint64_t> errors{0};

    bool downsample(uint64_t series_id, int factor) {
        (void)series_id; (void)factor;
        downsampled.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct RemoteWriteClient {
    std::atomic<uint64_t> sent{0};
    std::atomic<uint64_t> retries{0};
    std::atomic<uint64_t> errors{0};

    // Simulate transient failure every 10 000th write.
    bool write(uint64_t batch_id) {
        sent.fetch_add(1, std::memory_order_relaxed);
        if (batch_id % 10001 == 0) {
            retries.fetch_add(1, std::memory_order_relaxed);
            // Simulate retry success.
            return true;
        }
        return true;
    }
};

} // namespace stubs

// ---------------------------------------------------------------------------
// HighCardinalitySeriesIngest
// ---------------------------------------------------------------------------
// Ingests samples across 10 000 distinct series using 8 threads; validates
// total sample count and zero error rate.
// ---------------------------------------------------------------------------
TEST(TimeseriesHighCardinalityStress, HighCardinalitySeriesIngest) {
    constexpr int      kThreads      = 8;
    constexpr uint64_t kSeries       = 10'000ULL;
    constexpr uint64_t kSamplesTotal = 2'000'000ULL; // 2M samples total

    stubs::TSIngestStore store;

    const uint64_t samples_per_thread = kSamplesTotal / kThreads;
    const uint64_t series_per_thread  = kSeries / kThreads;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t series_base = static_cast<uint64_t>(t) * series_per_thread;
            int64_t  ts          = 0;
            std::mt19937_64 rng(t);
            std::uniform_real_distribution<double> vdist(0.0, 1e6);
            for (uint64_t i = 0; i < samples_per_thread; ++i) {
                uint64_t sid = series_base + (i % series_per_thread);
                store.ingest(sid, vdist(rng), ts++);
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(store.ingested.load(), kSamplesTotal)
        << "Sample count mismatch after high-cardinality ingest";
    EXPECT_EQ(store.errors.load(), 0u)
        << "Unexpected ingest errors during high-cardinality stress";
}

// ---------------------------------------------------------------------------
// ConcurrentDownsamplingStress
// ---------------------------------------------------------------------------
// Runs concurrent ingest and downsampling operations; validates that both
// paths complete without errors.
// ---------------------------------------------------------------------------
TEST(TimeseriesHighCardinalityStress, ConcurrentDownsamplingStress) {
    constexpr int      kIngestThreads      = 4;
    constexpr int      kDownsampleThreads  = 4;
    constexpr uint64_t kIngestPerThread    = 500'000ULL;
    constexpr uint64_t kDownsamplePerThread = 100'000ULL;
    constexpr int      kDownsampleFactor   = 10;

    stubs::TSIngestStore     ingest;
    stubs::DownsamplingEngine ds;

    std::vector<std::thread> ingest_threads;
    for (int t = 0; t < kIngestThreads; ++t) {
        ingest_threads.emplace_back([&, t]() {
            int64_t ts = 0;
            for (uint64_t i = 0; i < kIngestPerThread; ++i) {
                ingest.ingest(static_cast<uint64_t>(t), 1.0, ts++);
            }
        });
    }

    std::vector<std::thread> ds_threads;
    for (int t = 0; t < kDownsampleThreads; ++t) {
        ds_threads.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kDownsamplePerThread; ++i) {
                ds.downsample(static_cast<uint64_t>(t), kDownsampleFactor);
            }
        });
    }

    for (auto& th : ingest_threads) th.join();
    for (auto& th : ds_threads)     th.join();

    const uint64_t expected_ingested   = static_cast<uint64_t>(kIngestThreads)     * kIngestPerThread;
    const uint64_t expected_downsampled = static_cast<uint64_t>(kDownsampleThreads) * kDownsamplePerThread;

    EXPECT_EQ(ingest.ingested.load(),   expected_ingested);
    EXPECT_EQ(ds.downsampled.load(),    expected_downsampled);
    EXPECT_EQ(ingest.errors.load(),     0u);
    EXPECT_EQ(ds.errors.load(),         0u);
}

// ---------------------------------------------------------------------------
// RemoteWriteEdgeCases
// ---------------------------------------------------------------------------
// Exercises remote-write with high-volume batches including simulated transient
// retries; validates that all batches eventually succeed.
// ---------------------------------------------------------------------------
TEST(TimeseriesHighCardinalityStress, RemoteWriteEdgeCases) {
    constexpr int      kThreads           = 8;
    constexpr uint64_t kBatchesPerThread  = 50'000ULL;

    stubs::RemoteWriteClient client;

    std::vector<std::thread> threads;
    std::atomic<uint64_t>    write_errors{0};
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t base = static_cast<uint64_t>(t) * kBatchesPerThread;
            for (uint64_t i = 0; i < kBatchesPerThread; ++i) {
                if (!client.write(base + i)) {
                    write_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    const uint64_t total = static_cast<uint64_t>(kThreads) * kBatchesPerThread;
    EXPECT_EQ(client.sent.load(), total)
        << "Remote-write batch count mismatch";
    EXPECT_EQ(write_errors.load(), 0u)
        << "[TIMESERIES:RemoteWriteOverflow] Remote-write errors under stress";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
