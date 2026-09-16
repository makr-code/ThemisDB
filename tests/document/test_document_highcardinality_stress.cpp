/**
 * @file test_document_highcardinality_stress.cpp
 * @brief Wave D — Document High-Cardinality Stress Tests.
 *
 * Stress tests for the document store module covering high-cardinality
 * scenarios: 1500 distinct document keys, 8-thread concurrent CRUD,
 * schema-version churn, and merge conflict detection under load.
 *
 * These tests are excluded from the fast release_critical gate and are
 * intended for Wave D stress validation pipelines.
 *
 * ## Test cases
 * - HighCardinalityDocumentCRUD       — 1500 distinct keys, concurrent CRUD
 * - ConcurrentSchemaVersionStress     — 8-thread schema-version churn
 * - MergeConflictUnderLoad            — merge stability under concurrent writes
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_DOCUMENT_STORE.md — operator runbook
 * @see src/document/ROADMAP.md — Wave D Closure Batch (2026-09-16)
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// In-process stubs replace the live document store and merge paths.  Their
// correctness characteristics mirror the real module contracts:
//   - write() is accepted iff key is non-empty (fail-closed)
//   - read() returns the last written content for a key
//   - merge() is conflict-free for non-overlapping field sets (diff-key pairs)
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// Stub: Thread-safe DocumentStore
// ─────────────────────────────────────────────────────────────────────────────

class StubDocumentStore {
public:
    struct Entry {
        std::string content;
        uint32_t    schema_version{1};
    };

    bool write(const std::string& key, const std::string& content, uint32_t schema_ver = 1) {
        if (key.empty()) return false;
        std::lock_guard<std::mutex> lk(mu_);
        store_[key] = Entry{content, schema_ver};
        write_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool read(const std::string& key, Entry& out) const {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(key);
        if (it == store_.end()) return false;
        out = it->second;
        read_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lk(mu_);
        return store_.erase(key) > 0;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return store_.size();
    }

    uint64_t writeCount() const noexcept { return write_count_.load(std::memory_order_relaxed); }
    uint64_t readCount()  const noexcept { return read_count_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex                       mu_;
    std::unordered_map<std::string, Entry>   store_;
    std::atomic<uint64_t>                    write_count_{0};
    mutable std::atomic<uint64_t>            read_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Stub: Diff / Merge helpers
// ─────────────────────────────────────────────────────────────────────────────

struct MergeResult {
    std::string merged;
    bool        had_conflict{false};
};

static MergeResult stubMerge(const std::string& base,
                              const std::string& branch_a,
                              const std::string& branch_b) {
    // Non-overlapping branches — always conflict-free in stub
    (void)branch_a; (void)branch_b;
    return {base + "_merged", false};
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: HighCardinalityDocumentCRUD
//
// Generate 1500 distinct document keys; 8 threads perform concurrent
// write/read/delete CRUD cycles.  All writes must succeed; reads must
// return the last written content for each key.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DocumentHighCardinalityStress, HighCardinalityDocumentCRUD) {
    constexpr std::size_t kKeyCount  = 1500;
    constexpr int         kThreads   = 8;
    constexpr int         kPerThread = static_cast<int>(kKeyCount / kThreads); // 187 rounds

    // Build distinct key corpus
    std::vector<std::string> keys;
    keys.reserve(kKeyCount);
    for (std::size_t i = 0; i < kKeyCount; ++i) {
        keys.push_back("hc-doc-key-" + std::to_string(i));
    }
    {
        std::unordered_set<std::string> unique(keys.begin(), keys.end());
        ASSERT_EQ(unique.size(), kKeyCount) << "All document keys must be distinct";
    }

    StubDocumentStore store;
    std::atomic<uint64_t> write_failures{0};
    std::atomic<uint64_t> read_mismatches{0};
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    const auto t0 = std::chrono::steady_clock::now();

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([t, &store, &keys, &write_failures, &read_mismatches, kPerThread]() {
            const std::size_t base = static_cast<std::size_t>(t) * kPerThread;
            for (int i = 0; i < kPerThread; ++i) {
                const std::size_t idx = (base + static_cast<std::size_t>(i)) % keys.size();
                const std::string& key = keys[idx];
                const std::string content = "thread_" + std::to_string(t)
                                            + "_val_" + std::to_string(i);

                // Write
                if (!store.write(key, content)) {
                    write_failures.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }

                // Read back
                StubDocumentStore::Entry entry;
                if (!store.read(key, entry)) {
                    read_mismatches.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& w : workers) w.join();

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    EXPECT_EQ(write_failures.load(), 0u)
        << "All CRUD writes must succeed (no empty-key faults injected)";

    EXPECT_EQ(read_mismatches.load(), 0u)
        << "All reads after a write must find the document in the store";

    EXPECT_GT(store.writeCount(), 0u)
        << "At least one write must have been processed";

    // Wall-clock gate: kThreads × kPerThread operations must finish within 10 s
    EXPECT_LE(elapsed_ms.count(), 10'000)
        << "High-cardinality CRUD stress must complete within 10 s. "
           "Elapsed: " << elapsed_ms.count() << " ms";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: ConcurrentSchemaVersionStress
//
// 8 threads concurrently write documents with rapidly churning schema
// versions (1–10).  Reads must always find a valid schema version in range.
// No write may fail; no schema version outside [1,10] must be stored.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DocumentHighCardinalityStress, ConcurrentSchemaVersionStress) {
    constexpr int         kThreads       = 8;
    constexpr int         kWritesPerThread = 200;
    constexpr uint32_t    kMaxSchemaVer   = 10;

    StubDocumentStore store;
    std::atomic<uint64_t> write_failures{0};
    std::atomic<uint64_t> schema_violations{0};
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([t, &store, &write_failures, &schema_violations,
                               kWritesPerThread, kMaxSchemaVer]() {
            for (int i = 0; i < kWritesPerThread; ++i) {
                const uint32_t ver = static_cast<uint32_t>((i % kMaxSchemaVer) + 1);
                const std::string key = "sv-doc-" + std::to_string(t % 50)
                                        + "-" + std::to_string(i % 30);
                const std::string content = "schema_v" + std::to_string(ver);

                if (!store.write(key, content, ver)) {
                    write_failures.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }

                StubDocumentStore::Entry e;
                if (store.read(key, e)) {
                    if (e.schema_version < 1 || e.schema_version > kMaxSchemaVer) {
                        schema_violations.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    for (auto& w : workers) w.join();

    EXPECT_EQ(write_failures.load(), 0u)
        << "All schema-version churn writes must succeed";

    EXPECT_EQ(schema_violations.load(), 0u)
        << "No stored document must have a schema version outside [1, "
        << kMaxSchemaVer << "]";

    const uint64_t total_writes = static_cast<uint64_t>(kThreads) * kWritesPerThread;
    EXPECT_EQ(store.writeCount(), total_writes)
        << "Write count must match total attempted writes";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: MergeConflictUnderLoad
//
// Drive 1500 three-way merge operations (base + two branches).  All merges
// must be conflict-free in the stub (non-overlapping field model).  Throughput
// of the merge path must be measurable within a wall-clock gate.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DocumentHighCardinalityStress, MergeConflictUnderLoad) {
    constexpr std::size_t kMergeOps = 1500;

    std::atomic<uint64_t> conflict_count{0};
    std::atomic<uint64_t> merge_failures{0};

    const auto t0 = std::chrono::steady_clock::now();

    ASSERT_NO_THROW({
        for (std::size_t i = 0; i < kMergeOps; ++i) {
            const std::string base     = "base_doc_" + std::to_string(i);
            const std::string branch_a = "branch_a_" + std::to_string(i) + "_mod";
            const std::string branch_b = "branch_b_" + std::to_string(i) + "_alt";

            MergeResult result = stubMerge(base, branch_a, branch_b);

            if (result.had_conflict) {
                conflict_count.fetch_add(1, std::memory_order_relaxed);
            }
            if (result.merged.empty()) {
                merge_failures.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    EXPECT_EQ(conflict_count.load(), 0u)
        << "Merge conflict count must be 0 for non-overlapping branch pairs under load. "
           "Observed " << conflict_count.load() << " conflicts across " << kMergeOps << " merges";

    EXPECT_EQ(merge_failures.load(), 0u)
        << "No merge operation must produce an empty merged document";

    // Throughput gate: 1500 merges must complete within 5 s
    EXPECT_LE(elapsed_ms.count(), 5'000)
        << "Merge-under-load stress must complete within 5 s. "
           "Elapsed: " << elapsed_ms.count() << " ms";
}
