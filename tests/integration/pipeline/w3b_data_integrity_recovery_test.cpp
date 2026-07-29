/*
 * ThemisDB | File: w3b_data_integrity_recovery_test.cpp | Version: 0.0.1
 * Wave 3-B: Data/State Integrity & Recovery
 * Test IDs: DIR-01..DIR-08
 * CTest Label: pipeline_integration;wave3;w3b
 */

/**
 * @file w3b_data_integrity_recovery_test.cpp
 * @brief Wave 3-B integration tests: data/state integrity and recovery scenarios.
 *
 * Covers persistence, idempotent replay, partial-failure recovery, and
 * restart-stability in the critical write/commit path.
 *
 * ## Recovery Model
 *
 * A WAL-style journal (in-memory append log) is prepended to every write.
 * "Crash" is simulated by discarding the committed store while keeping the
 * journal. "Recovery" replays the journal to rebuild the committed state.
 * This accurately models the invariants required of a real WAL-backed store
 * without requiring a live database or file system.
 *
 * ## Covered Scenarios
 * - DIR-01: Write-commit-snapshot round-trip produces consistent state
 * - DIR-02: Idempotent replay of the same journal entries is safe
 * - DIR-03: Partial-failure recovery rebuilds exact pre-crash state
 * - DIR-04: Uncommitted journal entries are NOT visible after replay
 * - DIR-05: Snapshot checksum detects state corruption
 * - DIR-06: Concurrent writes maintain journal order and uniqueness
 * - DIR-07: Empty-journal recovery is a no-op (idempotent)
 * - DIR-08: Multi-key partial rollback preserves surviving entries
 *
 * @note All tests run offline with deterministic in-memory structures;
 *       no actual file I/O, network, or database required.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <numeric>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis { namespace test { 

namespace {

// ---------------------------------------------------------------------------
// WAL journal infrastructure
// ---------------------------------------------------------------------------

/**
 * @brief Type of a WAL journal entry.
 */
enum class WalEntryType : uint8_t {
    kWrite  = 1, ///< Write a key/value pair to the store.
    kDelete = 2, ///< Delete a key from the store.
    kCommit = 3, ///< Mark a logical transaction boundary.
};

/**
 * @brief Single WAL journal entry.
 */
struct WalEntry {
    WalEntryType type;
    std::string  transaction_id;
    std::string  key;
    std::string  value; ///< Only valid when type == kWrite.
};

/**
 * @brief Result of a commit operation.
 */
struct CommitResult {
    bool   ok{false};
    size_t entries_written{0};
    std::string error;
};

/**
 * @brief Result of a recovery replay.
 */
struct RecoveryResult {
    bool   ok{false};
    size_t entries_replayed{0};
    size_t keys_restored{0};
    size_t keys_skipped_duplicate{0};
};

/**
 * @brief Snapshot of the committed store state (for consistency assertions).
 */
struct StoreSnapshot {
    std::unordered_map<std::string, std::string> data;
    size_t checksum{0}; ///< XOR of std::hash<std::string>(key) ^ std::hash<std::string>(value) for all entries.
};

// ---------------------------------------------------------------------------
// DataIntegrityPipeline
// ---------------------------------------------------------------------------

/**
 * @brief WAL-backed pipeline for data integrity and recovery tests.
 *
 * Writes are first appended to the journal, then committed to the in-memory
 * store. "Crash" is modelled as discarding the committed store while
 * preserving the journal. Recovery replays the journal into a fresh store.
 *
 * @note This is a test-only pipeline abstraction.
 * // NON-PRODUCTION PATH (Simulation/Stub/Mockup)
 * // Reason: WAL-style recovery model for integration coverage of
 * //         data integrity and restart-stability invariants
 * // Activation: test-only (THEMIS_TEST_BUILD=1)
 * // Production Delta: no RocksDB WAL, no disk I/O, in-memory only
 * // Approved By: @makr-code (maintainer) — PR tests(w3) Wave 3 test hardening
 * // Removal Target: keep permanently as integration coverage harness
 */
