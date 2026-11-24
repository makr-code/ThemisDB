#include "sharding/shard_transfer_store.h"
#include "sharding/shard_range_snapshot.h"
#include "sharding/shard_artifact_fragment_store.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/rocksdb_open_compat.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/statistics.h>
#include <rocksdb/utilities/transaction_db.h>

namespace themis::sharding {
namespace {

constexpr auto kA = "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb";
constexpr auto kB = "3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d";
constexpr auto kAB = "fb8e20fc2e4c3f248c60c39bd652f3c1347298bb977b8b4d5903b85055620603";
constexpr auto kABC = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
constexpr auto kEmpty = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

TEST(ShardRangeSnapshot, UsesCanonicalRecordsAndRejectsRecordsOutsideTheAssignment) {
    const ShardTransferManifest assignment{"snapshot", "source", "target", "42", {}, 100, 200, 17};
    using namespace std::string_literals;
    std::string value = "a\0b"s;
    const auto snapshot = materializeShardRangeSnapshot(assignment, [&](const auto& visit) {
        visit(100, "a", value);
        visit(199, "z", "");
    });
    value = "changed";
    const auto expected = "\0\0\0\1\0\0\0\3aa\0b\0\0\0\1\0\0\0\0z"s;
    EXPECT_EQ(snapshot.manifest.bytes, expected.size());
    EXPECT_EQ(snapshot.manifest.chunks, 1u);
    EXPECT_EQ(snapshot.read_chunk(0), std::make_pair(expected, snapshot.manifest.sha256));
    EXPECT_THROW((void)snapshot.read_chunk(1), std::out_of_range);
    EXPECT_THROW((void)materializeShardRangeSnapshot(assignment, [](const auto& visit) {
        visit(200, "outside", "value");
    }), std::invalid_argument);
    EXPECT_THROW((void)materializeShardRangeSnapshot(assignment, [](const auto& visit) {
        visit(100, "a", "value");
        visit(101, "a", "duplicate");
    }), std::invalid_argument);

    const auto empty = materializeShardRangeSnapshot(assignment, [](const auto&) {});
    EXPECT_EQ(empty.manifest.chunks, 0u);
    EXPECT_EQ(empty.manifest.bytes, 0u);
    EXPECT_EQ(empty.manifest.sha256, kEmpty);
}

class ShardTransferStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::random_device random;
        const auto candidate = ".shard-transfer-store-test-" + std::to_string(random()) +
                               "-" + std::to_string(random());
        ASSERT_TRUE(std::filesystem::create_directory(candidate));
        path = candidate;
        reopen();
    }

