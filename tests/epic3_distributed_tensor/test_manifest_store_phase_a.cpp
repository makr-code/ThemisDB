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
 *   MS-13  placement, integrity, and freshness metadata survive store reopen
 *   MS-14  corrupted durable records prevent store startup
 *   MS-15  committed WAL records survive abrupt writer exit
 *   MS-16  concurrent updates retain the highest manifest version
 *   MS-17  deserialization rejects unknown enum values
 *   MS-18  valid no-update mode survives JSON serialization
 *   MS-19  invalid enum values cannot be stored
 *   MS-20  missing classification metadata prevents store startup
 */

#include <gtest/gtest.h>

#include "manifest_store.h"
#include "storage/rocksdb_wrapper.h"

#include <chrono>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction_db.h>
#include <nlohmann/json.hpp>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace themis::distributed_tensor;
using namespace themis::distributed_tensor::detail;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

std::filesystem::path makeTempDbPath() {
    static std::atomic<uint64_t> next_id{0};
    return std::filesystem::temp_directory_path() /
           ("themis_manifest_store_" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
            "_" + std::to_string(next_id.fetch_add(1)));
}

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
    m.shard_placements = {"node-a", "node-b"};
    return m;
}

} // namespace

// ---------------------------------------------------------------------------
// ManifestStoreTest fixture
// ---------------------------------------------------------------------------

class ManifestStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = makeTempDbPath();
        store_ = std::make_unique<ManifestStore>(db_path_.string());
    }

    void TearDown() override {
        store_.reset();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    std::filesystem::path db_path_;
    std::unique_ptr<ManifestStore> store_;
};

// MS-01: insert new entry
TEST_F(ManifestStoreTest, StoreNewEntryReturnsTrue) {
    const auto entry = makeEntry("users/embedding", 0, "art-1", 1);
    EXPECT_TRUE(store_->store(entry));
    EXPECT_EQ(store_->size(), 1u);
}

// MS-02: reject lower-version entry
TEST_F(ManifestStoreTest, StoreRejectsLowerVersion) {
    ASSERT_TRUE(store_->store(makeEntry("users/embedding", 0, "art-1", 5)));
    EXPECT_FALSE(store_->store(makeEntry("users/embedding", 0, "art-1", 4)));
    EXPECT_EQ(store_->size(), 1u);

    // Sanity: stored version should still be 5.
    auto result = store_->get("users/embedding", 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->version, 5u);
}

// MS-03: replace with higher version
TEST_F(ManifestStoreTest, StoreAcceptsHigherVersion) {
    ASSERT_TRUE(store_->store(makeEntry("users/embedding", 0, "art-1", 3)));
    ASSERT_TRUE(store_->store(makeEntry("users/embedding", 0, "art-1", 7)));
    EXPECT_EQ(store_->size(), 1u);

    auto result = store_->get("users/embedding", 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->version, 7u);
}

// MS-04: get() returns nullopt for unknown tensor
TEST_F(ManifestStoreTest, GetReturnsNulloptForUnknownTensor) {
    EXPECT_FALSE(store_->get("nonexistent/tensor", 0).has_value());
}

// MS-05: get() returns highest-version entry for the shard
TEST_F(ManifestStoreTest, GetReturnsFreshestEntryForShard) {
    ASSERT_TRUE(store_->store(makeEntry("items/vec", 0, "art-v1", 1)));
    ASSERT_TRUE(store_->store(makeEntry("items/vec", 0, "art-v2", 2)));
    ASSERT_TRUE(store_->store(makeEntry("items/vec", 1, "art-shard1", 10)));

    auto result = store_->get("items/vec", 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->version, 2u);
    EXPECT_EQ(result->shard_id, 0u);
}

// MS-06: list() returns sorted entries
TEST_F(ManifestStoreTest, ListReturnsSortedEntries) {
    ASSERT_TRUE(store_->store(makeEntry("orders/feat", 1, "art-s1v1", 1)));
    ASSERT_TRUE(store_->store(makeEntry("orders/feat", 0, "art-s0v2", 2)));
    ASSERT_TRUE(store_->store(makeEntry("orders/feat", 0, "art-s0v1", 1)));

    const auto entries = store_->list("orders/feat");
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
    ASSERT_TRUE(store_->store(makeEntry("t/e", 0, "to-evict", 1)));
    ASSERT_TRUE(store_->store(makeEntry("t/e", 0, "keep",     2)));
    ASSERT_EQ(store_->size(), 2u);

    const std::size_t removed = store_->evict("to-evict");
    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(store_->size(), 1u);

    // "keep" should still be retrievable.
    auto result = store_->get("t/e", 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->artifact_id, "keep");

    store_.reset();
    store_ = std::make_unique<ManifestStore>(db_path_.string());
    EXPECT_EQ(store_->size(), 1u);
    const auto restored = store_->get("t/e", 0);
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored->artifact_id, "keep");
}