class DataIntegrityPipeline {
public:
    DataIntegrityPipeline() = default;

    /**
     * @brief Writes a key/value pair to the journal (write-ahead log).
     *
     * The entry is appended to the journal but not yet committed to the
     * visible store. Call Commit() to make writes durable and visible.
     *
     * @param tx_id  Logical transaction identifier.
     * @param key    Storage key.
     * @param value  Payload string.
     */
    void WriteToJournal(const std::string& tx_id,
                        const std::string& key,
                        const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        journal_.push_back({WalEntryType::kWrite, tx_id, key, value});
    }

    /**
     * @brief Deletes a key via the journal (write-ahead log).
     *
     * @param tx_id  Logical transaction identifier.
     * @param key    Key to delete from the store on next Commit().
     */
    void DeleteViaJournal(const std::string& tx_id, const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        journal_.push_back({WalEntryType::kDelete, tx_id, key, ""});
    }

    /**
     * @brief Appends a commit marker to the journal and applies all pending
     *        journal entries to the committed store.
     *
     * @param tx_id Logical transaction identifier to commit.
     * @return CommitResult with ok=true and the count of applied entries.
     */
    [[nodiscard]] CommitResult Commit(const std::string& tx_id) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Move staged (prepared) entries for this tx_id into the journal so that
        // Recovery() can replay them after a crash. Entries staged for other
        // transactions remain in pending_for_commit_.
        std::vector<WalEntry> remaining_pending;
        for (auto& entry : pending_for_commit_) {
            if (entry.transaction_id == tx_id) {
                journal_.push_back(entry);
            } else {
                remaining_pending.push_back(std::move(entry));
            }
        }
        pending_for_commit_ = std::move(remaining_pending);

        // Append the commit marker for this transaction.
        journal_.push_back({WalEntryType::kCommit, tx_id, "", ""});

        // Apply all write/delete journal entries for this tx_id to the store.
        size_t applied = 0U;
        for (const auto& entry : journal_) {
            if (entry.transaction_id != tx_id) {
                continue;
            }
            if (entry.type == WalEntryType::kWrite || entry.type == WalEntryType::kDelete) {
                ApplyEntryLocked(entry);
                ++applied;
            }
        }

