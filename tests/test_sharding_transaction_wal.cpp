/**
 * @file test_sharding_transaction_wal.cpp
 * @brief Tests for TransactionWAL (src/sharding/transaction_wal.cpp)
 *
 * Covers:
 *   - Construction with temp-dir config
 *   - initialize() succeeds and creates directories
 *   - logBegin() returns valid LSN
 *   - logPrepare() returns valid LSN
 *   - logCommit() returns valid LSN
 *   - logAbort() returns valid LSN
 *   - logAborted() returns valid LSN
 *   - logCompensate() returns valid LSN (SAGA)
 *   - readEntries() after writes contains entries
 *   - getCurrentLSN() increases after each write
 *   - shouldCreateSnapshot()
 *   - TransactionWALConfig defaults
 *   - TransactionProtocol enum values
 */

#include <gtest/gtest.h>
#include "sharding/transaction_wal.h"
#include <filesystem>
#include <string>
#include <vector>

using namespace sharding;

// ============================================================================
// Fixture
// ============================================================================

class TransactionWALTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a unique temp directory for each test
        auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        wal_dir_  = "/tmp/themis_txn_wal_"  + std::to_string(ts);
        snap_dir_ = "/tmp/themis_txn_snap_" + std::to_string(ts);

        TransactionWALConfig cfg;
        cfg.wal_directory      = wal_dir_;
        cfg.snapshot_directory = snap_dir_;
        cfg.segment_size       = 1 * 1024 * 1024; // 1 MB
        cfg.sync_on_write      = false;             // faster tests

        wal_ = std::make_unique<TransactionWAL>(cfg);
        ASSERT_TRUE(wal_->initialize());
    }

    void TearDown() override {
        wal_.reset();
        std::filesystem::remove_all(wal_dir_);
        std::filesystem::remove_all(snap_dir_);
    }

    std::unique_ptr<TransactionWAL> wal_;
    std::string wal_dir_;
    std::string snap_dir_;

    // Helper: valid LSN check (non-null segment or offset)
    static bool validLSN(const LSN& lsn) {
        return lsn.segment > 0 || lsn.offset > 0;
    }
};

// ============================================================================
// Construction and initialization
// ============================================================================

TEST_F(TransactionWALTest, Initialize_CreatesDirectories) {
    EXPECT_TRUE(std::filesystem::exists(wal_dir_));
    EXPECT_TRUE(std::filesystem::exists(snap_dir_));
}

TEST_F(TransactionWALTest, Initialize_DoubleInit_Succeeds) {
    // Re-initializing should be idempotent
    EXPECT_TRUE(wal_->initialize());
}

// ============================================================================
// getCurrentLSN before any writes
// ============================================================================

TEST_F(TransactionWALTest, GetCurrentLSN_InitialState) {
    auto lsn = wal_->getCurrentLSN();
    // LSN starts at 0,0 before any writes
    EXPECT_EQ(lsn.segment, 0u);
    EXPECT_EQ(lsn.offset, 0u);
}

// ============================================================================
// logBegin
// ============================================================================

TEST_F(TransactionWALTest, LogBegin_ReturnsValidLSN) {
    auto before = wal_->getCurrentLSN();
    auto lsn = wal_->logBegin("txn-001",
                               TransactionProtocol::TWO_PHASE_COMMIT,
                               {"shard-1", "shard-2"});
    auto after = wal_->getCurrentLSN();

    // The first WAL entry is written at 0/0 and advances current LSN after write.
    EXPECT_EQ(lsn.segment, before.segment);
    EXPECT_EQ(lsn.offset, before.offset);
    EXPECT_TRUE(after.segment > before.segment || after.offset > before.offset);
}

TEST_F(TransactionWALTest, LogBegin_IncrementsCurrentLSN) {
    auto before = wal_->getCurrentLSN();
    wal_->logBegin("txn-002", TransactionProtocol::SAGA, {"s1"});
    auto after = wal_->getCurrentLSN();

    // After a write the LSN should have advanced
    bool advanced = after.segment > before.segment ||
                    after.offset > before.offset;
    EXPECT_TRUE(advanced);
}

// ============================================================================
// logPrepare
// ============================================================================

TEST_F(TransactionWALTest, LogPrepare_ReturnsValidLSN) {
    wal_->logBegin("txn-p", TransactionProtocol::TWO_PHASE_COMMIT, {"s1", "s2"});
    auto lsn = wal_->logPrepare("txn-p", "s1", nlohmann::json{{"data", "prepare_data"}});
    EXPECT_TRUE(validLSN(lsn));
}

// ============================================================================
// logCommit
// ============================================================================

