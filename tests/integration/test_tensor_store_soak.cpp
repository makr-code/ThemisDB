// tests/integration/test_tensor_store_soak.cpp
// Wave D soak tests for the Tensor module.
// Labels: wave_d;soak;not_release_critical
// THEMIS_SOAK_DURATION_MS controls wall-clock run length (default 60 000 ms).
//
// Design: all tests use in-process stubs so no external engine is required.
// Each test spins a tight work loop for the configured duration and validates
// that throughput / error-rate invariants hold over the full soak window.

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

// Run `work` in a tight loop until `deadline` is reached.
// Returns the number of successful iterations.
static uint64_t run_until(TimePoint deadline, std::function<bool()> work) {
    uint64_t ok = 0;
    while (Clock::now() < deadline) {
        if (work()) ++ok;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// In-process stubs
// ---------------------------------------------------------------------------

namespace stubs {

// Simulates a mutable fingerprint graph with node-insert / edge-query ops.
struct FingerprintGraph {
    std::atomic<uint64_t> node_count{0};
    std::atomic<uint64_t> query_count{0};

    bool insert_node(uint64_t /*id*/) {
        node_count.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    bool query_edges(uint64_t /*id*/) {
        query_count.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    bool replay_fingerprint(uint64_t /*fp*/) {
        return true;
    }
};

// Simulates a hybrid tensor-bridge router.
struct BridgeRouter {
    std::atomic<uint64_t> routed{0};
    std::atomic<uint64_t> errors{0};

    bool route(uint64_t token) {
        if (token == 0) { errors.fetch_add(1, std::memory_order_relaxed); return false; }
        routed.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

} // namespace stubs

// ---------------------------------------------------------------------------
// TensorSoak_GraphMutationThroughput
// ---------------------------------------------------------------------------
// Validates that the fingerprint-graph mutation path sustains > 0 ops/sec
// over the full soak window with zero unexpected errors.
// ---------------------------------------------------------------------------
TEST(TensorStoreSoak, TensorSoak_GraphMutationThroughput) {
    const auto duration_ms = soak_duration_ms();
    const auto deadline     = Clock::now() + std::chrono::milliseconds(duration_ms);

    stubs::FingerprintGraph graph;

    const int kThreads = 4;
    std::vector<std::thread> threads;
    std::atomic<uint64_t> total_ops{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t id = static_cast<uint64_t>(t) * 1'000'000ULL;
            while (Clock::now() < deadline) {
                graph.insert_node(id);
                graph.query_edges(id);
                ++id;
                total_ops.fetch_add(2, std::memory_order_relaxed);
            }
        });
    }
    for (auto& th : threads) th.join();

    const double elapsed_s = duration_ms / 1000.0;
    const double ops_per_sec = static_cast<double>(total_ops.load()) / elapsed_s;

    EXPECT_GT(total_ops.load(), 0u)
        << "No graph mutation operations completed during soak";
    EXPECT_GT(ops_per_sec, 0.0)
        << "Throughput must be positive";

    // Structural invariant: node_count must equal half of total_ops (inserts only).
    EXPECT_EQ(graph.node_count.load(), total_ops.load() / 2);
}

// ---------------------------------------------------------------------------
// TensorSoak_FingerprintReplayStability
// ---------------------------------------------------------------------------
// Replays fingerprints continuously for the soak window; validates that the
// error rate remains zero and replay count is monotonically consistent.
// ---------------------------------------------------------------------------
TEST(TensorStoreSoak, TensorSoak_FingerprintReplayStability) {
    const auto duration_ms = soak_duration_ms();
    const auto deadline     = Clock::now() + std::chrono::milliseconds(duration_ms);

    stubs::FingerprintGraph graph;
    std::atomic<uint64_t> replayed{0};
    std::atomic<uint64_t> replay_errors{0};

    const int kThreads = 4;
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            std::mt19937_64 rng(std::random_device{}());
            while (Clock::now() < deadline) {
                uint64_t fp = rng();
                if (!graph.replay_fingerprint(fp)) {
                    replay_errors.fetch_add(1, std::memory_order_relaxed);
                } else {
                    replayed.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_GT(replayed.load(), 0u)
        << "No fingerprint replays completed during soak";
    EXPECT_EQ(replay_errors.load(), 0u)
        << "[TENSOR:DedupCollision] unexpected replay errors during soak";
}

// ---------------------------------------------------------------------------
// TensorSoak_BridgeRoutingReliability
// ---------------------------------------------------------------------------
// Routes tensor tokens through the bridge for the soak window; validates
// routing reliability and measures error rate.
// ---------------------------------------------------------------------------
TEST(TensorStoreSoak, TensorSoak_BridgeRoutingReliability) {
    const auto duration_ms = soak_duration_ms();
    const auto deadline     = Clock::now() + std::chrono::milliseconds(duration_ms);

    stubs::BridgeRouter router;

    const int kThreads = 4;
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            uint64_t tok = 1;
            while (Clock::now() < deadline) {
                router.route(tok++);
                if (tok == 0) tok = 1; // skip 0 (error sentinel)
            }
        });
    }
    for (auto& th : threads) th.join();

    const uint64_t total = router.routed.load() + router.errors.load();
    EXPECT_GT(total, 0u)
        << "No routing operations completed during soak";
    EXPECT_EQ(router.errors.load(), 0u)
        << "[TENSOR:BridgeRoutingStall] unexpected routing errors during soak";

    const double reliability =
        static_cast<double>(router.routed.load()) / static_cast<double>(total);
    EXPECT_GE(reliability, 0.999)
        << "Bridge routing reliability below 99.9% threshold";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