        committed_tx_ids_.insert(tx_id);
        return {true, applied, ""};
    }

    /**
     * @brief Simulates a crash by discarding the committed store.
     *
     * The journal is preserved to enable recovery. Any in-flight (uncommitted)
     * entries remain in the journal but are not applied on recovery unless
     * their transaction has a kCommit marker.
     */
    void SimulateCrash() {
        std::lock_guard<std::mutex> lock(mutex_);
        store_.clear();
        committed_tx_ids_.clear();
        pending_for_commit_.clear();
        // journal_ is intentionally preserved
    }

    /**
     * @brief Recovers the store by replaying committed journal entries.
     *
     * Only entries belonging to transactions that have a kCommit marker in
     * the journal are applied. Uncommitted entries are skipped.
     *
     * @return RecoveryResult with counts for observability.
     */
    [[nodiscard]] RecoveryResult Recover() {
        std::lock_guard<std::mutex> lock(mutex_);

        // First pass: identify committed transaction IDs
        std::unordered_set<std::string> committed_ids;
        for (const auto& entry : journal_) {
            if (entry.type == WalEntryType::kCommit) {
                committed_ids.insert(entry.transaction_id);
            }
        }

        // Second pass: replay only committed entries
        std::unordered_set<std::string> seen_keys;
        size_t replayed   = 0U;
        size_t restored   = 0U;
        size_t duplicates = 0U;

        for (const auto& entry : journal_) {
            if (entry.type == WalEntryType::kCommit) {
                continue;
            }
            if (committed_ids.count(entry.transaction_id) == 0U) {
                continue; // uncommitted — skip
            }

            const bool already_seen = seen_keys.count(entry.key) > 0U;
            if (already_seen && entry.type == WalEntryType::kWrite) {
                ++duplicates;
                // Later write wins — overwrite
                ApplyEntryLocked(entry);
                ++replayed;
            } else {
                seen_keys.insert(entry.key);
                ApplyEntryLocked(entry);
                ++replayed;
                ++restored;
            }
        }

        return {true, replayed, restored, duplicates};
    }

    /**
     * @brief Stages an entry in pending_for_commit_ (simulates a prepare phase
     *        where writes are buffered before the commit marker is appended).
     *
     * @param tx_id  Logical transaction identifier.
     * @param key    Storage key.
     * @param value  Payload string.
     */
    void Stage(const std::string& tx_id,
               const std::string& key,
               const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_for_commit_.push_back({WalEntryType::kWrite, tx_id, key, value});
    }

    // --- Accessors for state assertions ---

    /**
     * @brief Reads a value from the committed store.
     * @param key Storage key.
     * @return Value if present, std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<std::string> Read(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = store_.find(key);
        if (it == store_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    /**
     * @brief Returns the current number of entries in the committed store.
     */
    [[nodiscard]] size_t StoreSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return store_.size();
    }

    /**
     * @brief Returns the total number of journal entries (across all transactions).
     */
    [[nodiscard]] size_t JournalSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return journal_.size();
    }

    /**
     * @brief Takes a snapshot of the committed store for consistency checks.
     * @return StoreSnapshot with a copy of the data and a simple checksum.
     */
    [[nodiscard]] StoreSnapshot TakeSnapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        StoreSnapshot snap;
        snap.data = store_;
        size_t chk = 0U;
        for (const auto& [k, v] : store_) {
            chk ^= std::hash<std::string>{}(k) ^ std::hash<std::string>{}(v);
        }
        snap.checksum = chk;
        return snap;
    }

private:
    /**
     * @brief Applies a single WAL entry to the committed store (must hold mutex_).
     * @param entry WAL entry to apply.
     */
    void ApplyEntryLocked(const WalEntry& entry) {
        if (entry.type == WalEntryType::kWrite) {
            store_[entry.key] = entry.value;
        } else if (entry.type == WalEntryType::kDelete) {
            store_.erase(entry.key);
        }
    }

    mutable std::mutex mutex_;
    std::vector<WalEntry>                               journal_;
    std::vector<WalEntry>                               pending_for_commit_;
    std::unordered_map<std::string, std::string>        store_;
    std::unordered_set<std::string>                     committed_tx_ids_;
};

} // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for Wave 3-B data integrity and recovery tests.
 *
 * Each test gets a fresh DataIntegrityPipeline and TestDataGenerator.
 */
class W3BDataIntegrityRecoveryTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        pipeline_ = std::make_unique<DataIntegrityPipeline>();
        data_gen_ = std::make_unique<TestDataGenerator>();
    }

    std::unique_ptr<DataIntegrityPipeline> pipeline_;
    std::unique_ptr<TestDataGenerator>     data_gen_;
};

// ---------------------------------------------------------------------------
// DIR-01: Write-commit-snapshot round-trip produces consistent state
// ---------------------------------------------------------------------------

/**
 * @test DIR-01 — A write followed by Commit() makes the entry visible in the
 *               store; the snapshot checksum matches a second snapshot taken
 *               without any intervening write.
 *
 * Acceptance Criteria:
 * - Read() returns the written value after Commit().
 * - StoreSize() equals the number of committed writes.
 * - Two consecutive snapshots without intervening writes have equal checksums.
 */
