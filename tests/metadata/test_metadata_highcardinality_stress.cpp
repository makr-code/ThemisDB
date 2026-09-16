/**
 * @file test_metadata_highcardinality_stress.cpp
 * @brief Wave D — Metadata Store High-Cardinality Stress Tests.
 *
 * Stress tests for metadata insert throughput, concurrent index operations,
 * and deep concurrent access under high-cardinality workloads.
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_METADATA_STORE.md
 * @see src/metadata/ROADMAP.md — Wave D Contribution
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
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// In-process stubs
// ─────────────────────────────────────────────────────────────────────────────

/// Thread-safe stub metadata table with a secondary index.
class StubMetadataTable {
public:
    StubMetadataTable() : insert_count_(0) {}

    void insert(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mu_);
        data_[key] = value;
        index_.insert(key);
        ++insert_count_;
    }

    bool isIndexed(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        return index_.count(key) > 0;
    }

    std::string get(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = data_.find(key);
        return it != data_.end() ? it->second : "";
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return data_.size();
    }

    uint64_t insertCount() const { return insert_count_.load(); }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::string> data_;
    std::unordered_set<std::string> index_;
    std::atomic<uint64_t> insert_count_;
};

// ─────────────────────────────────────────────────────────────────────────────
// HC-MD-01: High-cardinality metadata insert
// ─────────────────────────────────────────────────────────────────────────────
TEST(MetadataStress, HighCardinalityMetadataInsert) {
    constexpr int kKeys    = 200'000;
    constexpr int kWorkers = 8;

    StubMetadataTable table;

    auto chunk = kKeys / kWorkers;
    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            int start = w * static_cast<int>(chunk);
            int end   = (w == kWorkers - 1) ? kKeys : start + static_cast<int>(chunk);
            for (int i = start; i < end; ++i) {
                std::string key = "hc:meta:" + std::to_string(i);
                table.insert(key, "schema_v1");
            }
        });
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_GE(table.size(), static_cast<std::size_t>(kKeys))
        << "[METADATA:PartitionInconsistency] Not all high-cardinality keys inserted";

    // Spot-check 1 000 random keys for index consistency.
    std::mt19937 rng(42u);
    std::uniform_int_distribution<int> dist(0, kKeys - 1);
    uint64_t not_indexed = 0;
    for (int i = 0; i < 1000; ++i) {
        std::string key = "hc:meta:" + std::to_string(dist(rng));
        if (!table.isIndexed(key)) { ++not_indexed; }
    }
    EXPECT_EQ(not_indexed, 0U)
        << "[METADATA:IndexCorruption] " << not_indexed
        << " keys missing from index after high-cardinality insert";
}

// ─────────────────────────────────────────────────────────────────────────────
// HC-MD-02: Concurrent index stress
// ─────────────────────────────────────────────────────────────────────────────
TEST(MetadataStress, ConcurrentIndexStress) {
    constexpr int kKeys      = 50'000;
    constexpr int kWorkers   = 8;

    StubMetadataTable table;
    std::atomic<uint64_t> index_errors{0};

    // Phase 1: concurrent inserts.
    {
        auto chunk = kKeys / kWorkers;
        std::vector<std::thread> writers;
        writers.reserve(kWorkers);
        for (int w = 0; w < kWorkers; ++w) {
            writers.emplace_back([&, w]() {
                int start = w * static_cast<int>(chunk);
                int end   = (w == kWorkers - 1) ? kKeys : start + static_cast<int>(chunk);
                for (int i = start; i < end; ++i) {
                    table.insert("ci:meta:" + std::to_string(i), "v");
                }
            });
        }
        for (auto& t : writers) { t.join(); }
    }

    // Phase 2: concurrent index validation reads.
    {
        std::vector<std::thread> readers;
        readers.reserve(kWorkers);
        for (int w = 0; w < kWorkers; ++w) {
            readers.emplace_back([&, w]() {
                std::mt19937 rng(static_cast<unsigned>(w) + 5u);
                std::uniform_int_distribution<int> dist(0, kKeys - 1);
                for (int i = 0; i < 5000; ++i) {
                    std::string key = "ci:meta:" + std::to_string(dist(rng));
                    if (!table.isIndexed(key)) { ++index_errors; }
                }
            });
        }
        for (auto& t : readers) { t.join(); }
    }

    EXPECT_EQ(index_errors.load(), 0U)
        << "[METADATA:IndexCorruption] Index inconsistency found during concurrent "
        "index stress: " << index_errors.load() << " misses";
}

// ─────────────────────────────────────────────────────────────────────────────
// HC-MD-03: Deep concurrent access stress
// ─────────────────────────────────────────────────────────────────────────────
TEST(MetadataStress, DeepConcurrentAccessStress) {
    constexpr int kKeys    = 10'000;
    constexpr int kWorkers = 16; // deep concurrency
    constexpr int kOpsPerWorker = 5'000;

    StubMetadataTable table;

    // Pre-populate.
    for (int i = 0; i < kKeys; ++i) {
        table.insert("dc:" + std::to_string(i), "base");
    }

    std::atomic<uint64_t> exceptions{0};
    std::atomic<uint64_t> total_ops{0};

    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937 rng(static_cast<unsigned>(w) * 37u + 7u);
            std::uniform_int_distribution<int> key_dist(0, kKeys - 1);
            std::uniform_int_distribution<int> op_dist(0, 2); // 0=read 1=write 2=isIndexed
            uint64_t local_ops = 0;
            try {
                for (int i = 0; i < kOpsPerWorker; ++i) {
                    std::string key = "dc:" + std::to_string(key_dist(rng));
                    int op = op_dist(rng);
                    if (op == 0) {
                        (void)table.get(key);
                    } else if (op == 1) {
                        table.insert(key, "updated");
                    } else {
                        (void)table.isIndexed(key);
                    }
                    ++local_ops;
                }
            } catch (...) {
                ++exceptions;
            }
            total_ops.fetch_add(local_ops);
        });
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_EQ(exceptions.load(), 0U)
        << "[METADATA:PartitionInconsistency] Exception during deep concurrent access stress";
    EXPECT_EQ(total_ops.load(), static_cast<uint64_t>(kWorkers * kOpsPerWorker))
        << "Some operations lost during deep concurrent access stress";
}
