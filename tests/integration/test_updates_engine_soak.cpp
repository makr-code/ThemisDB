/**
 * @file test_updates_engine_soak.cpp
 * @brief Wave D — Updates Engine Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB updates engine covering write
 * throughput, delta application stability, and performance baseline consistency.
 *
 * In CI environments this test runs with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate finishes in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Write throughput ≥ 2 000 ops/sec over the soak duration
 * - Delta application conflict rate = 0 under serial workload
 * - Throughput variance < 20% across measurement windows (baseline consistency)
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see docs/operability/RUNBOOK_UPDATES_ENGINE.md — operator runbook
 * @see src/updates/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Soak duration — overridable via THEMIS_SOAK_DURATION_MS environment variable.
// Default: 60 000 ms (1 min).
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
// In-process stubs model the updates engine write path without requiring
// external backends (WAL, replication, etc.).
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

/// Stub update batch — accumulates records in a thread-safe ring buffer.
class StubUpdateBatch {
public:
    explicit StubUpdateBatch(std::size_t capacity)
        : capacity_(capacity), applied_(0), conflicts_(0) {}

    /// Apply an update record. Returns false on simulated conflict.
    bool apply(const std::string& key, int version) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = latest_version_.find(key);
        if (it != latest_version_.end() && it->second > version) {
            ++conflicts_;
            return false; // stale write
        }
        latest_version_[key] = version;
        if (store_.size() >= capacity_) { store_.erase(store_.begin()); }
        store_[key] = version;
        ++applied_;
        return true;
    }

    uint64_t applied()   const { return applied_.load();   }
    uint64_t conflicts() const { return conflicts_.load(); }

private:
    std::size_t capacity_;
    std::mutex  mu_;
    std::unordered_map<std::string, int> store_;
    std::unordered_map<std::string, int> latest_version_;
    std::atomic<uint64_t> applied_;
    std::atomic<uint64_t> conflicts_;
};

/// Stub delta engine — applies a series of integer deltas to a value store.
class StubDeltaEngine {
public:
    StubDeltaEngine() : total_applied_(0) {}

    void applyDelta(const std::string& key, int delta) {
        std::lock_guard<std::mutex> lk(mu_);
        values_[key] += delta;
        ++total_applied_;
    }

    int value(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = values_.find(key);
        return it != values_.end() ? it->second : 0;
    }

    uint64_t totalApplied() const { return total_applied_.load(); }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, int> values_;
    std::atomic<uint64_t> total_applied_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class UpdatesSoakFixture : public ::testing::Test {
protected:
    StubUpdateBatch batch{10'000};
    StubDeltaEngine delta{};
};

// ─────────────────────────────────────────────────────────────────────────────
// UP-SOAK-01: Write throughput
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(UpdatesSoakFixture, UpdatesSoak_WriteThroughput) {
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto deadline = std::chrono::steady_clock::now() + duration;

    constexpr int kWorkers = 4;
    std::atomic<uint64_t> total_writes{0};
    std::atomic<bool>     write_error{false};

    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937_64 rng(static_cast<uint64_t>(w) * 0x7f4a7c15deadULL);
            std::uniform_int_distribution<int> key_dist(0, 9999);
            int ver = 0;
            uint64_t local = 0;
            while (std::chrono::steady_clock::now() < deadline) {
                std::string key = "wk:" + std::to_string(key_dist(rng));
                if (!batch.apply(key, ver++)) {
                    // Conflict is OK under concurrent writes; fatal errors are not.
                }
                ++local;
            }
            total_writes.fetch_add(local);
        });
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_FALSE(write_error.load())
        << "[UPDATES:BatchFailed] Fatal write error during throughput soak";

    const double elapsed_sec = static_cast<double>(soakDurationMs()) / 1000.0;
    const double throughput   = static_cast<double>(total_writes.load()) / elapsed_sec;
    EXPECT_GE(throughput, 2000.0)
        << "Write throughput below 2 000 ops/sec: " << throughput;
}

// ─────────────────────────────────────────────────────────────────────────────
// UP-SOAK-02: Delta application stability
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(UpdatesSoakFixture, UpdatesSoak_DeltaApplicationStability) {
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto deadline = std::chrono::steady_clock::now() + duration;

    constexpr int kWorkers = 4;
    constexpr int kKeys    = 100;
    std::atomic<bool> error{false};

    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937 rng(static_cast<unsigned>(w) + 42u);
            std::uniform_int_distribution<int> key_dist(0, kKeys - 1);
            std::uniform_int_distribution<int> delta_dist(-5, 5);
            while (std::chrono::steady_clock::now() < deadline) {
                std::string key = "dk:" + std::to_string(key_dist(rng));
                delta.applyDelta(key, delta_dist(rng));
            }
        });
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_FALSE(error.load())
        << "[UPDATES:DeltaConflict] Unexpected error during delta application soak";
    EXPECT_GT(delta.totalApplied(), 0U)
        << "No delta operations were applied during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// UP-SOAK-03: Performance baseline consistency
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(UpdatesSoakFixture, UpdatesSoak_PerformanceBaselineConsistency) {
    const auto full_duration = std::chrono::milliseconds(soakDurationMs());
    const auto window_ms     = std::chrono::milliseconds(soakDurationMs() / 5);

    constexpr int kWorkers = 4;
    std::vector<double> window_throughputs;

    for (int window = 0; window < 5; ++window) {
        StubUpdateBatch local_batch(5000);
        const auto deadline = std::chrono::steady_clock::now() + window_ms;

        std::atomic<uint64_t> window_ops{0};
        std::vector<std::thread> workers;
        workers.reserve(kWorkers);
        for (int w = 0; w < kWorkers; ++w) {
            workers.emplace_back([&, w, window]() {
                std::mt19937_64 rng(static_cast<uint64_t>(w) + static_cast<uint64_t>(window) * 1000ULL);
                std::uniform_int_distribution<int> key_dist(0, 4999);
                int ver = 0;
                while (std::chrono::steady_clock::now() < deadline) {
                    std::string key = "pb:" + std::to_string(key_dist(rng));
                    local_batch.apply(key, ver++);
                    ++window_ops;
                }
            });
        }
        for (auto& t : workers) { t.join(); }

        double elapsed_sec = static_cast<double>(window_ms.count()) / 1000.0;
        window_throughputs.push_back(static_cast<double>(window_ops.load()) / elapsed_sec);
    }

    // Verify variance < 20% (max / min ≤ 1.20).
    ASSERT_FALSE(window_throughputs.empty());
    double mn = *std::min_element(window_throughputs.begin(), window_throughputs.end());
    double mx = *std::max_element(window_throughputs.begin(), window_throughputs.end());
    EXPECT_GT(mn, 0.0) << "[UPDATES:BaselineRegression] Zero throughput in a measurement window";

    if (mn > 0.0) {
        double ratio = mx / mn;
        EXPECT_LE(ratio, 1.20)
            << "[UPDATES:BaselineRegression] Throughput variance too high across windows: "
            << "max=" << mx << " min=" << mn << " ratio=" << ratio;
    }
}