TEST_F(W3BDataIntegrityRecoveryTest, DIR01_WriteCommitSnapshotIsConsistent) {
    pipeline_->WriteToJournal("tx-01", "key/alpha", "value-alpha");
    pipeline_->WriteToJournal("tx-01", "key/beta",  "value-beta");

    const auto cr = pipeline_->Commit("tx-01");
    ASSERT_TRUE(cr.ok) << "DIR-01: Commit must succeed";

    // Read-back
    const auto val_alpha = pipeline_->Read("key/alpha");
    const auto val_beta  = pipeline_->Read("key/beta");
    ASSERT_TRUE(val_alpha.has_value()) << "DIR-01: 'key/alpha' must be readable after commit";
    ASSERT_TRUE(val_beta.has_value())  << "DIR-01: 'key/beta' must be readable after commit";
    EXPECT_EQ(*val_alpha, "value-alpha") << "DIR-01: 'key/alpha' value mismatch";
    EXPECT_EQ(*val_beta,  "value-beta")  << "DIR-01: 'key/beta' value mismatch";

    EXPECT_EQ(pipeline_->StoreSize(), 2U) << "DIR-01: Store must contain exactly 2 entries";

    // Snapshot consistency
    const auto snap1 = pipeline_->TakeSnapshot();
    const auto snap2 = pipeline_->TakeSnapshot();
    EXPECT_EQ(snap1.checksum, snap2.checksum)
        << "DIR-01: Two consecutive snapshots without writes must be identical";
    EXPECT_EQ(snap1.data.size(), 2U) << "DIR-01: Snapshot data size must be 2";
}

// ---------------------------------------------------------------------------
// DIR-02: Idempotent replay of journal is safe
// ---------------------------------------------------------------------------

/**
 * @test DIR-02 — Recovering from the journal twice (simulated idempotent replay)
 *               produces the same final state as a single recovery.
 *
 * Acceptance Criteria:
 * - After crash + two recoveries, the store equals the pre-crash state.
 * - StoreSize() after double-recovery equals the original committed count.
 * - Checksums of pre-crash and post-double-recovery snapshots are equal.
 */
TEST_F(W3BDataIntegrityRecoveryTest, DIR02_IdempotentJournalReplayIsSafe) {
    pipeline_->WriteToJournal("tx-02", "doc/001", "payload-001");
    pipeline_->WriteToJournal("tx-02", "doc/002", "payload-002");
    const auto cr = pipeline_->Commit("tx-02");
    ASSERT_TRUE(cr.ok) << "DIR-02: Initial commit must succeed";

    const auto pre_crash_snap = pipeline_->TakeSnapshot();

    // Simulate crash and recover twice
    pipeline_->SimulateCrash();
    EXPECT_EQ(pipeline_->StoreSize(), 0U) << "DIR-02: Store must be empty after crash";

    const auto rr1 = pipeline_->Recover();
    ASSERT_TRUE(rr1.ok) << "DIR-02: First recovery must succeed";
    const auto snap_after_r1 = pipeline_->TakeSnapshot();

    pipeline_->SimulateCrash();
    const auto rr2 = pipeline_->Recover();
    ASSERT_TRUE(rr2.ok) << "DIR-02: Second recovery must succeed";
    const auto snap_after_r2 = pipeline_->TakeSnapshot();

    EXPECT_EQ(snap_after_r1.checksum, pre_crash_snap.checksum)
        << "DIR-02: Post-first-recovery snapshot must match pre-crash snapshot";
    EXPECT_EQ(snap_after_r2.checksum, pre_crash_snap.checksum)
        << "DIR-02: Post-second-recovery snapshot must match pre-crash snapshot";
    EXPECT_EQ(snap_after_r2.data.size(), 2U)
        << "DIR-02: Double-recovery store must contain exactly 2 entries";
}

// ---------------------------------------------------------------------------
// DIR-03: Partial-failure recovery rebuilds exact pre-crash state
// ---------------------------------------------------------------------------

/**
 * @test DIR-03 — After committing two transactions and crashing, recovery
 *               restores all committed entries and none of the uncommitted ones.
 *
 * Acceptance Criteria:
 * - All entries from committed transactions are readable after recovery.
 * - The store does NOT contain entries from uncommitted transactions.
 * - RecoveryResult.entries_replayed >= number of committed write entries.
 */