// MS-08: evictStale() removes old entries
TEST_F(ManifestStoreTest, EvictStaleRemovesOldEntries) {
    // Fresh entry (0 seconds old).
    ASSERT_TRUE(store_->store(makeEntry("t/e", 0, "fresh",  1, 0s)));
    // Stale entry (60 seconds old).
    ASSERT_TRUE(store_->store(makeEntry("t/e", 0, "stale",  2, 60s)));
    ASSERT_EQ(store_->size(), 2u);

    // Evict entries older than 30 seconds.
    const std::size_t removed = store_->evictStale(30.0);
    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(store_->size(), 1u);
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
        db_path_ = makeTempDbPath();
        store_ = std::make_unique<ManifestStore>(
            db_path_.string(),
            &themis::observability::MetricsCollector::getInstance());
    }

    void TearDown() override {
        store_.reset();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    std::filesystem::path db_path_;
    std::unique_ptr<ManifestStore> store_;
};

// MS-10: freshness gauge updated by refreshFreshnessMetrics()
TEST_F(ManifestStoreMetricsTest, RefreshFreshnessMetricsUpdatesGauge) {
    ASSERT_TRUE(store_->store(makeEntry("docs/embed", 0, "art-1", 1, 10s)));
    store_->refreshFreshnessMetrics();

    const auto exported = themis::observability::MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(exported.find("tensor_freshness_age_seconds"), std::string::npos)
        << "tensor_freshness_age_seconds gauge not found in Prometheus export";
}

// MS-11: delta log counter incremented on store()
TEST_F(ManifestStoreMetricsTest, DeltaLogCounterIncrementedOnStore) {
    ASSERT_TRUE(store_->store(makeEntry("docs/embed", 0, "art-1", 1)));
    ASSERT_TRUE(store_->store(makeEntry("docs/embed", 0, "art-2", 2)));

    const auto exported = themis::observability::MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(exported.find("tensor_delta_log_entries_total"), std::string::npos)
        << "tensor_delta_log_entries_total counter not found in Prometheus export";
}

TEST_F(ManifestStoreTest, ManifestPlacementAndFreshnessSurviveReopen) {
    const auto entry = makeEntry("distributed/vector", 7, "placed-artifact", 3, 5s);
    ASSERT_TRUE(store_->store(entry));
    store_.reset();

    store_ = std::make_unique<ManifestStore>(db_path_.string());
    const auto restored = store_->get("distributed/vector", 7);
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored->artifact_id, entry.artifact_id);
    EXPECT_EQ(restored->version, entry.version);
    EXPECT_EQ(restored->integrity.crc32, entry.integrity.crc32);
    EXPECT_EQ(restored->integrity.payload_bytes, entry.integrity.payload_bytes);
    EXPECT_EQ(restored->shard_placements, entry.shard_placements);
    EXPECT_EQ(restored->created_at, entry.created_at);
    EXPECT_TRUE(restored->isFresh(60.0));
}

TEST_F(ManifestStoreTest, CorruptPersistedManifestFailsStoreOpenClosed) {
    ASSERT_TRUE(store_->store(makeEntry("corrupt/tensor", 0, "corrupt-artifact", 1)));
    store_.reset();

    themis::RocksDBWrapper::Config config;
    config.db_path = db_path_.string();
    themis::RocksDBWrapper storage(config);
    ASSERT_TRUE(storage.open());
    auto* db = storage.getRawDB();
    ASSERT_NE(db, nullptr);
    {
        rocksdb::ReadOptions read_options;
        std::unique_ptr<rocksdb::Iterator> iterator(db->NewIterator(read_options));
        ASSERT_NE(iterator, nullptr);
        iterator->Seek("distributed_tensor/manifest/");
        ASSERT_TRUE(iterator->Valid());
        auto corrupted = nlohmann::json::parse(iterator->value().ToString());
        corrupted.erase("created_at_unix_nanos");
        rocksdb::WriteOptions write_options;
        ASSERT_TRUE(db->Put(write_options, iterator->key(), corrupted.dump()).ok());
    }
    storage.close();

    EXPECT_THROW(ManifestStore(db_path_.string()), std::runtime_error);
}

TEST_F(ManifestStoreTest, MissingClassificationMetadataFailsStoreOpenClosed) {
    ASSERT_TRUE(store_->store(makeEntry("corrupt/classification", 0,
                                        "corrupt-artifact", 1)));
    store_.reset();

    themis::RocksDBWrapper::Config config;
    config.db_path = db_path_.string();
    themis::RocksDBWrapper storage(config);
    ASSERT_TRUE(storage.open());
    auto* db = storage.getRawDB();
    ASSERT_NE(db, nullptr);
    {
        rocksdb::ReadOptions read_options;
        std::unique_ptr<rocksdb::Iterator> iterator(db->NewIterator(read_options));
        ASSERT_NE(iterator, nullptr);
        iterator->Seek("distributed_tensor/manifest/");
        ASSERT_TRUE(iterator->Valid());
        auto corrupted = nlohmann::json::parse(iterator->value().ToString());
        corrupted.erase("artifact_class");
        rocksdb::WriteOptions write_options;
        ASSERT_TRUE(db->Put(write_options, iterator->key(), corrupted.dump()).ok());
    }
    storage.close();

    EXPECT_THROW(ManifestStore(db_path_.string()), std::runtime_error);
}

