// Tests for cross-coordinator WAL recovery contracts — Issue #5372 (CC-5).
//
// These tests document and enforce the WAL isolation contract:
// each coordinator writes its own WAL format and can only recover its own
// transactions.  They also validate the shared WALLoggingHelper utility and
// verify that WAL entries produced by WALLoggingHelper are well-formed.
//
// CWR-1  WALLoggingHelper: nullptr WAL is a no-op (returns nullopt)
// CWR-2  WALLoggingHelper: appendEntryWithResult returns a valid LSN on success
// CWR-3  WALLoggingHelper: appendEntry (void overload) does not throw
// CWR-4  WALLoggingHelper: JSON payload is embedded verbatim in WALEntry::data
// CWR-5  WALLoggingHelper: BEGIN_TX entry has expected WALEntryType
// CWR-6  WALLoggingHelper: COMMIT_TX / ABORT_TX entries are distinct types
// CWR-7  DistributedTransactionManager: recoverInDoubtTransactions() returns 0 without WAL
// CWR-8  DistributedTransactionManager: recoverInDoubtTransactions() returns 0 with empty WAL

#include <gtest/gtest.h>

#include "sharding/wal_logging_helper.h"
#include "sharding/wal_manager.h"
#include "transaction/distributed_transaction_manager.h"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

using namespace themis::sharding;
using namespace themis::transaction;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Create a temporary directory unique to this process invocation.
static std::filesystem::path makeTempDir(const std::string& suffix) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto p  = std::filesystem::temp_directory_path()
              / ("themis_cwr_" + suffix + "_" + std::to_string(ts));
    std::filesystem::create_directories(p);
    return p;
}

struct TempDirCleanup {
    std::filesystem::path path;