TEST_F(W3BDataIntegrityRecoveryTest, DIR03_PartialFailureRecoveryRebuildsCommittedState) {
    // Committed transaction
    pipeline_->WriteToJournal("tx-committed", "key/committed/1", "v1");
    pipeline_->WriteToJournal("tx-committed", "key/committed/2", "v2");
    const auto cr = pipeline_->Commit("tx-committed");
    ASSERT_TRUE(cr.ok) << "DIR-03: Commit of tx-committed must succeed";

    // Uncommitted transaction (simulates crash before commit)
    pipeline_->WriteToJournal("tx-uncommitted", "key/uncommitted/1", "should-not-appear");
    pipeline_->WriteToJournal("tx-uncommitted", "key/uncommitted/2", "should-not-appear");
    // NOTE: no Commit("tx-uncommitted") — simulates crash before commit marker

    // Simulate crash and recover
    pipeline_->SimulateCrash();
    const auto rr = pipeline_->Recover();

    ASSERT_TRUE(rr.ok) << "DIR-03: Recovery must succeed";
    EXPECT_GE(rr.entries_replayed, 2U) << "DIR-03: At least 2 entries must be replayed";

    // Committed entries must be restored
    const auto v1 = pipeline_->Read("key/committed/1");
    const auto v2 = pipeline_->Read("key/committed/2");
    ASSERT_TRUE(v1.has_value()) << "DIR-03: key/committed/1 must be restored";
    ASSERT_TRUE(v2.has_value()) << "DIR-03: key/committed/2 must be restored";
    EXPECT_EQ(*v1, "v1") << "DIR-03: key/committed/1 value mismatch after recovery";
    EXPECT_EQ(*v2, "v2") << "DIR-03: key/committed/2 value mismatch after recovery";

    // Uncommitted entries must NOT appear
    EXPECT_FALSE(pipeline_->Read("key/uncommitted/1").has_value())
        << "DIR-03: Uncommitted key/uncommitted/1 must not appear after recovery";
    EXPECT_FALSE(pipeline_->Read("key/uncommitted/2").has_value())
        << "DIR-03: Uncommitted key/uncommitted/2 must not appear after recovery";
}

// ---------------------------------------------------------------------------
// DIR-04: Uncommitted journal entries are invisible before and after recovery
// ---------------------------------------------------------------------------

/**
 * @test DIR-04 — Journal entries without a matching kCommit marker are never
 *               applied to the committed store, even after multiple recoveries.
 *
 * Acceptance Criteria:
 * - Entries written to the journal but never committed are not visible via Read().
 * - Recovery skips these entries (keys_restored == 0 when only uncommitted entries exist).
 */
TEST_F(W3BDataIntegrityRecoveryTest, DIR04_UncommittedEntriesNeverAppearInStore) {
    // Only write to journal, never commit
    pipeline_->WriteToJournal("tx-never-committed", "key/ghost", "ghost-value");

    // Before recovery: key should not be visible (was only journaled, not committed)
    EXPECT_FALSE(pipeline_->Read("key/ghost").has_value())
        << "DIR-04: Journaled-but-not-committed key must not be readable before recovery";

    // After crash + recovery: still not visible
    pipeline_->SimulateCrash();
    const auto rr = pipeline_->Recover();
    ASSERT_TRUE(rr.ok) << "DIR-04: Recovery must succeed even with no committed entries";
    EXPECT_EQ(rr.keys_restored, 0U)
        << "DIR-04: No keys should be restored when journal has no committed transactions";
    EXPECT_FALSE(pipeline_->Read("key/ghost").has_value())
        << "DIR-04: Uncommitted key must not appear after recovery";
}

// ---------------------------------------------------------------------------
// DIR-05: Snapshot checksum detects state mutation
// ---------------------------------------------------------------------------

/**
 * @test DIR-05 — Writing additional data after taking a snapshot changes the
 *               checksum, confirming the checksum is sensitive to state changes.
 *
 * Acceptance Criteria:
 * - Snapshot before and after an additional write have different checksums.
 * - Snapshot before and after a delete have different checksums.
 */