TEST(ArtifactManifestSerializationTest, RejectsUnknownEnumValues) {
    auto encoded = nlohmann::json::parse(
        makeEntry("enum/tensor", 0, "enum-artifact", 1).toJSON());
    encoded["lifecycle_state"] = "UNKNOWN_STATE";
    EXPECT_FALSE(ArtifactManifest::fromJSON(encoded.dump()));

    encoded = nlohmann::json::parse(
        makeEntry("enum/tensor", 0, "enum-artifact", 1).toJSON());
    encoded["kind"] = 256;
    EXPECT_FALSE(ArtifactManifest::fromJSON(encoded.dump()));

    encoded["kind"] = -1;
    EXPECT_FALSE(ArtifactManifest::fromJSON(encoded.dump()));

    encoded["kind"] = 3;
    EXPECT_FALSE(ArtifactManifest::fromJSON(encoded.dump()));
}

TEST(ArtifactManifestSerializationTest, PreservesNoUpdateMode) {
    auto manifest = makeEntry("enum/tensor", 0, "enum-artifact", 1);
    manifest.update_mode = UpdateMode::NONE;
    const auto restored = ArtifactManifest::fromJSON(manifest.toJSON());
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored->update_mode, UpdateMode::NONE);
}

TEST_F(ManifestStoreTest, RejectsInvalidEnumValues) {
    auto entry = makeEntry("enum/tensor", 0, "enum-artifact", 1);
    entry.lifecycle_state = static_cast<LifecycleState>(255);
    EXPECT_FALSE(store_->store(entry));
    EXPECT_EQ(store_->size(), 0u);
}

// MS-12: concurrent store() calls
TEST(ManifestStoreConcurrencyTest, ConcurrentStoreDoesNotCorrupt) {
    const auto db_path = makeTempDbPath();
    {
        ManifestStore store(db_path.string());
        constexpr int kWorkers = 8;
        constexpr int kPerWorker = 50;
        std::atomic<std::size_t> successful_writes{0};

        std::vector<std::thread> threads;
        threads.reserve(kWorkers);

        for (int w = 0; w < kWorkers; ++w) {
            threads.emplace_back([&store, &successful_writes, w] {
                for (int i = 0; i < kPerWorker; ++i) {
                    const std::string id = "art-" + std::to_string(w) + "-" +
                                           std::to_string(i);
                    if (store.store(makeEntry("concurrent/tensor",
                                              static_cast<uint32_t>(w % 4), id,
                                              static_cast<uint64_t>(i)))) {
                        ++successful_writes;
                    }
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }

        EXPECT_EQ(successful_writes.load(), 400u);
        EXPECT_EQ(store.size(), 400u);
    }

    {
        ManifestStore reopened(db_path.string());
        EXPECT_EQ(reopened.size(), 400u);
    }
    std::error_code error;
    std::filesystem::remove_all(db_path, error);
}

TEST(ManifestStoreConcurrencyTest, ConcurrentUpdatesRetainHighestVersion) {
    const auto db_path = makeTempDbPath();
    {
        ManifestStore store(db_path.string());
        constexpr int kWorkers = 8;
        constexpr int kVersionsPerWorker = 50;
        std::vector<std::thread> threads;
        for (int worker = 0; worker < kWorkers; ++worker) {
            threads.emplace_back([&store, worker] {
                for (int version = 1; version <= kVersionsPerWorker; ++version) {
                    (void)store.store(makeEntry(
                        "same/tensor", 0, "same-artifact",
                        static_cast<uint64_t>(worker * kVersionsPerWorker + version)));
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        const auto latest = store.get("same/tensor", 0);
        ASSERT_TRUE(latest);
        EXPECT_EQ(latest->version, 400u);
    }

    std::error_code error;
    std::filesystem::remove_all(db_path, error);
}

#ifndef _WIN32
TEST(ManifestStoreCrashRecoveryTest, CommittedWriteSurvivesLostCallerAcknowledgment) {
    const auto db_path = makeTempDbPath();
    int acknowledgment_pipe[2];
    ASSERT_EQ(pipe(acknowledgment_pipe), 0);
    const pid_t child = fork();
    ASSERT_NE(child, -1);
    if (child == 0) {
        close(acknowledgment_pipe[0]);
        ManifestStore store(db_path.string());
        const bool stored = store.store(makeEntry("crash/tensor", 2, "crash-artifact", 9));
        close(acknowledgment_pipe[1]);
        _exit(stored ? 0 : 1);
    }

    close(acknowledgment_pipe[1]);
    char acknowledgment = 0;
    EXPECT_EQ(read(acknowledgment_pipe[0], &acknowledgment, 1), 0);
    close(acknowledgment_pipe[0]);

    int status = 0;
    ASSERT_EQ(waitpid(child, &status, 0), child);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);

    {
        ManifestStore reopened(db_path.string());
        const auto restored = reopened.get("crash/tensor", 2);
        ASSERT_TRUE(restored);
        EXPECT_EQ(restored->version, 9u);
    }
    std::error_code error;
    std::filesystem::remove_all(db_path, error);
}
#endif