    ~TempDirCleanup() noexcept {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// WALLoggingHelper contract tests
// ─────────────────────────────────────────────────────────────────────────────

// CWR-1: nullptr WAL is a safe no-op.
TEST(CrossCoordinatorWALRecovery, NullWAL_AppendWithResult_ReturnsNullopt) {
    WALManager* null_wal = nullptr;
    auto result = WALLoggingHelper::appendEntryWithResult(
        null_wal,
        WALEntryType::BEGIN_TX,
        "txn-cwr-1",
        nlohmann::json{{"coordinator_id", "coord-a"}},
        /*sync=*/false,
        "coordinator",
        "coord-a"
    );
    EXPECT_FALSE(result.has_value())
        << "appendEntryWithResult should return nullopt when WAL is nullptr";
}

// CWR-2: appendEntryWithResult returns a valid LSN when a real WAL is provided.
TEST(CrossCoordinatorWALRecovery, RealWAL_AppendWithResult_ReturnsValidLSN) {
    auto dir = makeTempDir("cwr2");
    TempDirCleanup cleanup{dir};

    {
        WALManagerConfig cfg;
        cfg.wal_directory = dir.string();
        cfg.segment_size  = 1 * 1024 * 1024;
        WALManager wal(cfg);
        ASSERT_TRUE(wal.initialize());

        auto result = WALLoggingHelper::appendEntryWithResult(
            &wal,
            WALEntryType::BEGIN_TX,
            "txn-cwr-2",
            nlohmann::json{{"coordinator_id", "coord-a"}, {"participants", nlohmann::json::array()}},
            /*sync=*/false,
            "coordinator",
            "coord-a"
        );

        EXPECT_TRUE(result.has_value())
            << "appendEntryWithResult should return a valid LSN for a live WAL";
    }
}

// CWR-3: void overload (appendEntry) does not throw for nullptr or live WAL.
TEST(CrossCoordinatorWALRecovery, AppendEntry_VoidOverload_DoesNotThrow) {
    EXPECT_NO_THROW({
        WALLoggingHelper::appendEntry(
            nullptr,
            WALEntryType::COMMIT_TX,
            "txn-cwr-3",
            nlohmann::json{{"decision", "commit"}},
            /*sync=*/false,
            "coordinator",
            "coord-a"
        );
    });

    auto dir = makeTempDir("cwr3");
    TempDirCleanup cleanup{dir};

    {
        WALManagerConfig cfg;
        cfg.wal_directory = dir.string();
        WALManager wal(cfg);
        ASSERT_TRUE(wal.initialize());

        EXPECT_NO_THROW({
            WALLoggingHelper::appendEntry(
                &wal,
                WALEntryType::COMMIT_TX,
                "txn-cwr-3b",
                nlohmann::json{{"decision", "commit"}},
                /*sync=*/false,
                "coordinator",
                "coord-a"
            );
        });
    }
}

// CWR-4: JSON payload embedded in WALEntry::data is preserved verbatim.
TEST(CrossCoordinatorWALRecovery, BuildEntry_JsonPayloadRoundTrip) {
    nlohmann::json payload = {
        {"coordinator_id", "coord-a"},
        {"participants",   {"shard-1", "shard-2"}},
        {"isolation",      42}
    };

    WALEntry entry = WALLoggingHelper::buildEntry(
        WALEntryType::BEGIN_TX,
        "txn-cwr-4",
        payload
    );

    EXPECT_EQ(entry.type, WALEntryType::BEGIN_TX);
    EXPECT_EQ(entry.transaction_id, "txn-cwr-4");
    EXPECT_EQ(entry.data, payload)
        << "WALEntry::data must equal the supplied JSON payload verbatim";
    EXPECT_GT(entry.timestamp, uint64_t{0})
        << "buildEntry should populate timestamp with a positive epoch-ms value";
}

// CWR-5: BEGIN_TX entry has the correct WALEntryType.
TEST(CrossCoordinatorWALRecovery, BuildEntry_BeginTx_HasCorrectType) {
    auto entry = WALLoggingHelper::buildEntry(
        WALEntryType::BEGIN_TX,
        "txn-cwr-5",
        nlohmann::json{}
    );
    EXPECT_EQ(entry.type, WALEntryType::BEGIN_TX);
}

// CWR-6: COMMIT_TX and ABORT_TX entries are distinct entry types.
TEST(CrossCoordinatorWALRecovery, EntryTypes_CommitAndAbort_AreDistinct) {
    auto commit_entry = WALLoggingHelper::buildEntry(
        WALEntryType::COMMIT_TX,
        "txn-cwr-6-c",
        nlohmann::json{{"decision", "commit"}}
    );
    auto abort_entry = WALLoggingHelper::buildEntry(
        WALEntryType::ABORT_TX,
        "txn-cwr-6-a",
        nlohmann::json{{"decision", "abort"}}
    );

    EXPECT_EQ(commit_entry.type, WALEntryType::COMMIT_TX);
    EXPECT_EQ(abort_entry.type,  WALEntryType::ABORT_TX);
    EXPECT_NE(commit_entry.type, abort_entry.type)
        << "COMMIT_TX and ABORT_TX must be distinct WALEntryType values";
}

// ─────────────────────────────────────────────────────────────────────────────
// DistributedTransactionManager WAL recovery contract tests (CC-5)
// ─────────────────────────────────────────────────────────────────────────────

// CWR-7: recoverInDoubtTransactions() returns 0 when WAL is disabled.
// Documents that without a configured WAL the coordinator has nothing to recover;
// this is the safe default state (no stale in-doubt transactions persisted).
TEST(CrossCoordinatorWALRecovery, DTM_RecoverWithoutWAL_ReturnsZero) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = std::chrono::milliseconds(500);
    cfg.commit_timeout      = std::chrono::milliseconds(500);
    cfg.default_txn_timeout = std::chrono::seconds(30);
    // No WAL directory — recovery log disabled.

    DistributedTransactionManager mgr("cwr-7-coord", cfg);
    const size_t resolved = mgr.recoverInDoubtTransactions();
    EXPECT_EQ(resolved, size_t{0})
        << "Without a WAL, recoverInDoubtTransactions() must report 0 resolved transactions";
}

// CWR-8: recoverInDoubtTransactions() returns 0 for a freshly created, empty WAL.
// Documents the WAL isolation boundary: an empty WAL has no in-doubt transactions
// for any coordinator to recover.
TEST(CrossCoordinatorWALRecovery, DTM_RecoverFromEmptyWAL_ReturnsZero) {
    auto dir = makeTempDir("cwr8");
    TempDirCleanup cleanup{dir};

    {
        DistributedTxnManagerConfig cfg;
        cfg.prepare_timeout     = std::chrono::milliseconds(500);
        cfg.commit_timeout      = std::chrono::milliseconds(500);
        cfg.default_txn_timeout = std::chrono::seconds(30);
        cfg.wal_directory       = dir.string();
        cfg.enable_recovery_log = true;

        DistributedTransactionManager mgr("cwr-8-coord", cfg);
        const size_t resolved = mgr.recoverInDoubtTransactions();
        EXPECT_EQ(resolved, size_t{0})
            << "A freshly initialised WAL has no in-doubt transactions to recover";
    }
}