    void TearDown() override {
        store.reset();
        if (!path.empty()) {
            std::error_code error;
            std::filesystem::remove_all(path, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    void reopen() {
        store.reset();
        store = std::make_unique<ShardTransferStore>(path, "target",
            [this](const auto&) { return current; });
    }

    std::string path;
    ShardTransferManifest manifest{"migration", "source", "target", "snapshot", kAB,
                                   10, 20, 7, 2, 2};
    bool current = true;
    std::unique_ptr<ShardTransferStore> store;
};

TEST_F(ShardTransferStoreTest, ResumesDurableCheckpointAndAcceptsOnlyIdenticalReplay) {
    EXPECT_THROW(store->prepare(manifest, "source"), std::logic_error);
    store->assign(manifest, "source");
    reopen();
    store->assign(manifest, "source");
    EXPECT_EQ(store->prepare(manifest, "source"), 0u);
    EXPECT_THROW(store->append(manifest.id, "source", 1, "b", kB), std::logic_error);
    EXPECT_THROW(store->append(manifest.id, "source", 0, "a", kB), std::invalid_argument);
    EXPECT_EQ(store->append(manifest.id, "source", 0, "a", kA), 1u);
    EXPECT_THROW(store->verify(manifest.id, "source"), std::logic_error);
    reopen();
    EXPECT_EQ(store->prepare(manifest, "source"), 1u);
    EXPECT_EQ(store->append(manifest.id, "source", 0, "a", kA), 1u);
    EXPECT_THROW(store->append(manifest.id, "source", 0, "b", kB), std::logic_error);
    EXPECT_EQ(store->append(manifest.id, "source", 1, "b", kB), 2u);
    EXPECT_NO_THROW(store->verify(manifest.id, "source"));
    reopen();
    EXPECT_EQ(store->prepare(manifest, "source"), 2u);
    EXPECT_NO_THROW(store->verify(manifest.id, "source"));
    EXPECT_THROW(store->append(manifest.id, "source", 2, "b", kB), std::logic_error);
}

TEST_F(ShardTransferStoreTest, ReportsDatabaseOpenFailure) {
    const auto invalid_path = (std::filesystem::path(path) / "CURRENT" / "store").string();
    EXPECT_THROW((void)ShardTransferStore(invalid_path, "target", [](const auto&) { return true; }),
                 std::runtime_error);
}

TEST_F(ShardTransferStoreTest, RequiresMatchingPrincipalManifestAndCurrentAssignment) {
    EXPECT_THROW(store->assign(manifest, "other"), std::logic_error);
    auto other_target = manifest;
    other_target.target = "other";
    EXPECT_THROW(store->assign(other_target, "source"), std::logic_error);
    current = false;
    EXPECT_THROW(store->assign(manifest, "source"), std::logic_error);
    current = true;
    store->assign(manifest, "source");
    auto other_manifest = manifest;
    ++other_manifest.epoch;
    EXPECT_THROW(store->assign(other_manifest, "source"), std::logic_error);
    EXPECT_THROW(store->prepare(other_manifest, "source"), std::logic_error);
    EXPECT_THROW(store->prepare(manifest, "other"), std::logic_error);
    EXPECT_EQ(store->prepare(manifest, "source"), 0u);
    EXPECT_THROW(store->append(manifest.id, "other", 0, "a", kA), std::logic_error);
    ASSERT_EQ(store->append(manifest.id, "source", 0, "a", kA), 1u);
    ASSERT_EQ(store->append(manifest.id, "source", 1, "b", kB), 2u);
    EXPECT_THROW(store->verify(manifest.id, "other"), std::logic_error);
    EXPECT_THROW(store->abort(manifest.id, "other"), std::logic_error);
    current = false;
    reopen();
    EXPECT_THROW(store->prepare(manifest, "source"), std::logic_error);
    EXPECT_THROW(store->append(manifest.id, "source", 0, "a", kA), std::logic_error);
    EXPECT_THROW(store->verify(manifest.id, "source"), std::logic_error);
    EXPECT_THROW(store->abort(manifest.id, "source"), std::logic_error);
    current = true;
    EXPECT_NO_THROW(store->verify(manifest.id, "source"));
    current = false;
    std::size_t consumed = 0;
    EXPECT_THROW(store->readVerified(manifest, [&](std::string_view) { ++consumed; }),
                 std::logic_error);
    EXPECT_EQ(consumed, 0u);
}

TEST_F(ShardTransferStoreTest, ValidatesLimitsSizesAndOrderedDigest) {
    auto malformed = manifest;
    malformed.chunks = 65'537;
    EXPECT_THROW(store->assign(malformed, "source"), std::invalid_argument);
    malformed = manifest;
    malformed.bytes = 1024ull * 1024 * 1024 + 1;
    EXPECT_THROW(store->assign(malformed, "source"), std::invalid_argument);
    malformed = manifest;
    malformed.bytes = 0;
    EXPECT_THROW(store->assign(malformed, "source"), std::invalid_argument);
    malformed = manifest;
    malformed.range_end = malformed.range_start;
    EXPECT_THROW(store->assign(malformed, "source"), std::invalid_argument);
    malformed = manifest;
    malformed.watermark = "\xc0\x80";
    EXPECT_THROW(store->assign(malformed, "source"), std::invalid_argument);
    EXPECT_THROW(store->prepare(malformed, "source"), std::invalid_argument);

    store->assign(manifest, "source");
    EXPECT_EQ(store->prepare(manifest, "source"), 0u);
    EXPECT_THROW(store->append(manifest.id, "source", 0, "", kEmpty), std::invalid_argument);
    EXPECT_THROW(store->append(manifest.id, "source", 0,
                               std::string(ShardTransferStore::kMaxChunkBytes + 1, 'a'), kA),
                 std::invalid_argument);
    EXPECT_THROW(store->append(manifest.id, "source", 0, "abc", kABC), std::logic_error);
    EXPECT_EQ(store->append(manifest.id, "source", 0, "b", kB), 1u);
    EXPECT_EQ(store->append(manifest.id, "source", 1, "a", kA), 2u);
    EXPECT_THROW(store->verify(manifest.id, "source"), std::logic_error);
}

TEST_F(ShardTransferStoreTest, EmptyTransferStillRequiresCorrectDigestAndPrepare) {
    manifest.chunks = 0;
    manifest.bytes = 0;
    manifest.sha256 = kEmpty;
    store->assign(manifest, "source");
    EXPECT_THROW(store->verify(manifest.id, "source"), std::logic_error);
    EXPECT_EQ(store->prepare(manifest, "source"), 0u);
    EXPECT_THROW(store->append(manifest.id, "source", 0, "a", kA), std::logic_error);
    EXPECT_NO_THROW(store->verify(manifest.id, "source"));
    auto wrong = manifest;
    wrong.id += "-wrong";
    wrong.sha256 = kA;
    store->assign(wrong, "source");
    EXPECT_EQ(store->prepare(wrong, "source"), 0u);
    EXPECT_THROW(store->verify(wrong.id, "source"), std::logic_error);
}

TEST_F(ShardTransferStoreTest, DurableAbortRemovesOnlyItsChunksAndPreventsRevival) {
    auto other = manifest;
    other.id += "-other";
    for (const auto& m : {manifest, other}) {
        store->assign(m, "source");
        ASSERT_EQ(store->prepare(m, "source"), 0u);
        ASSERT_EQ(store->append(m.id, "source", 0, "a", kA), 1u);
    }
    store->abort(manifest.id, "source");
    store.reset();
    {
        rocksdb::DB* raw = nullptr;
        const auto status = themis::storage::detail::openDbCompat(rocksdb::Options{}, path, &raw);
        ASSERT_TRUE(status.ok()) << status.ToString();
        const std::unique_ptr<rocksdb::DB> db(raw);
        std::string value;
        const std::string key = "c" + manifest.id + std::string(9, '\0');
        EXPECT_TRUE(db->Get(rocksdb::ReadOptions{}, key, &value).IsNotFound());
    }
    reopen();
    EXPECT_NO_THROW(store->abort(manifest.id, "source"));
    EXPECT_THROW(store->assign(manifest, "source"), std::logic_error);
    EXPECT_THROW(store->prepare(manifest, "source"), std::logic_error);
    EXPECT_THROW(store->append(manifest.id, "source", 1, "b", kB), std::logic_error);
    EXPECT_THROW(store->verify(manifest.id, "source"), std::logic_error);
    EXPECT_EQ(store->prepare(other, "source"), 1u);
    EXPECT_EQ(store->append(other.id, "source", 1, "b", kB), 2u);
    EXPECT_NO_THROW(store->verify(other.id, "source"));
}

TEST_F(ShardTransferStoreTest, RejectsPersistedMissingChunkEvenAfterVerification) {
    store->assign(manifest, "source");
    ASSERT_EQ(store->prepare(manifest, "source"), 0u);
    ASSERT_EQ(store->append(manifest.id, "source", 0, "a", kA), 1u);
    ASSERT_EQ(store->append(manifest.id, "source", 1, "b", kB), 2u);
    store->verify(manifest.id, "source");
    store.reset();
    {
        rocksdb::DB* raw = nullptr;
        const auto status = themis::storage::detail::openDbCompat(rocksdb::Options{}, path, &raw);
        ASSERT_TRUE(status.ok()) << status.ToString();
        const std::unique_ptr<rocksdb::DB> db(raw);
        const std::string key = "c" + manifest.id + std::string(9, '\0');
        rocksdb::WriteOptions options;
        options.sync = true;
        const auto deleted = db->Delete(options, key);
        ASSERT_TRUE(deleted.ok()) << deleted.ToString();
    }
    reopen();
    EXPECT_THROW(store->append(manifest.id, "source", 0, "a", kA), std::runtime_error);
    EXPECT_THROW(store->verify(manifest.id, "source"), std::runtime_error);
    std::size_t consumed = 0;
    EXPECT_THROW(store->readVerified(manifest, [&](std::string_view) { ++consumed; }),
                 std::runtime_error);
    EXPECT_EQ(consumed, 0u);
}

class ShardArtifactFragmentStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto suffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        path = std::filesystem::temp_directory_path() /
               ("themis-shard-artifact-fragments-" + suffix);
        ASSERT_TRUE(std::filesystem::create_directory(path));
        ASSERT_TRUE(openDatabase());
    }

    void TearDown() override {
        fragment_store.reset();
        if (database) {
            database->close();
            database.reset();
        }
        std::error_code error;
        std::filesystem::remove_all(path, error);
        EXPECT_FALSE(error) << error.message();
    }

    bool openDatabase(bool disable_wal_for_benchmark = false) {
        themis::storage::RocksDBWrapper::Config config;
        config.db_path = path.string();
        config.disable_wal_for_benchmark = disable_wal_for_benchmark;
        database = std::make_shared<themis::storage::RocksDBWrapper>(config);
        if (!database->open()) {
            return false;
        }
        fragment_store =
            std::make_unique<ShardArtifactFragmentStore>(database, 16);
        return true;
    }

    std::filesystem::path path;
    std::shared_ptr<themis::storage::RocksDBWrapper> database;
    std::unique_ptr<ShardArtifactFragmentStore> fragment_store;
};

TEST_F(ShardArtifactFragmentStoreTest, SyncsWalForNewAndRepeatedFragments) {
    fragment_store.reset();
    database->close();
    database.reset();
    ASSERT_TRUE(openDatabase(true));
    auto statistics = database->getRawDB()->GetOptions().statistics;
    ASSERT_TRUE(statistics);
    auto sync_count = statistics->getTickerCount(rocksdb::WAL_FILE_SYNCED);
    const std::vector<uint8_t> payload{'a', 'b', 'c'};

    ASSERT_EQ(fragment_store->put("tenant-a", "shard-1", "artifact", payload),
              ShardArtifactFragmentStore::PutResult::Stored);
    auto after_write = statistics->getTickerCount(rocksdb::WAL_FILE_SYNCED);
    EXPECT_GT(after_write, sync_count);
    ASSERT_EQ(fragment_store->put("tenant-a", "shard-1", "artifact", payload),
              ShardArtifactFragmentStore::PutResult::Unchanged);
    EXPECT_GT(statistics->getTickerCount(rocksdb::WAL_FILE_SYNCED), after_write);
    EXPECT_TRUE(database->getConfig().disable_wal_for_benchmark);
    EXPECT_FALSE(database->getConfig().force_sync_on_write);

    fragment_store.reset();
    database->close();
    database.reset();
    ASSERT_TRUE(openDatabase(true));
    const auto restored = fragment_store->read("tenant-a", "shard-1", "artifact", 3);
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored->bytes, payload);
}

