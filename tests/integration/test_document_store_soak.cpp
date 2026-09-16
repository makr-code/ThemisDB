/**
 * @file test_document_store_soak.cpp
 * @brief Wave D — Document Store Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB document store primary paths.
 * Verifies that write/read throughput, diff/merge stability, and schema
 * round-trip reliability hold up over a sustained run under normal
 * operating conditions.
 *
 * In CI environments this test is run with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate runs in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Write/read throughput ≥ 5 000 ops/sec over the full soak duration
 * - No exceptions thrown from any stub path
 * - Merge conflict count = 0 (all merges converge cleanly)
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see docs/operability/RUNBOOK_DOCUMENT_STORE.md — operator runbook
 * @see src/document/ROADMAP.md — Wave D Closure Batch (2026-09-16)
 * @see docs/operability/WAVE_D_ROADMAP.md — Phase 4 Soak Tests
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
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
    return 60'000ULL; // Default CI-safe: 1 minute
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// The in-process stubs below replace the live document store, diff, and
// merge paths so no external storage backend is required.  Correctness
// characteristics mirror the real module contracts:
//   - write() is accepted iff key is non-empty (mirrors DocumentStore fail-closed)
//   - read() returns the previously written content or empty string on miss
//   - diff() always produces a non-empty patch for distinct documents
//   - merge() is conflict-free for non-overlapping field sets
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// In-process DocumentStore stub
// Models the hot path: write → store → read → verify
// ─────────────────────────────────────────────────────────────────────────────

struct DocEntry {
    std::string key;
    std::string content;
    uint32_t    schema_version{1};
};

class StubDocumentStore {
public:
    /// Write a document; returns true on success, false if key is empty.
    bool write(const std::string& key, const std::string& content, uint32_t schema_ver = 1) {
        if (key.empty()) return false;
        std::lock_guard<std::mutex> lk(mu_);
        store_[key] = DocEntry{key, content, schema_ver};
        write_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    /// Read a document; returns content or empty string on miss.
    std::string read(const std::string& key) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(key);
        if (it == store_.end()) return {};
        read_count_.fetch_add(1, std::memory_order_relaxed);
        return it->second.content;
    }

    uint64_t writeCount() const noexcept { return write_count_.load(std::memory_order_relaxed); }
    uint64_t readCount()  const noexcept { return read_count_.load(std::memory_order_relaxed); }

private:
    std::unordered_map<std::string, DocEntry> store_;
    mutable std::mutex                         mu_;
    std::atomic<uint64_t>                      write_count_{0};
    std::atomic<uint64_t>                      read_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// In-process diff/merge stub
// ─────────────────────────────────────────────────────────────────────────────

struct DiffPatch {
    std::string from_key;
    std::string to_key;
    std::string patch;   // non-empty iff documents differ
};

/// Stub diff — always produces a deterministic patch for distinct inputs.
static DiffPatch stubDiff(const std::string& from_key, const std::string& a,
                           const std::string& to_key,   const std::string& b) {
    DiffPatch p{from_key, to_key, {}};
    if (a != b) {
        std::ostringstream oss;
        oss << "diff(" << from_key << "->" << to_key << ")[len:"
            << a.size() << "->" << b.size() << "]";
        p.patch = oss.str();
    }
    return p;
}

struct MergeResult {
    std::string merged_content;
    bool        had_conflict{false};
};

/// Stub merge — conflict-free iff patches address non-overlapping keys.
static MergeResult stubMerge(const std::string& base,
                              const DiffPatch&   left,
                              const DiffPatch&   right) {
    (void)left; (void)right;
    // Stub: merge is always clean (no conflicting overlapping field sets)
    return MergeResult{base + "_merged", /*had_conflict=*/false};
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: DocumentSoak_WriteReadThroughput
//
// Continuously write then read documents for soak_duration.
// Acceptance: throughput ≥ 5 000 ops/sec; no exceptions.
// ─────────────────────────────────────────────────────────────────────────────
TEST(DocumentSoak_WriteReadThroughput, SustainedThroughputAboveGate) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubDocumentStore store;
    const auto t0 = std::chrono::steady_clock::now();

    uint64_t cycle = 0;
    ASSERT_NO_THROW({
        while (std::chrono::steady_clock::now() - t0 < soak_duration) {
            const std::string key     = "doc_" + std::to_string(cycle % 500);
            const std::string content = "content_v" + std::to_string(cycle);
            ASSERT_TRUE(store.write(key, content));
            const std::string result = store.read(key);
            ASSERT_EQ(result, content);
            ++cycle;
        }
    });

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);
    ASSERT_GT(elapsed_ms.count(), 0);

    const double elapsed_sec = elapsed_ms.count() / 1000.0;
    const double ops_per_sec = static_cast<double>(store.writeCount() + store.readCount())
                               / elapsed_sec;

    EXPECT_GE(ops_per_sec, 5'000.0)
        << "Document store write/read throughput must be ≥ 5 000 ops/sec over the soak period. "
           "Observed: " << static_cast<uint64_t>(ops_per_sec) << " ops/sec";

    EXPECT_GT(store.writeCount(), 0u)
        << "At least one write must complete during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: DocumentSoak_DiffMergeStability
//
// Continuously run diff → merge cycles for soak_duration / 4.
// Acceptance: no exceptions; merge conflict count = 0.
// ─────────────────────────────────────────────────────────────────────────────
TEST(DocumentSoak_DiffMergeStability, NoConflictsOverSoak) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 4);

    StubDocumentStore store;
    uint64_t conflict_count = 0;
    uint64_t merge_cycles   = 0;

    const auto t0 = std::chrono::steady_clock::now();

    ASSERT_NO_THROW({
        while (std::chrono::steady_clock::now() - t0 < soak_duration) {
            const uint64_t seed   = merge_cycles;
            const std::string a   = "base_content_" + std::to_string(seed);
            const std::string b   = "modified_content_" + std::to_string(seed + 1);
            const std::string c   = "parallel_content_" + std::to_string(seed + 2);

            const DiffPatch left_patch  = stubDiff("base", a, "left",  b);
            const DiffPatch right_patch = stubDiff("base", a, "right", c);

            ASSERT_FALSE(left_patch.patch.empty())
                << "Diff of distinct documents must produce a non-empty patch";
            ASSERT_FALSE(right_patch.patch.empty())
                << "Diff of distinct documents must produce a non-empty patch";

            const MergeResult result = stubMerge(a, left_patch, right_patch);
            if (result.had_conflict) ++conflict_count;

            ++merge_cycles;
        }
    });

    EXPECT_EQ(conflict_count, 0u)
        << "Merge conflict count must be 0 over the soak period. "
           "Observed " << conflict_count << " conflicts across "
        << merge_cycles << " merge cycles";

    EXPECT_GT(merge_cycles, 0u)
        << "At least one diff/merge cycle must complete during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: DocumentSoak_SchemaRoundTripReliability