TEST_F(TransactionWALTest, LogCommit_ReturnsValidLSN) {
    wal_->logBegin("txn-c", TransactionProtocol::TWO_PHASE_COMMIT, {"s1"});
    auto lsn = wal_->logCommit("txn-c", nlohmann::json{{"decision", "commit"}});
    EXPECT_TRUE(validLSN(lsn));
}

// ============================================================================
// logAbort
// ============================================================================

TEST_F(TransactionWALTest, LogAbort_ReturnsValidLSN) {
    wal_->logBegin("txn-a", TransactionProtocol::TWO_PHASE_COMMIT, {"s1", "s2"});
    auto lsn = wal_->logAbort("txn-a", "timeout");
    EXPECT_TRUE(validLSN(lsn));
}

TEST_F(TransactionWALTest, LogAborted_ReturnsValidLSN) {
    wal_->logBegin("txn-ab", TransactionProtocol::TWO_PHASE_COMMIT, {"s1"});
    wal_->logAbort("txn-ab", "participant refused");
    auto lsn = wal_->logAborted("txn-ab", "s1");
    EXPECT_TRUE(validLSN(lsn));
}

// ============================================================================
// logCompensate (SAGA)
// ============================================================================

TEST_F(TransactionWALTest, LogCompensate_SAGA_ReturnsValidLSN) {
    wal_->logBegin("saga-001", TransactionProtocol::SAGA, {"svc-order", "svc-payment"});
    auto lsn = wal_->logCompensate("saga-001", "refund-step",
                                    nlohmann::json{{"amount", 50.0}});
    EXPECT_TRUE(validLSN(lsn));
}

// ============================================================================
// readEntries
// ============================================================================

TEST_F(TransactionWALTest, ReadEntries_Empty_ReturnsEmptyVec) {
    auto entries = wal_->readEntries();
    EXPECT_TRUE(entries.empty());
}

TEST_F(TransactionWALTest, ReadEntries_AfterWrites_ContainsEntries) {
    wal_->logBegin("txn-r", TransactionProtocol::TWO_PHASE_COMMIT, {"s1"});
    wal_->logCommit("txn-r", nlohmann::json{});

    auto entries = wal_->readEntries();
    EXPECT_GE(entries.size(), 1u);

    bool has_begin = false;
    for (const auto& e : entries) {
        if (e.type == TransactionWALEntryType::BEGIN &&
            e.transaction_id == "txn-r") {
            has_begin = true;
        }
    }
    EXPECT_TRUE(has_begin);
}

TEST_F(TransactionWALTest, ReadEntries_WithStartLSN_ReturnsSubset) {
    wal_->logBegin("t1", TransactionProtocol::TWO_PHASE_COMMIT, {"s1"});
    auto mid_lsn = wal_->getCurrentLSN();
    wal_->logCommit("t1", nlohmann::json{});

    // Reading from a later LSN should return fewer entries
    auto all    = wal_->readEntries(LSN(0, 0));
    auto subset = wal_->readEntries(mid_lsn);
    EXPECT_LE(subset.size(), all.size());
}

// ============================================================================
// shouldCreateSnapshot
// ============================================================================

TEST_F(TransactionWALTest, ShouldCreateSnapshot_BelowInterval_ReturnsFalse) {
    // Default interval is 1000 transactions
    EXPECT_FALSE(wal_->shouldCreateSnapshot(500));
}

TEST_F(TransactionWALTest, ShouldCreateSnapshot_AtOrAboveInterval_ReturnsTrue) {
    EXPECT_TRUE(wal_->shouldCreateSnapshot(1000));
    EXPECT_TRUE(wal_->shouldCreateSnapshot(2000));
}

// ============================================================================
// Config defaults
// ============================================================================

TEST(TransactionWALConfigTest, Defaults_AreReasonable) {
    TransactionWALConfig cfg;
    EXPECT_EQ(cfg.segment_size,       16u * 1024 * 1024); // 16 MB
    EXPECT_EQ(cfg.snapshot_interval,  1000u);
    EXPECT_EQ(cfg.max_snapshots,       10u);
    EXPECT_TRUE(cfg.sync_on_write);
}

// ============================================================================
// TransactionProtocol enum sanity
// ============================================================================

TEST(TransactionProtocolTest, AllValuesDistinct) {
    std::vector<TransactionProtocol> protocols = {
        TransactionProtocol::TWO_PHASE_COMMIT,
        TransactionProtocol::THREE_PHASE_COMMIT,
        TransactionProtocol::SAGA,
        TransactionProtocol::PERCOLATOR,
        TransactionProtocol::CALVIN,
    };
    for (size_t i = 0; i < protocols.size(); ++i) {
        for (size_t j = i + 1; j < protocols.size(); ++j) {
            EXPECT_NE(protocols[i], protocols[j]);
        }
    }
}
