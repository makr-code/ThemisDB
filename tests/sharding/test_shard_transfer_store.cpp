#include "sharding/shard_transfer_store.h"

#include <filesystem>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>
#include <rocksdb/db.h>

namespace themis::sharding {
namespace {

constexpr auto kA = "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb";
constexpr auto kB = "3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d";
constexpr auto kAB = "fb8e20fc2e4c3f248c60c39bd652f3c1347298bb977b8b4d5903b85055620603";
constexpr auto kABC = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
constexpr auto kEmpty = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

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
        const auto status = rocksdb::DB::Open(rocksdb::Options{}, path, &raw);
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
        const auto status = rocksdb::DB::Open(rocksdb::Options{}, path, &raw);
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
}

} // namespace
} // namespace themis::sharding