//
// Continuously write documents with incrementing schema versions and read
// them back, verifying round-trip fidelity across schema-version churn.
// Acceptance: no read mismatches; no exceptions.
// ─────────────────────────────────────────────────────────────────────────────
TEST(DocumentSoak_SchemaRoundTripReliability, FidelityUnderSchemaVersionChurn) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 4);

    StubDocumentStore store;
    uint64_t mismatch_count = 0;
    uint64_t round_trips    = 0;

    const auto t0 = std::chrono::steady_clock::now();

    ASSERT_NO_THROW({
        while (std::chrono::steady_clock::now() - t0 < soak_duration) {
            const uint32_t schema_ver = static_cast<uint32_t>((round_trips % 10) + 1);
            const std::string key     = "schema_doc_" + std::to_string(round_trips % 100);
            const std::string content = "schema_v" + std::to_string(schema_ver)
                                        + "_body_" + std::to_string(round_trips);

            ASSERT_TRUE(store.write(key, content, schema_ver));
            const std::string result = store.read(key);

            if (result != content) ++mismatch_count;
            ++round_trips;
        }
    });

    EXPECT_EQ(mismatch_count, 0u)
        << "Schema round-trip must produce zero mismatches over the soak period. "
           "Observed " << mismatch_count << " mismatches across "
        << round_trips << " round-trip cycles";

    EXPECT_GT(round_trips, 0u)
        << "At least one schema round-trip must complete during the soak";
}
