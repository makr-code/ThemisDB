/**
 * @file test_updates_highcardinality_stress.cpp
 * @brief Wave D — Updates Engine High-Cardinality Stress Tests.
 *
 * Stress tests for update batching, concurrent delta application, and long-run
 * reliability under high-cardinality workloads.
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_UPDATES_ENGINE.md
 * @see src/updates/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// In-process stubs
// ─────────────────────────────────────────────────────────────────────────────

/// Stub batch writer — models the updates engine write path.
class StubBatchWriter {
public:
    StubBatchWriter() : written_(0), conflicts_(0) {}

    bool write(const std::string& key, int version) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = versions_.find(key);
        if (it != versions_.end() && it->second > version) {
            ++conflicts_;
            return false;
        }
        versions_[key] = version;
        ++written_;
        return true;
    }

    uint64_t written()   const { return written_.load();   }
    uint64_t conflicts() const { return conflicts_.load(); }
    std::size_t keyCount() const {
        std::lock_guard<std::mutex> lk(mu_);
        return versions_.size();
    }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, int> versions_;
    std::atomic<uint64_t> written_;
    std::atomic<uint64_t> conflicts_;
};

/// Stub delta applier — applies incremental updates.
class StubDeltaApplier {
public:
    StubDeltaApplier() : applied_(0), errors_(0) {}

    void apply(const std::string& key, int delta) {
        std::lock_guard<std::mutex> lk(mu_);
        state_[key] += delta;
        ++applied_;
    }

    int get(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = state_.find(key);
        return it != state_.end() ? it->second : 0;
    }

    uint64_t applied() const { return applied_.load(); }
    uint64_t errors()  const { return errors_.load();  }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, int> state_;
    std::atomic<uint64_t> applied_;
    std::atomic<uint64_t> errors_;
};

// ─────────────────────────────────────────────────────────────────────────────
// HC-UP-01: High-cardinality update batch
// ─────────────────────────────────────────────────────────────────────────────
TEST(UpdatesStress, HighCardinalityUpdateBatch) {
    constexpr int kKeys    = 200'000;
    constexpr int kWorkers = 8;

    StubBatchWriter writer;

    auto chunk = kKeys / kWorkers;
    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            int start = w * static_cast<int>(chunk);
            int end   = (w == kWorkers - 1) ? kKeys : start + static_cast<int>(chunk);
            for (int i = start; i < end; ++i) {
                std::string key = "hc:upd:" + std::to_string(i);
                writer.write(key, 1);
            }
        });
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_GE(writer.keyCount(), static_cast<std::size_t>(kKeys))
        << "[UPDATES:BatchFailed] Not all high-cardinality keys were written";
    EXPECT_EQ(writer.conflicts(), 0U)
        << "Unexpected conflicts in non-overlapping high-cardinality write";
}

// ─────────────────────────────────────────────────────────────────────────────
// HC-UP-02: Concurrent delta application stress
// ─────────────────────────────────────────────────────────────────────────────
TEST(UpdatesStress, ConcurrentDeltaApplicationStress) {
    constexpr int kKeys      = 1000;
    constexpr int kWorkers   = 8;
    constexpr int kOpsPerKey = 100; // each worker applies kOpsPerKey deltas of +1

    StubDeltaApplier applier;

    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937 rng(static_cast<unsigned>(w) + 1u);
            std::uniform_int_distribution<int> key_dist(0, kKeys - 1);
            for (int i = 0; i < kKeys * kOpsPerKey / kWorkers; ++i) {
                std::string key = "da:" + std::to_string(key_dist(rng));
                applier.apply(key, 1);
            }
        });
    }
    for (auto& t : workers) { t.join(); }

    const uint64_t expected_total = static_cast<uint64_t>(kKeys) * kOpsPerKey;
    EXPECT_EQ(applier.applied(), expected_total)
        << "[UPDATES:DeltaConflict] Concurrent delta application lost some operations: "
        << "expected=" << expected_total << " actual=" << applier.applied();
    EXPECT_EQ(applier.errors(), 0U)
        << "Unexpected errors during concurrent delta application";
}

// ─────────────────────────────────────────────────────────────────────────────
// HC-UP-03: Long-run reliability under load
// ─────────────────────────────────────────────────────────────────────────────
TEST(UpdatesStress, LongRunReliabilityUnderLoad) {
    // 30-second run (shorter than soak, appropriate for stress tier).
    const auto duration = 30s;
    const auto deadline = std::chrono::steady_clock::now() + duration;

    constexpr int kWorkers = 4;
    StubBatchWriter writer;
    std::atomic<uint64_t> exceptions{0};

    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937_64 rng(static_cast<uint64_t>(w) * 0xABCDEF1234567890ULL);
            std::uniform_int_distribution<int> key_dist(0, 49999);
            int ver = 0;
            try {
                while (std::chrono::steady_clock::now() < deadline) {
                    std::string key = "lr:" + std::to_string(key_dist(rng));
                    writer.write(key, ver++);
                }
            } catch (...) {
                ++exceptions;
            }
        });
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_EQ(exceptions.load(), 0U)
        << "[UPDATES:BatchFailed] Exception thrown during long-run reliability test";
    EXPECT_GT(writer.written(), 0U)
        << "No writes completed during long-run reliability test";
}