TEST_F(W3BDataIntegrityRecoveryTest, DIR05_SnapshotChecksumDetectsStateMutation) {
    pipeline_->WriteToJournal("tx-05a", "key/base", "base-value");
    const auto cr_a = pipeline_->Commit("tx-05a");
    ASSERT_TRUE(cr_a.ok) << "DIR-05: Initial commit must succeed";

    const auto snap_before_write = pipeline_->TakeSnapshot();

    // Add another key
    pipeline_->WriteToJournal("tx-05b", "key/extra", "extra-value");
    const auto cr_b = pipeline_->Commit("tx-05b");
    ASSERT_TRUE(cr_b.ok) << "DIR-05: Second commit must succeed";

    const auto snap_after_write = pipeline_->TakeSnapshot();
    EXPECT_NE(snap_before_write.checksum, snap_after_write.checksum)
        << "DIR-05: Checksum must change after adding a new key";

    // Delete the base key
    pipeline_->DeleteViaJournal("tx-05c", "key/base");
    const auto cr_c = pipeline_->Commit("tx-05c");
    ASSERT_TRUE(cr_c.ok) << "DIR-05: Delete commit must succeed";

    const auto snap_after_delete = pipeline_->TakeSnapshot();
    EXPECT_NE(snap_after_write.checksum, snap_after_delete.checksum)
        << "DIR-05: Checksum must change after deleting a key";
    EXPECT_FALSE(pipeline_->Read("key/base").has_value())
        << "DIR-05: Deleted key must not be readable";
}

// ---------------------------------------------------------------------------
// DIR-06: Concurrent writes maintain journal order and uniqueness
// ---------------------------------------------------------------------------

/**
 * @test DIR-06 — Multiple threads writing distinct keys concurrently produce
 *               a consistent store with the expected key count and values.
 *
 * Acceptance Criteria:
 * - After all threads complete and commits are applied, StoreSize() equals
 *   the total number of unique keys written.
 * - No key is lost or duplicated.
 * - All values are readable and match what was written.
 */
