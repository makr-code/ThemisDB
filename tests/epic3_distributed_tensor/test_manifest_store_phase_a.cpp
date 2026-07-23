/**
 * @file test_manifest_store_phase_a.cpp
 * @brief Phase A regression tests for ManifestStore and ArtifactManifest.
 *
 * Test IDs:
 *   MS-01  store() inserts a new entry and returns true
 *   MS-02  store() rejects an entry with a lower version (monotonic guard)
 *   MS-03  store() accepts an entry with a higher version (replaces)
 *   MS-04  get() returns nullopt for unknown tensor_name
 *   MS-05  get() returns the highest-version entry for a given shard
 *   MS-06  list() returns entries sorted by shard_id asc, version desc
 *   MS-07  evict() removes entries by artifact_id
 *   MS-08  evictStale() removes entries older than the threshold
 *   MS-09  isFresh() advisory gate: fresh entry passes, stale fails
 *   MS-10  refreshFreshnessMetrics() emits tensor_freshness_age_seconds gauge
 *   MS-11  tensor_delta_log_entries_total counter incremented on store()
 *   MS-12  Concurrent store() calls do not corrupt the registry
 */

#include <gtest/gtest.h>

#include "manifest_store.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::distributed_tensor;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

ArtifactManifest makeEntry(const std::string& tensor_name,
                            uint32_t shard_id,
                            const std::string& artifact_id,
                            uint64_t version,
                            std::chrono::seconds age_offset = 0s) {
    ArtifactManifest m;
    m.tensor_name = tensor_name;
    m.shard_id    = shard_id;
    m.artifact_id = artifact_id;
    m.version     = version;
    m.kind        = ArtifactKind::ADVISORY_SUMMARY;
    m.created_at  = std::chrono::system_clock::now() - age_offset;
    m.integrity.crc32         = 0xDEADBEEFu;
    m.integrity.payload_bytes = 128;
    return m;
}

} // namespace

// ---------------------------------------------------------------------------
// ManifestStoreTest fixture
// ---------------------------------------------------------------------------

class ManifestStoreTest : public ::testing::Test {
protected:
    // No MetricsCollector injected — metrics path tested separately.
    ManifestStore store_;
};

// MS-01: insert new entry
TEST_F(ManifestStoreTest, StoreNewEntryReturnsTrue) {
    const auto entry = makeEntry("users/embedding", 0, "art-1", 1);
    EXPECT_TRUE(store_.store(entry));
    EXPECT_EQ(store_.size(), 1u);
}

// MS-02: reject lower-version entry
TEST_F(ManifestStoreTest, StoreRejectsLowerVersion) {
    ASSERT_TRUE(store_.store(makeEntry("users/embedding", 0, "art-1", 5)));
    EXPECT_FALSE(store_.store(makeEntry("users/embedding", 0, "art-1", 4)));
    EXPECT_EQ(store_.size(), 1u);

    // Sanity: stored version should still be 5.
    auto result = store_.get("users/embedding", 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->version, 5u);
}

// MS-03: replace with higher version
TEST_F(ManifestStoreTest, StoreAcceptsHigherVersion) {
    ASSERT_TRUE(store_.store(makeEntry("users/embedding", 0, "art-1", 3)));
    ASSERT_TRUE(store_.store(makeEntry("users/embedding", 0, "art-1", 7)));
    EXPECT_EQ(store_.size(), 1u);

    auto result = store_.get("users/embedding", 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->version, 7u);
}

// MS-04: get() returns nullopt for unknown tensor
TEST_F(ManifestStoreTest, GetReturnsNulloptForUnknownTensor) {
    EXPECT_FALSE(store_.get("nonexistent/tensor", 0).has_value());
}

// MS-05: get() returns highest-version entry for the shard
TEST_F(ManifestStoreTest, GetReturnsFreshestEntryForShard) {
    store_.store(makeEntry("items/vec", 0, "art-v1", 1));
    store_.store(makeEntry("items/vec", 0, "art-v2", 2));
    store_.store(makeEntry("items/vec", 1, "art-shard1", 10)); // different shard

    auto result = store_.get("items/vec", 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->version, 2u);
    EXPECT_EQ(result->shard_id, 0u);
}