TEST_F(ShardArtifactFragmentStoreTest, CheckedTransactionReadDistinguishesAbsenceAndFailure) {
    auto transaction = database->beginTransaction();
    ASSERT_TRUE(transaction);
    ASSERT_TRUE(transaction->isActive());
    const auto missing = transaction->getChecked("missing-key");
    ASSERT_TRUE(missing);
    EXPECT_FALSE(missing->has_value());
    ASSERT_TRUE(transaction->put("present-key", std::vector<uint8_t>{'x'}));
    const auto present = transaction->getChecked("present-key");
    ASSERT_TRUE(present);
    ASSERT_TRUE(present->has_value());
    EXPECT_EQ(**present, (std::vector<uint8_t>{'x'}));
    transaction->rollback();
    EXPECT_FALSE(transaction->getChecked("missing-key"));
}

TEST_F(ShardArtifactFragmentStoreTest, EnforcesImmutablePrincipalScopedManifestMapping) {
    const std::vector<uint8_t> payload{'a', 'b', 'c'};
    EXPECT_EQ(fragment_store->put("tenant-a", "shard-1", "artifact-7", payload),
              ShardArtifactFragmentStore::PutResult::Stored);
    EXPECT_EQ(fragment_store->put("tenant-a", "shard-1", "artifact-7", payload),
              ShardArtifactFragmentStore::PutResult::Unchanged);

    const auto fetched = fragment_store->read("tenant-a", "shard-1", "artifact-7", 3);
    ASSERT_TRUE(fetched);
    EXPECT_EQ(fetched->bytes, payload);
    EXPECT_EQ(fetched->sha256,
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    EXPECT_FALSE(fragment_store->read("tenant-b", "shard-1", "artifact-7", 3));
    EXPECT_FALSE(fragment_store->read("tenant-a", "shard-2", "artifact-7", 3));
    EXPECT_FALSE(fragment_store->read("tenant-a", "shard-1", "artifact-7", 2));
    EXPECT_EQ(fragment_store->put("tenant-a", "shard-1", "artifact-7",
                                  std::vector<uint8_t>{'a', 'b', 'd'}),
              ShardArtifactFragmentStore::PutResult::Conflict);
    EXPECT_EQ(fragment_store->put("tenant-a", "shard-1", "oversized",
                                  std::vector<uint8_t>(17, 0)),
              ShardArtifactFragmentStore::PutResult::InvalidInput);
    EXPECT_EQ(fragment_store->put("tenant-a", "shard-1", "empty",
                                  std::vector<uint8_t>{}),
              ShardArtifactFragmentStore::PutResult::Stored);
    const auto empty = fragment_store->read("tenant-a", "shard-1", "empty", 1);
    ASSERT_TRUE(empty);
    EXPECT_TRUE(empty->bytes.empty());
    EXPECT_EQ(empty->sha256,
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    fragment_store.reset();
    database->close();
    database.reset();
    ASSERT_TRUE(openDatabase());
    const auto after_restart =
        fragment_store->read("tenant-a", "shard-1", "artifact-7", 3);
    ASSERT_TRUE(after_restart);
    EXPECT_EQ(after_restart->bytes, payload);
}

} // namespace
} // namespace themis::sharding
