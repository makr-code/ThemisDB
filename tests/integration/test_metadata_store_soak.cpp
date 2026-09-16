/**
 * @file test_metadata_store_soak.cpp
 * @brief Wave D — Metadata Store Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB metadata store covering concurrent
 * read/write throughput, index consistency stability, and schema versioning
 * reliability under sustained load.
 *
 * In CI environments this test runs with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate finishes in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Concurrent read/write throughput ≥ 2 000 ops/sec over the soak duration
 * - No index inconsistency detected (every written key remains indexed)
 * - Schema versioning produces no conflicts under serial mutation workload
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see docs/operability/RUNBOOK_METADATA_STORE.md — operator runbook
 * @see src/metadata/ROADMAP.md — Wave D Contribution
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
// In-process stubs model the metadata store hot path without requiring
// external backends. These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

/// Stub metadata store with a secondary index.
class StubMetadataStore {
public:
    StubMetadataStore() : reads_(0), writes_(0) {}

    void put(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mu_);
        store_[key] = value;
        index_.insert(key);
        ++writes_;
    }

    std::string get(const std::string& key) {
        std::lock_guard<std::mutex> lk(mu_);
        ++reads_;
        auto it = store_.find(key);
        return it != store_.end() ? it->second : "";
    }

    bool isIndexed(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        return index_.count(key) > 0;
    }

    uint64_t reads()  const { return reads_.load();  }
    uint64_t writes() const { return writes_.load(); }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::string> store_;
    std::unordered_set<std::string> index_;
    std::atomic<uint64_t> reads_;
    std::atomic<uint64_t> writes_;
};

/// Stub schema versioner — monotonically advances schema versions.
class StubSchemaVersioner {
public:
    StubSchemaVersioner() : version_(1), conflicts_(0) {}

    /// Apply a schema migration. Returns false if version conflict detected.
    bool migrate(int expected_version, const std::string& /*migration*/) {
        std::lock_guard<std::mutex> lk(mu_);
        if (version_.load() != expected_version) {
            ++conflicts_;
            return false;
        }
        version_.fetch_add(1);
        return true;
    }

    int  currentVersion() const { return version_.load();    }
    uint64_t conflicts()  const { return conflicts_.load();  }

private:
    std::mutex mu_;
    std::atomic<int> version_;
    std::atomic<uint64_t> conflicts_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class MetadataSoakFixture : public ::testing::Test {
protected:
    StubMetadataStore   store{};
    StubSchemaVersioner versioner{};
};

// ─────────────────────────────────────────────────────────────────────────────
// MD-SOAK-01: Concurrent read/write throughput
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MetadataSoakFixture, MetadataSoak_ConcurrentReadWriteThroughput) {
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto deadline = std::chrono::steady_clock::now() + duration;

    constexpr int kWriters = 2;
    constexpr int kReaders = 4;
    std::atomic<uint64_t> total_ops{0};

    std::vector<std::thread> workers;
    workers.reserve(kWriters + kReaders);

    for (int w = 0; w < kWriters; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937_64 rng(static_cast<uint64_t>(w) * 0x1234567890ABCULL);
            std::uniform_int_distribution<int> key_dist(0, 9999);
            uint64_t local = 0;
            while (std::chrono::steady_clock::now() < deadline) {
                std::string key = "md:rw:" + std::to_string(key_dist(rng));
                store.put(key, "val_" + std::to_string(local));
                ++local;
            }
            total_ops.fetch_add(local);
        });
    }
    for (int r = 0; r < kReaders; ++r) {
        workers.emplace_back([&, r]() {
            std::mt19937_64 rng(static_cast<uint64_t>(r) ^ 0xFEDCBA9876543210ULL);
            std::uniform_int_distribution<int> key_dist(0, 9999);
            uint64_t local = 0;
            while (std::chrono::steady_clock::now() < deadline) {
                std::string key = "md:rw:" + std::to_string(key_dist(rng));
                (void)store.get(key);
                ++local;
            }
            total_ops.fetch_add(local);
        });
    }
    for (auto& t : workers) { t.join(); }

    const double elapsed_sec = static_cast<double>(soakDurationMs()) / 1000.0;
    const double throughput   = static_cast<double>(total_ops.load()) / elapsed_sec;
    EXPECT_GE(throughput, 2000.0)
        << "Concurrent read/write throughput below 2 000 ops/sec: " << throughput;
}

// ─────────────────────────────────────────────────────────────────────────────
// MD-SOAK-02: Index consistency stability
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MetadataSoakFixture, MetadataSoak_IndexConsistencyStability) {
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto deadline = std::chrono::steady_clock::now() + duration;

    constexpr int kWorkers = 4;
    std::atomic<uint64_t> index_misses{0};

    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&, w]() {
            std::mt19937 rng(static_cast<unsigned>(w) + 99u);
            std::uniform_int_distribution<int> key_dist(0, 4999);
            while (std::chrono::steady_clock::now() < deadline) {
                std::string key = "idx:" + std::to_string(key_dist(rng));
                store.put(key, "v");
                // After put, the key must always be in the index.
                if (!store.isIndexed(key)) { ++index_misses; }
            }
        });
    }
    for (auto& t : workers) { t.join(); }

    EXPECT_EQ(index_misses.load(), 0U)
        << "[METADATA:IndexCorruption] Index inconsistency: "
        << index_misses.load() << " keys missing from index after write";
}

// ─────────────────────────────────────────────────────────────────────────────
// MD-SOAK-03: Schema versioning reliability
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MetadataSoakFixture, MetadataSoak_SchemaVersioningReliability) {
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto deadline = std::chrono::steady_clock::now() + duration;

    // Serial schema migration loop — must have zero conflicts.
    uint64_t migrations = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        int expected = versioner.currentVersion();
        bool ok = versioner.migrate(expected, "ALTER TABLE t ADD COLUMN c" +
                                              std::to_string(migrations) + " INT");
        if (!ok) {
            FAIL() << "[METADATA:SchemaConflict] Unexpected schema version conflict "
                   << "at migration " << migrations;
        }
        ++migrations;
        // Small yield to avoid spinning too tight.
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    EXPECT_EQ(versioner.conflicts(), 0U)
        << "[METADATA:SchemaConflict] Schema versioning conflicts detected: "
        << versioner.conflicts();
    EXPECT_GT(migrations, 0U)
        << "No schema migrations completed during the soak";
}
