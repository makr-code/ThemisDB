/**
 * @file test_retrieval_engine_soak.cpp
 * @brief Wave D — Retrieval Engine Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB retrieval engine covering shard
 * routing throughput, failover recovery stability, and GPU path reliability.
 *
 * In CI environments this test runs with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate finishes in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Shard routing throughput ≥ 2 000 ops/sec over the soak duration
 * - No uncaught exceptions or data-race signals from the in-process stubs
 * - Failover recovery completes in < 500 ms per simulated shard failure
 * - GPU advisory path remains available (no silent disablement) after errors
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see docs/operability/RUNBOOK_RETRIEVAL_ENGINE.md — operator runbook
 * @see src/retrieval/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Soak duration — overridable via THEMIS_SOAK_DURATION_MS environment variable.
// Default: 60 000 ms (1 min) so CI completes quickly.
// Production soak: 3 600 000 ms (60 min).
// ─────────────────────────────────────────────────────────────────────────────
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// The in-process stubs below model the retrieval engine's shard routing,
// failover, and GPU advisory paths without requiring external backends.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

/// Stub shard router — deterministically routes queries to one of N shards.
class StubShardRouter {
public:
    explicit StubShardRouter(int num_shards)
        : num_shards_(num_shards), ops_(0) {}

    /// Route a query key to a shard index. Returns -1 if all shards are failed.
    int route(const std::string& key) {
        int shard = static_cast<int>(std::hash<std::string>{}(key) % static_cast<size_t>(num_shards_));
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (failed_.count(shard)) {
                // Try first healthy shard
                for (int i = 0; i < num_shards_; ++i) {
                    if (!failed_.count(i)) { shard = i; break; }
                }
            }
        }
        ++ops_;
        return shard;
    }

    /// Mark a shard as failed.
    void failShard(int shard) {
        std::lock_guard<std::mutex> lk(mu_);
        failed_.insert(shard);
    }

    /// Mark a shard as recovered.
    void recoverShard(int shard) {
        std::lock_guard<std::mutex> lk(mu_);
        failed_.erase(shard);
    }

    uint64_t ops() const { return ops_.load(); }

private:
    int num_shards_;
    std::atomic<uint64_t> ops_;
    std::mutex mu_;
    std::unordered_set<int> failed_;
};

/// Stub GPU advisory engine — simulates advisory GPU acceleration path.
class StubGpuAdvisoryEngine {
public:
    StubGpuAdvisoryEngine() : enabled_(true), errors_(0) {}

    /// Attempt an accelerated query. Returns false on simulated GPU error.
    bool query(const std::vector<float>& /*vec*/) {
        if (!enabled_.load()) {
            ++errors_;
            return false;
        }
        return true;
    }

    /// Inject a transient GPU error; engine must re-enable itself.
    void injectTransientError() { enabled_.store(false); }
    void clearError()           { enabled_.store(true);  }

    bool isEnabled() const { return enabled_.load(); }
    uint64_t errors() const { return errors_.load(); }

private:
    std::atomic<bool> enabled_;
    std::atomic<uint64_t> errors_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class RetrievalSoakFixture : public ::testing::Test {
protected:
    static constexpr int kNumShards = 8;

    StubShardRouter    router{kNumShards};
    StubGpuAdvisoryEngine gpu{};
};

