// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file test_two_wal_entry_helper.cpp
 * @brief Unit tests for WALEntryHelper — shared WAL-entry construction and
 *        append utilities used by all three ThemisDB 2PC coordinators.
 *
 * Tests cover:
 *  - buildEntry(): type/txn_id/data propagation, timestamp non-zero and monotone
 *  - appendOrLog(): returns true on success (real WAL in /tmp)
 *  - appendOrLog(): returns false when wal pointer is null (no I/O)
 *  - appendOrLog(): is noexcept (static_assert)
 *  - append(): entry written and readable back from real WAL
 *  - append(): sync_on_write=true does not throw
 */

#include <gtest/gtest.h>

#include "transaction/wal_entry_helper.h"
#include "sharding/wal_manager.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Counter for unique per-test directories (avoids clashes in parallel runs).
std::atomic<int> g_test_counter{0};

/// Return a unique temp directory for WAL files.  WALManager creates it.
std::string uniqueWALDir() {
    return "/tmp/test_wal_entry_helper_" +
           std::to_string(
               std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now().time_since_epoch()
               ).count()
           ) + "_" + std::to_string(g_test_counter.fetch_add(1));
}

/// Construct a WALManager backed by a temporary directory.
std::unique_ptr<themis::sharding::WALManager> makeWAL(
    const std::string& dir = uniqueWALDir()
) {
    themis::sharding::WALManagerConfig cfg;
    cfg.wal_directory = dir;
    cfg.sync_on_write = false;
    return std::make_unique<themis::sharding::WALManager>(cfg);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Fixture — cleans up temp directories after each test
// ─────────────────────────────────────────────────────────────────────────────

class WALEntryHelperTest : public ::testing::Test {
protected:
    std::string                                       wal_dir_;
    std::unique_ptr<themis::sharding::WALManager>     wal_;

    void SetUp() override {
        wal_dir_ = uniqueWALDir();
        themis::sharding::WALManagerConfig cfg;
        cfg.wal_directory = wal_dir_;
        cfg.sync_on_write = false;
        wal_ = std::make_unique<themis::sharding::WALManager>(cfg);
    }

    void TearDown() override {
        wal_.reset(); // close WAL files before removing the directory
        fs::remove_all(wal_dir_);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// buildEntry tests (pure function — no I/O)
// ─────────────────────────────────────────────────────────────────────────────

TEST(WALEntryHelperBuildEntry, PropagatesType) {
    const auto entry = themis::transaction::WALEntryHelper::buildEntry(
        themis::sharding::WALEntryType::BEGIN_TX, "txn-1", {}
    );
    EXPECT_EQ(entry.type, themis::sharding::WALEntryType::BEGIN_TX);
}

TEST(WALEntryHelperBuildEntry, PropagatesCommitType) {
    const auto entry = themis::transaction::WALEntryHelper::buildEntry(
        themis::sharding::WALEntryType::COMMIT_TX, "txn-1", {}
    );
    EXPECT_EQ(entry.type, themis::sharding::WALEntryType::COMMIT_TX);
}

TEST(WALEntryHelperBuildEntry, PropagatesAbortType) {
    const auto entry = themis::transaction::WALEntryHelper::buildEntry(
        themis::sharding::WALEntryType::ABORT_TX, "txn-1", {}
    );
    EXPECT_EQ(entry.type, themis::sharding::WALEntryType::ABORT_TX);
}

TEST(WALEntryHelperBuildEntry, PropagatesTransactionId) {
    const auto entry = themis::transaction::WALEntryHelper::buildEntry(
        themis::sharding::WALEntryType::COMMIT_TX, "txn-unique-abc", {}
    );
    EXPECT_EQ(entry.transaction_id, "txn-unique-abc");
}

TEST(WALEntryHelperBuildEntry, PropagatesJsonData) {
    const nlohmann::json data = {{"phase", "decision"}, {"decision", "commit"}};
    const auto entry = themis::transaction::WALEntryHelper::buildEntry(
        themis::sharding::WALEntryType::COMMIT_TX, "txn-1", data
    );
    EXPECT_EQ(entry.data, data);
}

TEST(WALEntryHelperBuildEntry, EmptyDataProducesEmptyJson) {
    const auto entry = themis::transaction::WALEntryHelper::buildEntry(
        themis::sharding::WALEntryType::ABORT_TX, "txn-2", {}
    );
    EXPECT_TRUE(entry.data.empty());
}

TEST(WALEntryHelperBuildEntry, TimestampIsNonZero) {
    const auto entry = themis::transaction::WALEntryHelper::buildEntry(
        themis::sharding::WALEntryType::BEGIN_TX, "txn-3", {}
    );
    EXPECT_GT(entry.timestamp, uint64_t{0});
}

TEST(WALEntryHelperBuildEntry, SuccessiveTimestampsAreNonDecreasing) {
    const auto e1 = themis::transaction::WALEntryHelper::buildEntry(
        themis::sharding::WALEntryType::BEGIN_TX, "txn-ts-1", {}
    );
    const auto e2 = themis::transaction::WALEntryHelper::buildEntry(
        themis::sharding::WALEntryType::COMMIT_TX, "txn-ts-1", {}
    );
    EXPECT_LE(e1.timestamp, e2.timestamp);
}

TEST(WALEntryHelperBuildEntry, AllStandardEntryTypesAccepted) {
    using T = themis::sharding::WALEntryType;
    for (const auto t : {T::BEGIN_TX, T::PREPARE_TX, T::COMMIT_TX,
                          T::ABORT_TX, T::CHECKPOINT}) {
        const auto entry = themis::transaction::WALEntryHelper::buildEntry(t, "txn", {});
        EXPECT_EQ(entry.type, t);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// appendOrLog — null WAL (no I/O)
// ─────────────────────────────────────────────────────────────────────────────

TEST(WALEntryHelperAppendOrLog, ReturnsFalseWhenWALIsNull) {
    const bool ok = themis::transaction::WALEntryHelper::appendOrLog(
        nullptr,
        themis::sharding::WALEntryType::BEGIN_TX,
        "txn-null",
        {},
        false,
        "TestCoord"
    );
    EXPECT_FALSE(ok);
}

TEST(WALEntryHelperAppendOrLog, IsNoexcept) {
    // appendOrLog must satisfy noexcept — verified at compile time.
    static_assert(
        noexcept(themis::transaction::WALEntryHelper::appendOrLog(
            static_cast<themis::sharding::WALManager*>(nullptr),
            themis::sharding::WALEntryType::BEGIN_TX,
            std::string{},
            nlohmann::json{},
            false,
            std::string_view{}
        )),
        "appendOrLog must be declared noexcept"
    );
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// appendOrLog — real WAL (I/O)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALEntryHelperTest, AppendOrLogReturnsTrueOnSuccess) {
    const bool ok = themis::transaction::WALEntryHelper::appendOrLog(
        wal_.get(),
        themis::sharding::WALEntryType::BEGIN_TX,
        "txn-orlog-1",
        {},
        false,
        "TestCoord"
    );
    EXPECT_TRUE(ok);
}

TEST_F(WALEntryHelperTest, AppendOrLogWritesEntry) {
    themis::transaction::WALEntryHelper::appendOrLog(
        wal_.get(),
        themis::sharding::WALEntryType::COMMIT_TX,
        "txn-written-1",
        {{"phase", "complete"}},
        false,
        "TestCoord"
    );
    const auto entries = wal_->readRange(themis::sharding::LSN{0, 0});
    ASSERT_FALSE(entries.empty());
    EXPECT_EQ(entries.back().transaction_id, "txn-written-1");
    EXPECT_EQ(entries.back().type, themis::sharding::WALEntryType::COMMIT_TX);
}

TEST_F(WALEntryHelperTest, AppendOrLogMultipleCallsSucceed) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(
            themis::transaction::WALEntryHelper::appendOrLog(
                wal_.get(),
                themis::sharding::WALEntryType::BEGIN_TX,
                "txn-multi-" + std::to_string(i),
                {},
                false,
                "TestCoord"
            )
        );
    }
    const auto entries = wal_->readRange(themis::sharding::LSN{0, 0});
    EXPECT_EQ(entries.size(), size_t{5});
}

TEST_F(WALEntryHelperTest, AppendOrLogPreservesJsonPayload) {
    const nlohmann::json data = {
        {"coordinator_id", "coord-1"},
        {"shards", {"s1", "s2"}},
        {"phase", "decision"}
    };
    themis::transaction::WALEntryHelper::appendOrLog(
        wal_.get(),
        themis::sharding::WALEntryType::COMMIT_TX,
        "txn-payload",
        data,
        false,
        "TestCoord"
    );
    const auto entries = wal_->readRange(themis::sharding::LSN{0, 0});
    ASSERT_FALSE(entries.empty());
    EXPECT_EQ(entries.back().data, data);
}

// ─────────────────────────────────────────────────────────────────────────────
// append — real WAL (throwing variant)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALEntryHelperTest, AppendWritesEntryToWAL) {
    EXPECT_NO_THROW(
        themis::transaction::WALEntryHelper::append(
            *wal_,
            themis::sharding::WALEntryType::BEGIN_TX,
            "txn-append-1",
            {{"transaction_id", "txn-append-1"}},
            false
        )
    );
    const auto entries = wal_->readRange(themis::sharding::LSN{0, 0});
    ASSERT_FALSE(entries.empty());
    EXPECT_EQ(entries.back().transaction_id, "txn-append-1");
}

TEST_F(WALEntryHelperTest, AppendWithSyncOnWriteSucceeds) {
    EXPECT_NO_THROW(
        themis::transaction::WALEntryHelper::append(
            *wal_,
            themis::sharding::WALEntryType::COMMIT_TX,
            "txn-sync",
            {},
            /*sync_on_write=*/true
        )
    );
}

TEST_F(WALEntryHelperTest, AppendPreservesEntryOrder) {
    themis::transaction::WALEntryHelper::append(
        *wal_, themis::sharding::WALEntryType::BEGIN_TX, "txn-ord", {}, false
    );
    themis::transaction::WALEntryHelper::append(
        *wal_, themis::sharding::WALEntryType::COMMIT_TX, "txn-ord", {}, false
    );

    const auto entries = wal_->readRange(themis::sharding::LSN{0, 0});
    ASSERT_GE(entries.size(), size_t{2});
    EXPECT_EQ(entries[entries.size() - 2].type, themis::sharding::WALEntryType::BEGIN_TX);
    EXPECT_EQ(entries[entries.size() - 1].type, themis::sharding::WALEntryType::COMMIT_TX);
}
