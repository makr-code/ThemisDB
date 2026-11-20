#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "storage/mvcc_store.h"
#include "storage/rocksdb_wrapper.h"

using namespace themis;

namespace {

std::vector<uint8_t> asBytes(std::string_view value) {
    return {value.begin(), value.end()};
}

class MVCCRangeExportTest : public ::testing::Test {
protected:
    void SetUp() override {
        static std::atomic<uint64_t> next_id{0};
        const auto id = next_id.fetch_add(1, std::memory_order_relaxed);
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("themis_mvcc_range_export_" +
                     std::to_string(std::chrono::steady_clock::now()
                                        .time_since_epoch().count()) +
                     "_" + std::to_string(id))).string();

        RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.enable_wal = true;
        db_ = std::make_shared<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        store_ = std::make_unique<MVCCStore>(db_);
    }

    void TearDown() override {
        store_.reset();
        db_.reset();
        std::error_code ec;
        std::filesystem::remove_all(db_path_, ec);
    }

    MVCCStore::SnapshotExportOptions allKeys() const {
        MVCCStore::SnapshotExportOptions options;
        options.include_key = [](std::string_view) { return true; };
        return options;
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper> db_;
    std::unique_ptr<MVCCStore> store_;
};

TEST_F(MVCCRangeExportTest, ExportsLatestLogicalRecordsAndExplicitDeletes) {
    store_->put("b", asBytes("old"));
    store_->put("b", asBytes("new"));
    store_->put("a", asBytes("value"));

    auto txn = db_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    store_->delInTxn(*txn, "b");
    ASSERT_TRUE(txn->commit());

    auto result = store_->exportSnapshot(allKeys());
    ASSERT_TRUE(result);
    ASSERT_EQ(result->records.size(), 2u);
    EXPECT_EQ(result->records[0].key, "a");
    EXPECT_EQ(result->records[0].kind, MVCCStore::SnapshotRecordKind::Put);
    EXPECT_EQ(result->records[0].value, asBytes("value"));
    EXPECT_EQ(result->records[1].key, "b");
    EXPECT_EQ(result->records[1].kind, MVCCStore::SnapshotRecordKind::Delete);
    EXPECT_TRUE(result->records[1].value.empty());
    EXPECT_GT(result->rocksdb_sequence, 0u);
    EXPECT_GT(result->hlc_watermark.value, 0u);
}

TEST_F(MVCCRangeExportTest, SnapshotExcludesCommitAfterCaptureEvenWhenHlcWasAssigned) {
    auto txn = db_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    const auto assigned = store_->putInTxn(*txn, "late", asBytes("not-visible-yet"));

    auto reader_result = store_->beginSnapshot();
    ASSERT_TRUE(reader_result);
    auto reader = std::move(reader_result).value();
    auto metadata = reader.exportRecords(allKeys());
    ASSERT_TRUE(metadata);
    EXPECT_GE(metadata->hlc_watermark.value, assigned.value);

    ASSERT_TRUE(txn->commit());

    auto captured = reader.exportRecords(allKeys());
    ASSERT_TRUE(captured);
    EXPECT_TRUE(captured->records.empty());

    auto later = store_->exportSnapshot(allKeys());
    ASSERT_TRUE(later);
    ASSERT_EQ(later->records.size(), 1u);
    EXPECT_EQ(later->records.front().key, "late");
}

TEST_F(MVCCRangeExportTest, SnapshotKeepsItsViewAcrossWritesAndGc) {
    const auto old_timestamp = store_->put("key", asBytes("before"));
    auto reader_result = store_->beginSnapshot();
    ASSERT_TRUE(reader_result);
    auto reader = std::move(reader_result).value();

    const auto new_timestamp = store_->put("key", asBytes("after"));
    EXPECT_GT(new_timestamp, old_timestamp);
    store_->gcVersionsBefore(
        "key",
        HLCTimestamp(new_timestamp.value + 1),
        MVCCStore::GCOptions{1}
    );

    auto captured = reader.exportRecords(allKeys());
    ASSERT_TRUE(captured);
    ASSERT_EQ(captured->records.size(), 1u);
    EXPECT_EQ(captured->records.front().value, asBytes("before"));

    auto current = store_->exportSnapshot(allKeys());
    ASSERT_TRUE(current);
    ASSERT_EQ(current->records.size(), 1u);
    EXPECT_EQ(current->records.front().value, asBytes("after"));
}

TEST_F(MVCCRangeExportTest, MoveAssignmentReleasesAndReplacesSnapshot) {
    store_->put("key", asBytes("before"));
    auto first_result = store_->beginSnapshot();
    ASSERT_TRUE(first_result);
    auto first = std::move(first_result).value();

    store_->put("key", asBytes("after"));
    auto second_result = store_->beginSnapshot();
    ASSERT_TRUE(second_result);
    auto second = std::move(second_result).value();

    first = std::move(second);
    auto result = first.exportRecords(allKeys());
    ASSERT_TRUE(result);
    ASSERT_EQ(result->records.size(), 1u);
    EXPECT_EQ(result->records.front().value, asBytes("after"));
}

TEST_F(MVCCRangeExportTest, EmptySelectionIsAValidSnapshotAndFailuresAreNotEmptySuccess) {
    MVCCStore::SnapshotExportOptions empty_selection;
    empty_selection.include_key = [](std::string_view) { return false; };
    auto empty = store_->exportSnapshot(empty_selection);
    ASSERT_TRUE(empty);
    EXPECT_TRUE(empty->records.empty());

    EXPECT_FALSE(store_->exportSnapshot({}));

    auto cancelled = allKeys();
    cancelled.is_cancelled = [] { return true; };
    EXPECT_FALSE(store_->exportSnapshot(cancelled));

    auto callback_failure = allKeys();
    callback_failure.include_key = [](std::string_view) -> bool {
        throw std::runtime_error("predicate failure");
    };
    store_->put("key", asBytes("value"));
    EXPECT_FALSE(store_->exportSnapshot(callback_failure));

    auto too_small = allKeys();
    too_small.max_encoded_bytes = 24;
    EXPECT_FALSE(store_->exportSnapshot(too_small));
}

} // namespace