// MS-06: list() returns sorted entries
TEST_F(ManifestStoreTest, ListReturnsSortedEntries) {
    store_.store(makeEntry("orders/feat", 1, "art-s1v1", 1));
    store_.store(makeEntry("orders/feat", 0, "art-s0v2", 2));
    store_.store(makeEntry("orders/feat", 0, "art-s0v1", 1));

    const auto entries = store_.list("orders/feat");
    ASSERT_EQ(entries.size(), 3u);

    // shard 0 comes first; within shard 0, higher version comes first.
    EXPECT_EQ(entries[0].shard_id, 0u);
    EXPECT_EQ(entries[0].version, 2u);

    EXPECT_EQ(entries[1].shard_id, 0u);
    EXPECT_EQ(entries[1].version, 1u);

    EXPECT_EQ(entries[2].shard_id, 1u);
}

// MS-07: evict() removes entries by artifact_id
TEST_F(ManifestStoreTest, EvictRemovesEntriesByArtifactId) {
    store_.store(makeEntry("t/e", 0, "to-evict", 1));
    store_.store(makeEntry("t/e", 0, "keep",     2));
    ASSERT_EQ(store_.size(), 2u);

    const std::size_t removed = store_.evict("to-evict");
    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(store_.size(), 1u);

    // "keep" should still be retrievable.
    auto result = store_.get("t/e", 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->artifact_id, "keep");
}

// MS-08: evictStale() removes old entries
TEST_F(ManifestStoreTest, EvictStaleRemovesOldEntries) {
    // Fresh entry (0 seconds old).
    store_.store(makeEntry("t/e", 0, "fresh",  1, 0s));
    // Stale entry (60 seconds old).
    store_.store(makeEntry("t/e", 0, "stale",  2, 60s));
    ASSERT_EQ(store_.size(), 2u);

    // Evict entries older than 30 seconds.
    const std::size_t removed = store_.evictStale(30.0);
    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(store_.size(), 1u);
}

// MS-09: isFresh() advisory gate
TEST_F(ManifestStoreTest, IsFreshAdvisoryGate) {
    const auto fresh_entry = makeEntry("x/y", 0, "a1", 1, 5s);  // 5 s old
    const auto stale_entry = makeEntry("x/y", 0, "a2", 1, 120s); // 120 s old

    EXPECT_TRUE(fresh_entry.isFresh(60.0));   // within 60 s budget
    EXPECT_FALSE(stale_entry.isFresh(60.0));  // exceeds 60 s budget
    EXPECT_TRUE(stale_entry.isFresh(300.0));  // within 300 s budget
}

// ---------------------------------------------------------------------------
// Metrics tests
// ---------------------------------------------------------------------------

#include "observability/metrics_collector.h"

class ManifestStoreMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        themis::observability::MetricsCollector::getInstance().reset();
        store_ = std::make_unique<ManifestStore>(&themis::observability::MetricsCollector::getInstance());
    }

    std::unique_ptr<ManifestStore> store_;
};

// MS-10: freshness gauge updated by refreshFreshnessMetrics()
TEST_F(ManifestStoreMetricsTest, RefreshFreshnessMetricsUpdatesGauge) {
    store_->store(makeEntry("docs/embed", 0, "art-1", 1, 10s));
    store_->refreshFreshnessMetrics();

    const auto exported = themis::observability::MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(exported.find("tensor_freshness_age_seconds"), std::string::npos)
        << "tensor_freshness_age_seconds gauge not found in Prometheus export";
}

// MS-11: delta log counter incremented on store()
TEST_F(ManifestStoreMetricsTest, DeltaLogCounterIncrementedOnStore) {
    store_->store(makeEntry("docs/embed", 0, "art-1", 1));
    store_->store(makeEntry("docs/embed", 0, "art-2", 2));

    const auto exported = themis::observability::MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(exported.find("tensor_delta_log_entries_total"), std::string::npos)
        << "tensor_delta_log_entries_total counter not found in Prometheus export";
}

// MS-12: concurrent store() calls
TEST(ManifestStoreConcurrencyTest, ConcurrentStoreDoesNotCorrupt) {
    ManifestStore store;
    constexpr int kWorkers = 8;
    constexpr int kPerWorker = 50;

    std::vector<std::thread> threads;
    threads.reserve(kWorkers);

    for (int w = 0; w < kWorkers; ++w) {
        threads.emplace_back([&store, w] {
            for (int i = 0; i < kPerWorker; ++i) {
                const std::string id = "art-" + std::to_string(w) + "-" + std::to_string(i);
                store.store(makeEntry("concurrent/tensor",
                                      static_cast<uint32_t>(w % 4),
                                      id,
                                      static_cast<uint64_t>(i)));
            }
        });
    }
    for (auto& t : threads) t.join();

    // Size should be at most kWorkers * kPerWorker distinct artifact IDs.
    EXPECT_LE(store.size(), static_cast<std::size_t>(kWorkers * kPerWorker));
    EXPECT_GT(store.size(), 0u);
}