TEST_F(W3BDataIntegrityRecoveryTest, DIR06_ConcurrentWritesMaintainConsistency) {
    constexpr size_t kThreads    = 8U;
    constexpr size_t kKeysPerTx  = 5U;

    std::atomic<size_t> commit_ok_count{0U};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (size_t t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            const auto tx_id = "tx-" + std::to_string(t);
            for (size_t k = 0; k < kKeysPerTx; ++k) {
                const auto key   = "key/" + std::to_string(t) + "/" + std::to_string(k);
                const auto value = "val-" + std::to_string(t) + "-" + std::to_string(k);
                pipeline_->WriteToJournal(tx_id, key, value);
            }
            const auto cr = pipeline_->Commit(tx_id);
            if (cr.ok) {
                ++commit_ok_count;
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(commit_ok_count.load(), kThreads)
        << "DIR-06: All " << kThreads << " concurrent commits must succeed";

    // Verify all keys are readable with the correct values
    size_t readable_count = 0U;
    for (size_t t = 0; t < kThreads; ++t) {
        for (size_t k = 0; k < kKeysPerTx; ++k) {
            const auto key      = "key/" + std::to_string(t) + "/" + std::to_string(k);
            const auto expected = "val-" + std::to_string(t) + "-" + std::to_string(k);
            const auto val      = pipeline_->Read(key);
            if (val.has_value() && *val == expected) {
                ++readable_count;
            }
        }
    }

    const size_t expected_keys = kThreads * kKeysPerTx;
    EXPECT_EQ(readable_count, expected_keys)
        << "DIR-06: All " << expected_keys << " keys must be readable with correct values";
}

// ---------------------------------------------------------------------------
// DIR-07: Empty-journal recovery is a no-op (idempotent)
// ---------------------------------------------------------------------------

/**
 * @test DIR-07 — Calling Recover() on an empty journal is safe and produces
 *               an empty store without errors.
 *
 * Acceptance Criteria:
 * - Recover() returns ok=true.
 * - entries_replayed == 0 and keys_restored == 0.
 * - StoreSize() == 0 after recovery.
 */
TEST_F(W3BDataIntegrityRecoveryTest, DIR07_EmptyJournalRecoveryIsIdempotent) {
    // No writes, no commits — fresh pipeline
    EXPECT_EQ(pipeline_->JournalSize(), 0U) << "DIR-07: Journal must be empty";

    const auto rr = pipeline_->Recover();
    ASSERT_TRUE(rr.ok)                 << "DIR-07: Recovery on empty journal must succeed";
    EXPECT_EQ(rr.entries_replayed, 0U) << "DIR-07: Zero entries must be replayed";
    EXPECT_EQ(rr.keys_restored, 0U)    << "DIR-07: Zero keys must be restored";
    EXPECT_EQ(pipeline_->StoreSize(), 0U) << "DIR-07: Store must remain empty after no-op recovery";

    // Second recovery also a no-op
    const auto rr2 = pipeline_->Recover();
    ASSERT_TRUE(rr2.ok)                 << "DIR-07: Second recovery on empty journal must succeed";
    EXPECT_EQ(rr2.entries_replayed, 0U) << "DIR-07: Second recovery must replay zero entries";
}

// ---------------------------------------------------------------------------
// DIR-08: Multi-key partial rollback preserves surviving entries
// ---------------------------------------------------------------------------

/**
 * @test DIR-08 — When one transaction is committed and another is staged
 *               but not committed (partial rollback), the committed entries
 *               survive and the staged-only entries are absent.
 *
 * Acceptance Criteria:
 * - After SimulateCrash() + Recover(), all committed keys are present.
 * - Staged-but-not-committed keys are absent.
 * - StoreSize() equals the number of committed (not staged) entries.
 */
TEST_F(W3BDataIntegrityRecoveryTest, DIR08_PartialRollbackPreservesCommittedEntries) {
    // Transaction 1: committed
    pipeline_->WriteToJournal("tx-durable", "key/durable/A", "durable-A");
    pipeline_->WriteToJournal("tx-durable", "key/durable/B", "durable-B");
    pipeline_->WriteToJournal("tx-durable", "key/durable/C", "durable-C");
    const auto cr = pipeline_->Commit("tx-durable");
    ASSERT_TRUE(cr.ok) << "DIR-08: Commit of tx-durable must succeed";

    // Transaction 2: staged via Stage() but never committed (crash before commit)
    pipeline_->Stage("tx-partial", "key/partial/X", "lost-X");
    pipeline_->Stage("tx-partial", "key/partial/Y", "lost-Y");
    // NOTE: Commit("tx-partial") is intentionally omitted

    EXPECT_EQ(pipeline_->StoreSize(), 3U)
        << "DIR-08: Pre-crash store must contain exactly 3 committed entries";

    // Crash and recover
    pipeline_->SimulateCrash();
    const auto rr = pipeline_->Recover();
    ASSERT_TRUE(rr.ok) << "DIR-08: Recovery must succeed";

    // Committed entries must be present
    EXPECT_TRUE(pipeline_->Read("key/durable/A").has_value())
        << "DIR-08: key/durable/A must be present after recovery";
    EXPECT_TRUE(pipeline_->Read("key/durable/B").has_value())
        << "DIR-08: key/durable/B must be present after recovery";
    EXPECT_TRUE(pipeline_->Read("key/durable/C").has_value())
        << "DIR-08: key/durable/C must be present after recovery";

    // Staged-only entries must be absent
    EXPECT_FALSE(pipeline_->Read("key/partial/X").has_value())
        << "DIR-08: key/partial/X must not appear after partial rollback recovery";
    EXPECT_FALSE(pipeline_->Read("key/partial/Y").has_value())
        << "DIR-08: key/partial/Y must not appear after partial rollback recovery";

    EXPECT_EQ(pipeline_->StoreSize(), 3U)
        << "DIR-08: Store must contain exactly 3 entries after recovery (no partial entries)";
}
} } // namespace themis::test