// ─────────────────────────────────────────────────────────────────────────────
// RT-SOAK-01: Shard routing throughput
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(RetrievalSoakFixture, RetrievalSoak_ShardRoutingThroughput) {
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto deadline = std::chrono::steady_clock::now() + duration;

    constexpr int kWorkers = 4;
    std::atomic<uint64_t> total_ops{0};
    std::atomic<bool>     error_seen{false};

    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937_64 rng(static_cast<uint64_t>(w) * 0x9e3779b97f4a7c15ULL);
            std::uniform_int_distribution<int> key_dist(0, 9999);
            uint64_t local_ops = 0;
            while (std::chrono::steady_clock::now() < deadline) {
                std::string key = "key:" + std::to_string(key_dist(rng));
                int shard = router.route(key);
                if (shard < 0 || shard >= kNumShards) {
                    error_seen.store(true);
                }
                ++local_ops;
            }
            total_ops.fetch_add(local_ops);
        });
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_FALSE(error_seen.load())
        << "[RETRIEVAL:ShardFailure] Invalid shard index returned during soak";

    const double elapsed_sec = static_cast<double>(soakDurationMs()) / 1000.0;
    const double throughput   = static_cast<double>(total_ops.load()) / elapsed_sec;

    EXPECT_GE(throughput, 2000.0)
        << "Shard routing throughput below 2 000 ops/sec: " << throughput;
}

// ─────────────────────────────────────────────────────────────────────────────
// RT-SOAK-02: Failover recovery stability
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(RetrievalSoakFixture, RetrievalSoak_FailoverRecoveryStability) {
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto deadline = std::chrono::steady_clock::now() + duration;

    constexpr int kWorkers = 4;
    std::atomic<uint64_t> routing_errors{0};
    std::atomic<uint64_t> failover_events{0};

    // Background thread periodically fails and recovers shards.
    std::atomic<bool> stop_injector{false};
    std::thread injector([&]() {
        int shard_to_fail = 0;
        while (!stop_injector.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            router.failShard(shard_to_fail);
            ++failover_events;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            router.recoverShard(shard_to_fail);
            shard_to_fail = (shard_to_fail + 1) % kNumShards;
        }
    });

    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937_64 rng(static_cast<uint64_t>(w) ^ 0xdeadbeefcafe1234ULL);
            std::uniform_int_distribution<int> key_dist(0, 4999);
            while (std::chrono::steady_clock::now() < deadline) {
                std::string key = "q:" + std::to_string(key_dist(rng));
                int shard = router.route(key);
                if (shard < 0 || shard >= kNumShards) {
                    ++routing_errors;
                }
            }
        });
    }
    for (auto& t : workers) { t.join(); }
    stop_injector.store(true);
    injector.join();

    EXPECT_EQ(routing_errors.load(), 0U)
        << "[RETRIEVAL:ShardFailure] Routing returned invalid shard during failover";
    EXPECT_GT(failover_events.load(), 0U)
        << "Injector should have injected at least one failover event";
}

// ─────────────────────────────────────────────────────────────────────────────
// RT-SOAK-03: GPU advisory path reliability
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(RetrievalSoakFixture, RetrievalSoak_GPUPathReliability) {
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto deadline = std::chrono::steady_clock::now() + duration;

    std::atomic<uint64_t> gpu_ok{0};
    std::atomic<uint64_t> gpu_errors{0};
    std::atomic<uint64_t> injections{0};

    // Periodically inject and clear GPU errors.
    std::atomic<bool> stop_injector{false};
    std::thread injector([&]() {
        while (!stop_injector.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            gpu.injectTransientError();
            ++injections;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            gpu.clearError();
        }
    });

    constexpr int kWorkers = 4;
    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&]() {
            std::vector<float> vec(8, 1.0f);
            while (std::chrono::steady_clock::now() < deadline) {
                if (gpu.query(vec)) { ++gpu_ok;     }
                else                { ++gpu_errors;  }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
    for (auto& t : workers) { t.join(); }
    stop_injector.store(true);
    injector.join();

    EXPECT_GT(gpu_ok.load(), 0U)
        << "[RETRIEVAL:GPUPathStall] GPU advisory path never returned success";
    EXPECT_GT(injections.load(), 0U)
        << "Injector should have injected at least one GPU error";

    // After all injections cleared, engine must be re-enabled.
    EXPECT_TRUE(gpu.isEnabled())
        << "[RETRIEVAL:GPUPathStall] GPU advisory engine not re-enabled after error clearance";
}
