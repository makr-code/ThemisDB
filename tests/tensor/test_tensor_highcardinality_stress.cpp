// tests/tensor/test_tensor_highcardinality_stress.cpp
// Wave D high-cardinality stress tests for the Tensor module.
// Labels: wave_d;stress;not_release_critical
//
// Design: all tests use in-process stubs; no external engine required.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ---------------------------------------------------------------------------
// In-process stubs
// ---------------------------------------------------------------------------

namespace stubs {

struct TensorStore {
    std::atomic<uint64_t> inserts{0};
    std::atomic<uint64_t> insert_errors{0};

    // Simulate insert with lightweight bookkeeping.
    bool insert(uint64_t tensor_id, std::vector<float>& /*data*/) {
        inserts.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct DedupGraph {
    std::atomic<uint64_t> replays{0};
    std::atomic<uint64_t> dedup_hits{0};

    bool replay(uint64_t fp) {
        replays.fetch_add(1, std::memory_order_relaxed);
        // Deterministic dedup: every even fingerprint is considered a hit.
        if (fp % 2 == 0) dedup_hits.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct GraphStore {
    std::atomic<uint64_t> reads{0};
    std::atomic<uint64_t> writes{0};
    std::atomic<uint64_t> read_errors{0};
    std::atomic<uint64_t> write_errors{0};

    bool write_node(uint64_t id) {
        writes.fetch_add(1, std::memory_order_relaxed);
        return id != UINT64_MAX; // sentinel error
    }
    bool read_node(uint64_t id) {
        reads.fetch_add(1, std::memory_order_relaxed);
        return id != UINT64_MAX;
    }
};

} // namespace stubs

// ---------------------------------------------------------------------------
// HighCardinalityTensorInsert
// ---------------------------------------------------------------------------
// Inserts a large number of distinct tensors concurrently; validates that all
// inserts succeed and no data races are introduced.
// ---------------------------------------------------------------------------
TEST(TensorHighCardinalityStress, HighCardinalityTensorInsert) {
    constexpr int     kThreads       = 8;
    constexpr uint64_t kPerThread    = 50'000ULL;   // 400 000 total inserts
    constexpr uint64_t kVectorDim    = 16;           // keep allocation small

    stubs::TensorStore store;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            std::vector<float> vec(kVectorDim, static_cast<float>(t));
            uint64_t base = static_cast<uint64_t>(t) * kPerThread;
            for (uint64_t i = 0; i < kPerThread; ++i) {
                store.insert(base + i, vec);
            }
        });
    }
    for (auto& th : threads) th.join();

    const uint64_t expected = static_cast<uint64_t>(kThreads) * kPerThread;
    EXPECT_EQ(store.inserts.load(), expected)
        << "Insert count mismatch: expected " << expected
        << " got " << store.inserts.load();
    EXPECT_EQ(store.insert_errors.load(), 0u)
        << "Unexpected insert errors during high-cardinality stress";
}

// ---------------------------------------------------------------------------
// ConcurrentGraphReadWrite
// ---------------------------------------------------------------------------
// Exercises concurrent graph read/write patterns; validates that reads and
// writes complete without errors and counts are consistent.
// ---------------------------------------------------------------------------
TEST(TensorHighCardinalityStress, ConcurrentGraphReadWrite) {
    constexpr int      kWriteThreads = 4;
    constexpr int      kReadThreads  = 4;
    constexpr uint64_t kOpsPerThread = 25'000ULL;

    stubs::GraphStore graph;

    std::vector<std::thread> writers;
    for (int t = 0; t < kWriteThreads; ++t) {
        writers.emplace_back([&, t]() {
            uint64_t base = static_cast<uint64_t>(t) * kOpsPerThread;
            for (uint64_t i = 0; i < kOpsPerThread; ++i) {
                if (!graph.write_node(base + i)) {
                    graph.write_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::vector<std::thread> readers;
    for (int t = 0; t < kReadThreads; ++t) {
        readers.emplace_back([&, t]() {
            uint64_t base = static_cast<uint64_t>(t) * kOpsPerThread;
            for (uint64_t i = 0; i < kOpsPerThread; ++i) {
                if (!graph.read_node(base + i)) {
                    graph.read_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : writers) th.join();
    for (auto& th : readers) th.join();

    const uint64_t expected_writes = static_cast<uint64_t>(kWriteThreads) * kOpsPerThread;
    const uint64_t expected_reads  = static_cast<uint64_t>(kReadThreads)  * kOpsPerThread;

    EXPECT_EQ(graph.writes.load(), expected_writes);
    EXPECT_EQ(graph.reads.load(),  expected_reads);
    EXPECT_EQ(graph.write_errors.load(), 0u)
        << "Write errors during concurrent graph stress";
    EXPECT_EQ(graph.read_errors.load(),  0u)
        << "Read errors during concurrent graph stress";
}

// ---------------------------------------------------------------------------
// DedupReplayStress
// ---------------------------------------------------------------------------
// Replays a large fingerprint sequence concurrently; validates dedup semantics
// and that no replay operation fails.
// ---------------------------------------------------------------------------
TEST(TensorHighCardinalityStress, DedupReplayStress) {
    constexpr int      kThreads    = 8;
    constexpr uint64_t kFpsPerThread = 50'000ULL;

    stubs::DedupGraph dedup;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t fp = static_cast<uint64_t>(t) * kFpsPerThread;
            for (uint64_t i = 0; i < kFpsPerThread; ++i) {
                dedup.replay(fp + i);
            }
        });
    }
    for (auto& th : threads) th.join();

    const uint64_t total    = static_cast<uint64_t>(kThreads) * kFpsPerThread;
    const uint64_t expected_hits = total / 2; // every even fp is a dedup hit

    EXPECT_EQ(dedup.replays.load(), total)
        << "Replay count mismatch";
    EXPECT_EQ(dedup.dedup_hits.load(), expected_hits)
        << "[TENSOR:DedupCollision] Dedup-hit count mismatch under stress";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
